#include <catch2/catch_test_macros.hpp>
#include "lexer/TokenStream.h"
#include "managers/SourceManager.h"
#include "modules/Module.h"
#include "modules/ModuleManager.h"
#include "utils/FileSystem.h"
#include "utils/StringPool.h"

// Test fixture: creates a fully-wired lexer from a source string
struct LexerFixture {
    sw::FileSystem  fs;
    sw::StringPool  pool{4096};
    ModuleManager   modman;
    Module*         mod;
    SourceManager*  sm;
    TokenStream*    lex;

    explicit LexerFixture(std::string_view source) {
        auto* fh = fs.createVirtualFile("test.sw", std::string(source));
        ModuleContext ctx{fh, modman, pool};
        mod = modman.insert(ctx);
        sm  = new SourceManager(mod);
        lex = new TokenStream(*sm);
    }

    ~LexerFixture() {
        delete lex;
        delete sm;
    }

    Token next() { return lex->next(); }
};

// Capture token once, then check all fields
#define CHECK_TOK(next_call, cat, val, id) do { \
    auto _t_ = (next_call);                     \
    CHECK(_t_.type    == (cat));                \
    CHECK(_t_.value   == (val));                \
    CHECK(_t_.tokenid == (id));                 \
} while(0)

// ──────────────────────────────────────────────────────────
// Empty & whitespace-only input
// ──────────────────────────────────────────────────────────
TEST_CASE("Lexer handles empty input", "[lexer]") {
    LexerFixture f("");
    auto tok = f.next();
    CHECK(tok.type == NONE);
    CHECK(f.lex->eof());
}

TEST_CASE("Lexer handles whitespace-only input", "[lexer]") {
    SECTION("spaces") {
        LexerFixture f("   ");
        CHECK(f.next().type == NONE);
    }
    SECTION("tabs") {
        LexerFixture f("\t\t");
        CHECK(f.next().type == NONE);
    }
    SECTION("newlines") {
        LexerFixture f("\n\n\n");
        CHECK(f.next().type == NONE);
    }
    SECTION("mixed") {
        LexerFixture f(" \t\n \t\n");
        CHECK(f.next().type == NONE);
    }
}

// ──────────────────────────────────────────────────────────
// Keywords
// ──────────────────────────────────────────────────────────
TEST_CASE("Lexer recognizes all keywords", "[lexer][keywords]") {
    LexerFixture f(
        "return if else for while mut true false undefined "
        "enum protocol const static break continue elif extern "
        "comptime let import export var fn volatile struct"
    );

    CHECK_TOK(f.next(), KEYWORD, "return",     Token::KW_RETURN);
    CHECK_TOK(f.next(), KEYWORD, "if",         Token::KW_IF);
    CHECK_TOK(f.next(), KEYWORD, "else",       Token::KW_ELSE);
    CHECK_TOK(f.next(), KEYWORD, "for",        Token::KW_FOR);
    CHECK_TOK(f.next(), KEYWORD, "while",      Token::KW_WHILE);
    CHECK_TOK(f.next(), KEYWORD, "mut",        Token::KW_MUT);
    CHECK_TOK(f.next(), KEYWORD, "true",       Token::KW_TRUE);
    CHECK_TOK(f.next(), KEYWORD, "false",      Token::KW_FALSE);
    CHECK_TOK(f.next(), KEYWORD, "undefined",  Token::KW_UNDEFINED);
    CHECK_TOK(f.next(), KEYWORD, "enum",       Token::KW_ENUM);
    CHECK_TOK(f.next(), KEYWORD, "protocol",   Token::KW_PROTOCOL);
    CHECK_TOK(f.next(), KEYWORD, "const",      Token::KW_CONST);
    CHECK_TOK(f.next(), KEYWORD, "static",     Token::KW_STATIC);
    CHECK_TOK(f.next(), KEYWORD, "break",      Token::KW_BREAK);
    CHECK_TOK(f.next(), KEYWORD, "continue",   Token::KW_CONTINUE);
    CHECK_TOK(f.next(), KEYWORD, "elif",       Token::KW_ELIF);
    CHECK_TOK(f.next(), KEYWORD, "extern",     Token::KW_EXTERN);
    CHECK_TOK(f.next(), KEYWORD, "comptime",   Token::KW_COMPTIME);
    CHECK_TOK(f.next(), KEYWORD, "let",        Token::KW_LET);
    CHECK_TOK(f.next(), KEYWORD, "import",     Token::KW_IMPORT);
    CHECK_TOK(f.next(), KEYWORD, "export",     Token::KW_EXPORT);
    CHECK_TOK(f.next(), KEYWORD, "var",        Token::KW_VAR);
    CHECK_TOK(f.next(), KEYWORD, "fn",         Token::KW_FN);
    CHECK_TOK(f.next(), KEYWORD, "volatile",   Token::KW_VOLATILE);
    CHECK_TOK(f.next(), KEYWORD, "struct",     Token::KW_STRUCT);
    CHECK(f.next().type == NONE);
}

