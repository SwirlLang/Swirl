#include <string>
#include <definitions.h>

#include "lexer/TokenStream.h"
#include "lexer/Tokens.h"

using namespace std::string_view_literals;

bool TokenStream::isKeyword(const std::string& _str) {
    return KeywordMap.contains(_str);
}

bool TokenStream::isDigit(const char chr) {
    return ('0' <= chr && chr <= '9') || chr == '_';
}

bool TokenStream::isHexDigit(const char chr) {
    return ('0' <= chr && chr <= '9') || ('A' <= chr && chr <= 'F') || ('a' <= chr && chr <= 'f') || chr == '_';
}

bool TokenStream::isOctalDigit(const char chr) {
    return ('0' <= chr && chr <= '7') || chr == '_';
}

bool TokenStream::isBinaryDigit(const char chr) {
    return chr == '0' || chr == '1' || chr == '_';
}

bool TokenStream::isIdStart(const char chr) {
    return ('A' <= chr && chr <= 'Z') || ('a' <= chr && chr <= 'z') || chr == '_';
}

bool TokenStream::isId(const char chr) {
    return isIdStart(chr) || isDigit(chr);
}

static constexpr auto PuncTable = [] {
    std::array<Token::TokenValue, 256> table{};
    table['{'] = Token::PUNC_LBRACE;
    table['}'] = Token::PUNC_RBRACE;
    table['('] = Token::PUNC_LPAREN;
    table[')'] = Token::PUNC_RPAREN;
    table['['] = Token::PUNC_LBRACKET;
    table[']'] = Token::PUNC_RBRACKET;
    table[','] = Token::PUNC_COMMA;
    table[';'] = Token::PUNC_SEMI;
    table['#'] = Token::PUNC_HASH;
    return table;
}();

static constexpr auto OpCharTable = [] {
    std::array<bool, 256> table{};
    table['='] = true;
    table['+'] = true;
    table['-'] = true;
    table['*'] = true;
    table['/'] = true;
    table['%'] = true;
    table[':'] = true;
    table['.'] = true;
    table['|'] = true;
    table['&'] = true;
    table['!'] = true;
    table['>'] = true;
    table['<'] = true;
    table['^'] = true;
    table['~'] = true;
    return table;
}();

bool TokenStream::isOpChar(const char _chr) {
    return OpCharTable[static_cast<unsigned char>(_chr)];
}

std::string TokenStream::readEscaped(const char _end) const {
    bool is_escaped = false;
    std::string ret;

    while (!m_Stream.eof()) {
        const char chr = m_Stream.next();
        if (is_escaped) {
            switch (chr) {
                case 'n':
                    ret += '\n';
                    break;
                case 't':
                    ret += '\t';
                    break;
                case 'r':
                    ret += '\r';
                    break;
                case 'b':
                    ret += '\b';
                    break;
                case 'f':
                    ret += '\f';
                    break;
                case '\\':
                    ret += '\\';
                    break;
                case '\"':
                    ret += '\"';
                    break;
                case '\'':
                    ret += '\'';
                    break;
                case '0':
                    ret += '\0';
                    break;
                default:
                    ret += '\\';
                    ret += chr;
                    break;
            }
            is_escaped = false;
        } else if (chr == '\\')
            is_escaped = true;
        else if (chr == _end)
            break;
        else
            ret += chr;
    }

    return ret;
}

Token TokenStream::readString(const char del) const {
    auto str = readEscaped(del);
    // if (!m_Stream.eof()) m_Stream.next();  // escape the '"'
    return {STRING, std::move(str), getStreamState(), Token::STRING};
}