TEST_CASE("'as' is treated as operator, not keyword", "[lexer][keywords]") {
    LexerFixture f("as");
    CHECK_TOK(f.next(), OP, "as", Token::OP_AS);
    CHECK(f.next().type == NONE);
}

TEST_CASE("keyword-like identifiers are not keywords", "[lexer][keywords]") {
    LexerFixture f("Struct IF");
    CHECK_TOK(f.next(), IDENT, "Struct", Token::IDENT);
    CHECK_TOK(f.next(), IDENT, "IF", Token::IDENT);
    CHECK(f.next().type == NONE);
}

TEST_CASE("keyword adjacent to identifier is parsed as ident", "[lexer][keywords]") {
    LexerFixture f("letx");
    CHECK_TOK(f.next(), IDENT, "letx", Token::IDENT);
    CHECK(f.next().type == NONE);
}

// ──────────────────────────────────────────────────────────
// Identifiers
// ──────────────────────────────────────────────────────────
TEST_CASE("Lexer handles identifiers", "[lexer][idents]") {
    SECTION("simple") {
        LexerFixture f("foo");
        CHECK_TOK(f.next(), IDENT, "foo", Token::IDENT);
    }
    SECTION("underscore prefix") {
        LexerFixture f("_foo");
        CHECK_TOK(f.next(), IDENT, "_foo", Token::IDENT);
    }
    SECTION("only underscore") {
        LexerFixture f("_");
        CHECK_TOK(f.next(), IDENT, "_", Token::IDENT);
    }
    SECTION("underscore with digits") {
        LexerFixture f("_123");
        CHECK_TOK(f.next(), IDENT, "_123", Token::IDENT);
    }
    SECTION("mixed case") {
        LexerFixture f("Foo Bar");
        CHECK_TOK(f.next(), IDENT, "Foo", Token::IDENT);
        CHECK_TOK(f.next(), IDENT, "Bar", Token::IDENT);
    }
    SECTION("single letter") {
        LexerFixture f("x y z");
        CHECK_TOK(f.next(), IDENT, "x", Token::IDENT);
        CHECK_TOK(f.next(), IDENT, "y", Token::IDENT);
        CHECK_TOK(f.next(), IDENT, "z", Token::IDENT);
    }
    SECTION("identifier with embedded digits") {
        LexerFixture f("a1b2");
        CHECK_TOK(f.next(), IDENT, "a1b2", Token::IDENT);
    }
}

// ──────────────────────────────────────────────────────────
// Punctuation
// ──────────────────────────────────────────────────────────
TEST_CASE("Lexer recognizes punctuation", "[lexer][punc]") {
    LexerFixture f("{}()[],;#");
    CHECK_TOK(f.next(), PUNC, "{", Token::PUNC_LBRACE);
    CHECK_TOK(f.next(), PUNC, "}", Token::PUNC_RBRACE);
    CHECK_TOK(f.next(), PUNC, "(", Token::PUNC_LPAREN);
    CHECK_TOK(f.next(), PUNC, ")", Token::PUNC_RPAREN);
    CHECK_TOK(f.next(), PUNC, "[", Token::PUNC_LBRACKET);
    CHECK_TOK(f.next(), PUNC, "]", Token::PUNC_RBRACKET);
    CHECK_TOK(f.next(), PUNC, ",", Token::PUNC_COMMA);
    CHECK_TOK(f.next(), PUNC, ";", Token::PUNC_SEMI);
    CHECK_TOK(f.next(), PUNC, "#", Token::PUNC_HASH);
    CHECK(f.next().type == NONE);
}