Token TokenStream::readOperator() const {
    const char curr = m_Stream.getCurrentChar();
    const char next = m_Stream.eof() ? '\0' : m_Stream.peek();

    switch (curr) {
        case '=':
            if (next == '=') {
                m_Stream.next();
                return {OP, "==", getStreamState(), Token::OP_EQ};
            }
            return {OP, "=", getStreamState(), Token::OP_ASSIGN};

        case '+':
            if (next == '=') {
                m_Stream.next();
                return {OP, "+=", getStreamState(), Token::OP_PLUS_ASSIGN};
            }
            return {OP, "+", getStreamState(), Token::OP_PLUS};

        case '-':
            if (next == '=') {
                m_Stream.next();
                return {OP, "-=", getStreamState(), Token::OP_MINUS_ASSIGN};
            }
            return {OP, "-", getStreamState(), Token::OP_MINUS};

        case '*':
            if (next == '*') {
                m_Stream.next();
                if (m_Stream.peek() == '=') {
                    m_Stream.next();
                    return {OP, "**=", getStreamState(), Token::OP_EXP_ASSIGN};
                }
                return {OP, "**", getStreamState(), Token::OP_EXP};
            }
            if (next == '=') {
                m_Stream.next();
                return {OP, "*=", getStreamState(), Token::OP_MUL_ASSIGN};
            }
            return {OP, "*", getStreamState(), Token::OP_MUL};

        case '/':
            if (next == '=') {
                m_Stream.next();
                return {OP, "/=", getStreamState(), Token::OP_DIV_ASSIGN};
            }
            if (next == '/') {
                if (!m_Stream.eof())
                    m_Stream.next();
                return {OP, "//", getStreamState(), Token::OP_COMMENT};
            }
            return {OP, "/", getStreamState(), Token::OP_DIV};

        case '%':
            if (next == '=') {
                m_Stream.next();
                return {OP, "%=", getStreamState(), Token::OP_MOD_ASSIGN};
            }
            return {OP, "%", getStreamState(), Token::OP_MOD};

        case ':':
            if (next == ':') {
                m_Stream.next();
                return {OP, "::", getStreamState(), Token::OP_SCOPE_RES};
            }
            return {OP, ":", getStreamState(), Token::PUNC_COLON};

        case '|':
            if (next == '|') {
                m_Stream.next();
                return {OP, "||", getStreamState(), Token::OP_LOGICAL_OR};
            }

            if (m_Stream.peek() == '=') {
                m_Stream.next();
                return {OP, "|=", getStreamState(), Token::OP_BITWISE_OR_ASSIGN};
            }

            return {OP, "|", getStreamState(), Token::OP_BITWISE_OR};

        case '&':
            if (next == '&') {
                m_Stream.next();
                return {OP, "&&", getStreamState(), Token::OP_LOGICAL_AND};
            }

            if (m_Stream.peek() == '=') {
                m_Stream.next();
                return {OP, "&=", getStreamState(), Token::OP_BITWISE_AND};
            }

            return {OP, "&", getStreamState(), Token::OP_BITWISE_AND};

        case '!':
            if (next == '=') {
                m_Stream.next();
                return {OP, "!=", getStreamState(), Token::OP_NOT_EQ};
            }
            return {OP, "!", getStreamState(), Token::OP_NOT};

        case '>':
            if (next == '=') {
                m_Stream.next();
                return {OP, ">=", getStreamState(), Token::OP_GT_EQ};
            }

            // >>
            if (next == '>') {
                m_Stream.next();

                // >>=
                if (m_Stream.peek() == '=') {
                    m_Stream.next();
                    return {OP, ">>=", getStreamState(), Token::OP_RBITSHIFT_ASSIGN};
                }

                return {OP, ">>", getStreamState(), Token::OP_RBITSHIFT};
            }

            return {OP, ">", getStreamState(), Token::OP_GT};

        case '<':
            if (next == '=') {
                m_Stream.next();
                return {OP, "<=", getStreamState(), Token::OP_LT_EQ};
            }

            // <<
            if (next == '<') {
                m_Stream.next();

                // <<=
                if (m_Stream.peek() == '=') {
                    m_Stream.next();
                    return {OP, "<<=", getStreamState(), Token::OP_LBITSHIFT_ASSIGN};
                }

                return {OP, "<<", getStreamState(), Token::OP_LBITSHIFT};
            }

            return {OP, "<", getStreamState(), Token::OP_LT};

        case '^':
            if (next == '=') {
                m_Stream.next();
                return {OP, "^=", getStreamState(), Token::OP_XOR_ASSIGN};
            } return {OP, "^", getStreamState(), Token::OP_XOR};
        case '~':
            return {OP, "~", getStreamState(), Token::OP_BITWISE_NOT};
        case '.':
            return {OP, ".", getStreamState(), Token::OP_DOT};

        default:
            // TODO: use swirl error manager instead of throwing rawdog exceptions
            throw std::runtime_error(std::format("Invalid operator character: {}", curr));
    }
}