// ──────────────────────────────────────────────────────────
// Operators
// ──────────────────────────────────────────────────────────
TEST_CASE("Lexer recognizes single-char operators", "[lexer][operators]") {
    LexerFixture f("= + - * / % . ! < > | & :");
    CHECK_TOK(f.next(), OP, "=", Token::OP_ASSIGN);
    CHECK_TOK(f.next(), OP, "+", Token::OP_PLUS);
    CHECK_TOK(f.next(), OP, "-", Token::OP_MINUS);
    CHECK_TOK(f.next(), OP, "*", Token::OP_MUL);
    CHECK_TOK(f.next(), OP, "/", Token::OP_DIV);
    CHECK_TOK(f.next(), OP, "%", Token::OP_MOD);
    CHECK_TOK(f.next(), OP, ".", Token::OP_DOT);
    CHECK_TOK(f.next(), OP, "!", Token::OP_NOT);
    CHECK_TOK(f.next(), OP, "<", Token::OP_LT);
    CHECK_TOK(f.next(), OP, ">", Token::OP_GT);
    CHECK_TOK(f.next(), OP, "|", Token::OP_BITWISE_OR);
    CHECK_TOK(f.next(), OP, "&", Token::OP_BITWISE_AND);
    CHECK_TOK(f.next(), OP, ":", Token::PUNC_COLON);
    CHECK(f.next().type == NONE);
}

TEST_CASE("Lexer recognizes compound operators", "[lexer][operators]") {
    LexerFixture f("== += -= *= /= %= != <= >= || && ::");
    CHECK_TOK(f.next(), OP, "==", Token::OP_EQ);
    CHECK_TOK(f.next(), OP, "+=", Token::OP_PLUS_ASSIGN);
    CHECK_TOK(f.next(), OP, "-=", Token::OP_MINUS_ASSIGN);
    CHECK_TOK(f.next(), OP, "*=", Token::OP_MUL_ASSIGN);
    CHECK_TOK(f.next(), OP, "/=", Token::OP_DIV_ASSIGN);
    CHECK_TOK(f.next(), OP, "%=", Token::OP_MOD_ASSIGN);
    CHECK_TOK(f.next(), OP, "!=", Token::OP_NOT_EQ);
    CHECK_TOK(f.next(), OP, "<=", Token::OP_LT_EQ);
    CHECK_TOK(f.next(), OP, ">=", Token::OP_GT_EQ);
    CHECK_TOK(f.next(), OP, "||", Token::OP_LOGICAL_OR);
    CHECK_TOK(f.next(), OP, "&&", Token::OP_LOGICAL_AND);
    CHECK_TOK(f.next(), OP, "::", Token::OP_SCOPE_RES);
    CHECK(f.next().type == NONE);
}

TEST_CASE("Lexer recognizes ellipsis", "[lexer][operators]") {
    LexerFixture f("...");
    CHECK_TOK(f.next(), OP, "...", Token::OP_ELLIPSIS);
    CHECK(f.next().type == NONE);
}

TEST_CASE("adjacent operators tokenized greedily", "[lexer][operators]") {
    LexerFixture f("++--");
    CHECK_TOK(f.next(), OP, "+", Token::OP_PLUS);
    CHECK_TOK(f.next(), OP, "+", Token::OP_PLUS);
    CHECK_TOK(f.next(), OP, "-", Token::OP_MINUS);
    CHECK_TOK(f.next(), OP, "-", Token::OP_MINUS);
    CHECK(f.next().type == NONE);
}

// ──────────────────────────────────────────────────────────
// Comments
// ──────────────────────────────────────────────────────────
TEST_CASE("Lexer discards line comments", "[lexer][comments]") {
    SECTION("comment only") {
        LexerFixture f("// hello world\n");
        CHECK(f.next().type == NONE);
    }
    SECTION("code after comment") {
        LexerFixture f("// comment\nlet");
        CHECK_TOK(f.next(), KEYWORD, "let", Token::KW_LET);
    }
    SECTION("comment mid-line") {
        LexerFixture f("let // inline\nx");
        CHECK_TOK(f.next(), KEYWORD, "let", Token::KW_LET);
        CHECK_TOK(f.next(), IDENT,   "x",   Token::IDENT);
    }
}

// ──────────────────────────────────────────────────────────
// Integer literals
// ──────────────────────────────────────────────────────────
TEST_CASE("Lexer handles decimal integers", "[lexer][numbers]") {
    SECTION("simple") {
        LexerFixture f("42");
        CHECK_TOK(f.next(), NUMBER, "42", Token::NUM_INT);
    }
    SECTION("zero") {
        LexerFixture f("0");
        CHECK_TOK(f.next(), NUMBER, "0", Token::NUM_INT);
    }
    SECTION("with underscores") {
        LexerFixture f("1_000_000");
        CHECK_TOK(f.next(), NUMBER, "1000000", Token::NUM_INT);
    }
}

TEST_CASE("Lexer handles binary integers", "[lexer][numbers]") {
    LexerFixture f("0b1010");
    CHECK_TOK(f.next(), NUMBER, "0b1010", Token::NUM_INT);
    CHECK(f.next().type == NONE);
}

TEST_CASE("Lexer handles octal integers", "[lexer][numbers]") {
    LexerFixture f("0o77");
    CHECK_TOK(f.next(), NUMBER, "0o77", Token::NUM_INT);
    CHECK(f.next().type == NONE);
}

TEST_CASE("Lexer handles hexadecimal integers", "[lexer][numbers]") {
    SECTION("uppercase") {
        LexerFixture f("0xFF");
        CHECK_TOK(f.next(), NUMBER, "0xFF", Token::NUM_INT);
    }
    SECTION("lowercase") {
        LexerFixture f("0xdeadbeef");
        CHECK_TOK(f.next(), NUMBER, "0xdeadbeef", Token::NUM_INT);
    }
    SECTION("with underscores") {
        LexerFixture f("0xDEAD_BEEF");
        CHECK_TOK(f.next(), NUMBER, "0xDEADBEEF", Token::NUM_INT);
    }
}

TEST_CASE("Lexer handles binary with underscores", "[lexer][numbers]") {
    LexerFixture f("0b1001_0110");
    CHECK_TOK(f.next(), NUMBER, "0b10010110", Token::NUM_INT);
    CHECK(f.next().type == NONE);
}

TEST_CASE("Lexer handles octal with underscores", "[lexer][numbers]") {
    LexerFixture f("0o777_000");
    CHECK_TOK(f.next(), NUMBER, "0o777000", Token::NUM_INT);
    CHECK(f.next().type == NONE);
}

// ──────────────────────────────────────────────────────────
// Float literals
// ──────────────────────────────────────────────────────────
TEST_CASE("Lexer handles float literals", "[lexer][numbers]") {
    SECTION("simple") {
        LexerFixture f("1.5");
        CHECK_TOK(f.next(), NUMBER, "1.5", Token::NUM_FLOAT);
    }
    SECTION("leading dot") {
        LexerFixture f(".5");
        CHECK_TOK(f.next(), NUMBER, "0.5", Token::NUM_FLOAT);
    }
    SECTION("underscore before dot") {
        LexerFixture f("1_000.5");
        CHECK_TOK(f.next(), NUMBER, "1000.5", Token::NUM_FLOAT);
    }
    SECTION("underscore after dot") {
        LexerFixture f("1.000_5");
        CHECK_TOK(f.next(), NUMBER, "1.0005", Token::NUM_FLOAT);
    }
}

TEST_CASE("dot after identifier is member access, not float", "[lexer][numbers]") {
    LexerFixture f("x.5");
    CHECK_TOK(f.next(), IDENT,  "x", Token::IDENT);
    CHECK_TOK(f.next(), OP,     ".", Token::OP_DOT);
    CHECK_TOK(f.next(), NUMBER, "5", Token::NUM_INT);
    CHECK(f.next().type == NONE);
}

TEST_CASE("number adjacent to identifier is separate tokens", "[lexer]") {
    LexerFixture f("42foo");
    CHECK_TOK(f.next(), NUMBER, "42", Token::NUM_INT);
    CHECK_TOK(f.next(), IDENT,  "foo", Token::IDENT);
    CHECK(f.next().type == NONE);
}