Token TokenStream::readNextTok() {
    if (m_Stream.eof())
        return {NONE, "TOKEN:EOF", getStreamState()};

    switch (const char ch = m_Stream.next()) {
    case '"':
        return readString('"');
    case '\'':
        return readString('\'');
    default:
        if (isOpChar(ch)) {
            Token op = readOperator();
            if (m_Stream.eof()) [[unlikely]]
                return op;
            if (op.value == ".") {
                const char next_char = m_Stream.peek();
                if (!m_isPreviousTokIdent && isDigit(next_char) && next_char != '_')
                    return {NUMBER, "0" + readWhile(isDigit, [](const char c) { return c == '_'; }), getStreamState(),
                            Token::NUM_FLOAT};

                if (next_char == '.' && !m_Stream.almostEOF() && m_Stream.peekDeeper() == '.') {
                    m_Stream.next();
                    m_Stream.next();
                    m_isPreviousTokIdent = false;
                    return {OP, "...", getStreamState(), Token::OP_ELLIPSIS};
                }
            }
            m_isPreviousTokIdent = false;
            if (op.value == "//") {
                return {COMMENT, readWhile([](const char c) { return c != '\n'; }), getStreamState(),
                        Token::OP_COMMENT};
            }
            return op;
        }

        m_isPreviousTokIdent = false;
        if (isIdStart(ch)) {
            auto val = readWhile(isId);
            if (KeywordMap.contains(val)) {
                Token::TokenValue kw_val = KeywordMap.at(val);
                return {KEYWORD, std::move(val), getStreamState(), kw_val};
            } if (val == "as")
                return {OP, std::move(val), getStreamState(), Token::OP_AS};
            m_isPreviousTokIdent = true;
            return {IDENT, std::move(val), getStreamState(), Token::IDENT};
        }

        if (isDigit(ch)) {
            if (m_Stream.eof())
                return {NUMBER, std::string(1, ch), getStreamState(), Token::NUM_INT};
            if (ch == '0') {
                switch (m_Stream.peek()) {
                    case 'b':
                        m_Stream.next();
                        return {NUMBER, "0" + readWhile(isBinaryDigit, [](const char c) { return c == '_'; }),
                                getStreamState(), Token::NUM_INT};
                    case 'o':
                        m_Stream.next();
                        return {NUMBER, "0" + readWhile(isOctalDigit, [](const char c) { return c == '_'; }),
                                getStreamState(), Token::NUM_INT};
                    case 'x':
                        m_Stream.next();
                        return {NUMBER, "0" + readWhile(isHexDigit, [](const char c) { return c == '_'; }),
                                getStreamState(), Token::NUM_INT};
                }
            }
            auto val = readWhile(isDigit, [](char c) { return c == '_'; });
            if (m_Stream.eof())
                return {NUMBER, std::move(val), getStreamState(), Token::NUM_INT};
            if (m_Stream.peek() == '.') {
                if (m_Stream.almostEOF())
                    return {NUMBER, std::move(val), getStreamState(), Token::NUM_INT};
                if (isDigit(m_Stream.peekDeeper()) && m_Stream.peekDeeper() != '_') {
                    m_Stream.next();
                    return {NUMBER, val + readWhile(isDigit, [](const char c) { return c == '_'; }), getStreamState(),
                            Token::NUM_FLOAT};
                }
            }
            return {NUMBER, std::move(val), getStreamState(), Token::NUM_INT};
        }

        return {PUNC, std::string(1, ch), getStreamState(), PuncTable.at(ch)};
    }
}

TokenStream::TokenStream(SourceManager& src_man) : m_Stream{src_man} {}

void TokenStream::setReturnPoint() {
    m_Cache = m_Stream.getStreamState();
}

void TokenStream::restoreCache() const {
    m_Stream.setStreamState(m_Cache);
}

void TokenStream::expectTypes(std::initializer_list<TokenCategory>&& types) {
    m_Filter.is_active = true;
    m_Filter.only_type = true;
    m_Filter.expected_types.assign(types.begin(), types.end());
}

void TokenStream::expectTokens(std::initializer_list<Token>&& tokens) {
    m_Filter.is_active = true;
    m_Filter.only_type = false;
    m_Filter.expected_tokens.assign(tokens.begin(), tokens.end());
}

Token TokenStream::next(const bool modify_cur_tk) {
    Token cur_tk;
    unsigned char c{};

    // Discard junk tokens
    do {
        cur_tk = readNextTok();
        if (!cur_tk.value.empty()) {
            c = static_cast<unsigned char>(cur_tk.value[0]);
        }
    } while ((cur_tk.type == PUNC && (c <= ' ' || c == 0x7F)) || cur_tk.type == COMMENT);

    if (modify_cur_tk)
        CurTok = cur_tk;
    return cur_tk;
}

Token TokenStream::peek() {
    setReturnPoint();

    if (m_Stream.eof()) {
        restoreCache();
        return {NONE, "TOKEN:EOF", getStreamState()};
    }

    PeekTok = next(false);
    restoreCache();
    return PeekTok;
}

StreamState TokenStream::getStreamState() const {
    return m_Stream.getStreamState();
}

bool TokenStream::eof() const {
    return CurTok.type == NONE;
}