// ──────────────────────────────────────────────────────────
// String literals
// ──────────────────────────────────────────────────────────
TEST_CASE("Lexer handles basic string literals", "[lexer][strings]") {
    SECTION("double-quoted empty") {
        LexerFixture f("\"\"");
        CHECK_TOK(f.next(), STRING, "", Token::STRING);
    }
    SECTION("single-quoted empty") {
        LexerFixture f("''");
        CHECK_TOK(f.next(), STRING, "", Token::STRING);
    }
    SECTION("double-quoted basic") {
        LexerFixture f("\"hello\"");
        CHECK_TOK(f.next(), STRING, "hello", Token::STRING);
    }
    SECTION("single-quoted basic") {
        LexerFixture f("'world'");
        CHECK_TOK(f.next(), STRING, "world", Token::STRING);
    }
}

TEST_CASE("Lexer handles string escape sequences", "[lexer][strings]") {
    SECTION("newline") {
        LexerFixture f("\"a\\nb\"");
        auto tok = f.next();
        CHECK(tok.type == STRING);
        CHECK(tok.value == "a\nb");
    }
    SECTION("tab") {
        LexerFixture f("\"a\\tb\"");
        auto tok = f.next();
        CHECK(tok.type == STRING);
        CHECK(tok.value == "a\tb");
    }
    SECTION("backslash") {
        LexerFixture f("\"a\\\\b\"");
        auto tok = f.next();
        CHECK(tok.type == STRING);
        CHECK(tok.value == "a\\b");
    }
    SECTION("double quote") {
        LexerFixture f("\"a\\\"b\"");
        auto tok = f.next();
        CHECK(tok.type == STRING);
        CHECK(tok.value == "a\"b");
    }
    SECTION("single quote") {
        LexerFixture f("'a\\'b'");
        auto tok = f.next();
        CHECK(tok.type == STRING);
        CHECK(tok.value == "a'b");
    }
    SECTION("null") {
        LexerFixture f("\"a\\0b\"");
        auto tok = f.next();
        CHECK(tok.type == STRING);
        CHECK(tok.value == std::string("a\0b", 3));
    }
    SECTION("carriage return") {
        LexerFixture f("\"a\\rb\"");
        auto tok = f.next();
        CHECK(tok.type == STRING);
        CHECK(tok.value == "a\rb");
    }
    SECTION("form feed") {
        LexerFixture f("\"a\\fb\"");
        auto tok = f.next();
        CHECK(tok.type == STRING);
        CHECK(tok.value == "a\fb");
    }
    SECTION("backspace") {
        LexerFixture f("\"a\\bb\"");
        auto tok = f.next();
        CHECK(tok.type == STRING);
        CHECK(tok.value == "a\bb");
    }
}

// ──────────────────────────────────────────────────────────
// peek behavior
// ──────────────────────────────────────────────────────────
TEST_CASE("peek returns same token as next without consuming", "[lexer][peek]") {
    LexerFixture f("let x = 42");
    // First peek should match the first next()
    auto p1 = f.lex->peek();
    auto n1 = f.next();
    CHECK(p1.type == n1.type);
    CHECK(p1.value == n1.value);
    CHECK(p1.tokenid == n1.tokenid);
}

TEST_CASE("peek at eof returns NONE", "[lexer][peek]") {
    LexerFixture f("");
    auto p = f.lex->peek();
    CHECK(p.type == NONE);
}

TEST_CASE("peek then advance gives correct stream", "[lexer][peek]") {
    LexerFixture f("x y");
    auto p = f.lex->peek();
    CHECK(p.value == "x");
    auto n1 = f.next();
    CHECK(n1.value == "x");
    auto n2 = f.next();
    CHECK(n2.value == "y");
}

// ──────────────────────────────────────────────────────────
// Edge cases
// ──────────────────────────────────────────────────────────
TEST_CASE("/= is div-assign, not comment start", "[lexer][edge]") {
    LexerFixture f("/=");
    CHECK_TOK(f.next(), OP, "/=", Token::OP_DIV_ASSIGN);
    CHECK(f.next().type == NONE);
}

TEST_CASE("multiple tokens without spaces", "[lexer][edge]") {
    LexerFixture f("let x=42;");
    CHECK_TOK(f.next(), KEYWORD, "let", Token::KW_LET);
    CHECK_TOK(f.next(), IDENT,   "x",   Token::IDENT);
    CHECK_TOK(f.next(), OP,      "=",   Token::OP_ASSIGN);
    CHECK_TOK(f.next(), NUMBER,  "42",  Token::NUM_INT);
    CHECK_TOK(f.next(), PUNC,    ";",   Token::PUNC_SEMI);
    CHECK(f.next().type == NONE);
}

TEST_CASE("eof returns NONE consistently", "[lexer][edge]") {
    LexerFixture f("a");
    CHECK(f.next().type != NONE);
    CHECK(f.next().type == NONE);
    CHECK(f.next().type == NONE);
}

// ──────────────────────────────────────────────────────────
// Integration: small code snippets
// ──────────────────────────────────────────────────────────
TEST_CASE("tokenize simple let binding", "[lexer][integration]") {
    LexerFixture f("let x: int = 42;");
    CHECK_TOK(f.next(), KEYWORD, "let", Token::KW_LET);
    CHECK_TOK(f.next(), IDENT,   "x",   Token::IDENT);
    CHECK_TOK(f.next(), OP,      ":",   Token::PUNC_COLON);
    CHECK_TOK(f.next(), IDENT,   "int", Token::IDENT);
    CHECK_TOK(f.next(), OP,      "=",   Token::OP_ASSIGN);
    CHECK_TOK(f.next(), NUMBER,  "42",  Token::NUM_INT);
    CHECK_TOK(f.next(), PUNC,    ";",   Token::PUNC_SEMI);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize function declaration", "[lexer][integration]") {
    LexerFixture f("fn add(a: int, b: int) -> int { return a + b; }");
    CHECK_TOK(f.next(), KEYWORD, "fn",     Token::KW_FN);
    CHECK_TOK(f.next(), IDENT,   "add",    Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    "(",      Token::PUNC_LPAREN);
    CHECK_TOK(f.next(), IDENT,   "a",      Token::IDENT);
    CHECK_TOK(f.next(), OP,      ":",      Token::PUNC_COLON);
    CHECK_TOK(f.next(), IDENT,   "int",    Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    ",",      Token::PUNC_COMMA);
    CHECK_TOK(f.next(), IDENT,   "b",      Token::IDENT);
    CHECK_TOK(f.next(), OP,      ":",      Token::PUNC_COLON);
    CHECK_TOK(f.next(), IDENT,   "int",    Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    ")",      Token::PUNC_RPAREN);
    CHECK_TOK(f.next(), OP,      "-",      Token::OP_MINUS);
    CHECK_TOK(f.next(), OP,      ">",      Token::OP_GT);
    CHECK_TOK(f.next(), IDENT,   "int",    Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    "{",      Token::PUNC_LBRACE);
    CHECK_TOK(f.next(), KEYWORD, "return", Token::KW_RETURN);
    CHECK_TOK(f.next(), IDENT,   "a",      Token::IDENT);
    CHECK_TOK(f.next(), OP,      "+",      Token::OP_PLUS);
    CHECK_TOK(f.next(), IDENT,   "b",      Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    ";",      Token::PUNC_SEMI);
    CHECK_TOK(f.next(), PUNC,    "}",      Token::PUNC_RBRACE);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize struct declaration", "[lexer][integration]") {
    LexerFixture f("struct Point { x: int, y: int }");
    CHECK_TOK(f.next(), KEYWORD, "struct", Token::KW_STRUCT);
    CHECK_TOK(f.next(), IDENT,   "Point",  Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    "{",      Token::PUNC_LBRACE);
    CHECK_TOK(f.next(), IDENT,   "x",      Token::IDENT);
    CHECK_TOK(f.next(), OP,      ":",      Token::PUNC_COLON);
    CHECK_TOK(f.next(), IDENT,   "int",    Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    ",",      Token::PUNC_COMMA);
    CHECK_TOK(f.next(), IDENT,   "y",      Token::IDENT);
    CHECK_TOK(f.next(), OP,      ":",      Token::PUNC_COLON);
    CHECK_TOK(f.next(), IDENT,   "int",    Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    "}",      Token::PUNC_RBRACE);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize if-elif-else", "[lexer][integration]") {
    LexerFixture f("if x == 1 { true } elif x == 2 { false } else { undefined }");
    CHECK_TOK(f.next(), KEYWORD, "if",        Token::KW_IF);
    CHECK_TOK(f.next(), IDENT,   "x",         Token::IDENT);
    CHECK_TOK(f.next(), OP,      "==",        Token::OP_EQ);
    CHECK_TOK(f.next(), NUMBER,  "1",         Token::NUM_INT);
    CHECK_TOK(f.next(), PUNC,    "{",         Token::PUNC_LBRACE);
    CHECK_TOK(f.next(), KEYWORD, "true",      Token::KW_TRUE);
    CHECK_TOK(f.next(), PUNC,    "}",         Token::PUNC_RBRACE);
    CHECK_TOK(f.next(), KEYWORD, "elif",      Token::KW_ELIF);
    CHECK_TOK(f.next(), IDENT,   "x",         Token::IDENT);
    CHECK_TOK(f.next(), OP,      "==",        Token::OP_EQ);
    CHECK_TOK(f.next(), NUMBER,  "2",         Token::NUM_INT);
    CHECK_TOK(f.next(), PUNC,    "{",         Token::PUNC_LBRACE);
    CHECK_TOK(f.next(), KEYWORD, "false",     Token::KW_FALSE);
    CHECK_TOK(f.next(), PUNC,    "}",         Token::PUNC_RBRACE);
    CHECK_TOK(f.next(), KEYWORD, "else",      Token::KW_ELSE);
    CHECK_TOK(f.next(), PUNC,    "{",         Token::PUNC_LBRACE);
    CHECK_TOK(f.next(), KEYWORD, "undefined", Token::KW_UNDEFINED);
    CHECK_TOK(f.next(), PUNC,    "}",         Token::PUNC_RBRACE);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize range expression with ellipsis", "[lexer][integration]") {
    LexerFixture f("0...10");
    CHECK_TOK(f.next(), NUMBER, "0",   Token::NUM_INT);
    CHECK_TOK(f.next(), OP,     "...", Token::OP_ELLIPSIS);
    CHECK_TOK(f.next(), NUMBER, "10",  Token::NUM_INT);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize for loop with range", "[lexer][integration]") {
    LexerFixture f("for i in 0...10 { break }");
    CHECK_TOK(f.next(), KEYWORD, "for",   Token::KW_FOR);
    CHECK_TOK(f.next(), IDENT,   "i",     Token::IDENT);
    CHECK_TOK(f.next(), IDENT,   "in",    Token::IDENT);
    CHECK_TOK(f.next(), NUMBER,  "0",     Token::NUM_INT);
    CHECK_TOK(f.next(), OP,      "...",   Token::OP_ELLIPSIS);
    CHECK_TOK(f.next(), NUMBER,  "10",    Token::NUM_INT);
    CHECK_TOK(f.next(), PUNC,    "{",     Token::PUNC_LBRACE);
    CHECK_TOK(f.next(), KEYWORD, "break", Token::KW_BREAK);
    CHECK_TOK(f.next(), PUNC,    "}",     Token::PUNC_RBRACE);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize scope resolution and method call", "[lexer][integration]") {
    LexerFixture f("Foo::bar()");
    CHECK_TOK(f.next(), IDENT, "Foo", Token::IDENT);
    CHECK_TOK(f.next(), OP,    "::",  Token::OP_SCOPE_RES);
    CHECK_TOK(f.next(), IDENT, "bar", Token::IDENT);
    CHECK_TOK(f.next(), PUNC,  "(",   Token::PUNC_LPAREN);
    CHECK_TOK(f.next(), PUNC,  ")",   Token::PUNC_RPAREN);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize attribute syntax", "[lexer][integration]") {
    LexerFixture f("#[inline]");
    CHECK_TOK(f.next(), PUNC,    "#",      Token::PUNC_HASH);
    CHECK_TOK(f.next(), PUNC,    "[",      Token::PUNC_LBRACKET);
    CHECK_TOK(f.next(), IDENT,   "inline", Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    "]",      Token::PUNC_RBRACKET);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize extern function declaration", "[lexer][integration]") {
    LexerFixture f("extern fn puts(s: &str) -> int;");
    CHECK_TOK(f.next(), KEYWORD, "extern", Token::KW_EXTERN);
    CHECK_TOK(f.next(), KEYWORD, "fn",     Token::KW_FN);
    CHECK_TOK(f.next(), IDENT,   "puts",   Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    "(",      Token::PUNC_LPAREN);
    CHECK_TOK(f.next(), IDENT,   "s",      Token::IDENT);
    CHECK_TOK(f.next(), OP,      ":",      Token::PUNC_COLON);
    CHECK_TOK(f.next(), OP,      "&",      Token::OP_BITWISE_AND);
    CHECK_TOK(f.next(), IDENT,   "str",    Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    ")",      Token::PUNC_RPAREN);
    CHECK_TOK(f.next(), OP,      "-",      Token::OP_MINUS);
    CHECK_TOK(f.next(), OP,      ">",      Token::OP_GT);
    CHECK_TOK(f.next(), IDENT,   "int",    Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    ";",      Token::PUNC_SEMI);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize comptime block", "[lexer][integration]") {
    LexerFixture f("const x = comptime { compute() };");
    CHECK_TOK(f.next(), KEYWORD, "const",    Token::KW_CONST);
    CHECK_TOK(f.next(), IDENT,   "x",        Token::IDENT);
    CHECK_TOK(f.next(), OP,      "=",        Token::OP_ASSIGN);
    CHECK_TOK(f.next(), KEYWORD, "comptime", Token::KW_COMPTIME);
    CHECK_TOK(f.next(), PUNC,    "{",        Token::PUNC_LBRACE);
    CHECK_TOK(f.next(), IDENT,   "compute",  Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    "(",        Token::PUNC_LPAREN);
    CHECK_TOK(f.next(), PUNC,    ")",        Token::PUNC_RPAREN);
    CHECK_TOK(f.next(), PUNC,    "}",        Token::PUNC_RBRACE);
    CHECK_TOK(f.next(), PUNC,    ";",        Token::PUNC_SEMI);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize pointer and reference types", "[lexer][integration]") {
    LexerFixture f("let p: *int = &x;");
    CHECK_TOK(f.next(), KEYWORD, "let", Token::KW_LET);
    CHECK_TOK(f.next(), IDENT,   "p",   Token::IDENT);
    CHECK_TOK(f.next(), OP,      ":",   Token::PUNC_COLON);
    CHECK_TOK(f.next(), OP,      "*",   Token::OP_MUL);
    CHECK_TOK(f.next(), IDENT,   "int", Token::IDENT);
    CHECK_TOK(f.next(), OP,      "=",   Token::OP_ASSIGN);
    CHECK_TOK(f.next(), OP,      "&",   Token::OP_BITWISE_AND);
    CHECK_TOK(f.next(), IDENT,   "x",   Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    ";",   Token::PUNC_SEMI);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize comparison chain", "[lexer][integration]") {
    LexerFixture f("a <= b && c >= d");
    CHECK_TOK(f.next(), IDENT, "a",  Token::IDENT);
    CHECK_TOK(f.next(), OP,    "<=", Token::OP_LT_EQ);
    CHECK_TOK(f.next(), IDENT, "b",  Token::IDENT);
    CHECK_TOK(f.next(), OP,    "&&", Token::OP_LOGICAL_AND);
    CHECK_TOK(f.next(), IDENT, "c",  Token::IDENT);
    CHECK_TOK(f.next(), OP,    ">=", Token::OP_GT_EQ);
    CHECK_TOK(f.next(), IDENT, "d",  Token::IDENT);
    CHECK(f.next().type == NONE);
}

TEST_CASE("tokenize module import", "[lexer][integration]") {
    LexerFixture f("import std.io;");
    CHECK_TOK(f.next(), KEYWORD, "import", Token::KW_IMPORT);
    CHECK_TOK(f.next(), IDENT,   "std",    Token::IDENT);
    CHECK_TOK(f.next(), OP,      ".",     Token::OP_DOT);
    CHECK_TOK(f.next(), IDENT,   "io",     Token::IDENT);
    CHECK_TOK(f.next(), PUNC,    ";",      Token::PUNC_SEMI);
    CHECK(f.next().type == NONE);
}
