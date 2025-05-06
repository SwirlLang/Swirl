#include <algorithm>
#include <string>
#include <definitions.h>
#include <string_view>
#include <managers/ErrorManager.h>
#include <lexer/TokenStream.h>

using namespace std::string_view_literals;

bool TokenStream::isKeyword(const std::string& _str) {
    return keywords.contains(_str);
}

bool TokenStream::isDigit(const char chr) {
    return ('0' <= chr && chr <= '9');
}

bool TokenStream::isNumeric(const char chr) {
    return isDigit(chr) || chr == 'x' || chr == '_' || chr == '.' || chr == 'o';
}

bool TokenStream::isIdStart(const char chr) {
    return ('A' <= chr && chr <= 'Z') || ('a' <= chr && chr <= 'z') || chr == '_';
}

bool TokenStream::isId(const char chr) {
    return isIdStart(chr) || isDigit(chr);
}

bool TokenStream::isOpChar(const char _chr) {
    return "+-/*><=&@|.:!"sv.find(_chr) != std::string::npos;
}

bool TokenStream::isWhiteSpace(const char _chr) {
    return " \t\n"sv.find(_chr) != std::string::npos;
}

std::string TokenStream::readEscaped(const char _end) {
    bool is_escaped = false;
    std::string ret;

    while (!m_Stream.eof()) {
        const char chr = m_Stream.next();
        if (is_escaped) {
            switch (chr) {
                case 'n': ret += '\n'; break;
                case 't': ret += '\t'; break;
                case 'r': ret += '\r'; break;
                case 'b': ret += '\b'; break;
                case 'f': ret += '\f'; break;
                case '\\': ret += '\\'; break;
                case '\"': ret += '\"'; break;
                case '\'': ret += '\''; break;
                case '0': ret += '\0'; break;
                default: ret += '\\'; ret += chr; break;
            }
            is_escaped = false;
        } else if (chr == '\\') is_escaped = true;
        else if (chr == _end) break;
        else ret += chr;
    }

    return ret;
}

Token TokenStream::readString(const char del) {
    auto str = readEscaped(del);
    m_Stream.next();  // escape the '"'
    return {STRING, std::move(str), getStreamState()};
}

std::string TokenStream::readWhile(const std::function<bool(char)>& pred) {
    std::string ret(1, m_Stream.getCurrentChar());

    while (pred(m_Stream.peek()))
        ret += m_Stream.next();
    return ret;
}

Token TokenStream::readOperator() {
    if (const auto pot_op = std::string(1, m_Stream.getCurrentChar()) + m_Stream.peek(); operators.contains(pot_op)) {
        m_Stream.next();
        return {OP, pot_op, getStreamState()};
    }

    Token tok = {OP, std::string(1, m_Stream.getCurrentChar()), getStreamState()};
    return tok;
}

Token TokenStream::readNextTok() {
    if (m_Stream.eof())
        return {NONE, "TOKEN:EOF", m_Stream.Line};

    switch (const char ch = m_Stream.next()) {
        case '"': return readString('"');
        case '\'': return readString('\'');
        default:
            if (isIdStart(ch)) {
                auto val = readWhile(isId);
                return {
                    keywords.contains(val)
                        ? KEYWORD
                        : val == "as" ? OP : IDENT,

                    std::move(val),
                    getStreamState()
                };
            }
            if (isOpChar(ch)) {
                Token op = readOperator();
                if (op.value == "//") {
                    return {COMMENT, readWhile([](const char c) { return c != '\n'; }), getStreamState()};
                }
                return op;
            }
            if (isDigit(ch)) {
                auto val = readWhile(isNumeric);
                return {NUMBER, val, getStreamState(), val.contains('.') ? CT_FLOAT : CT_INT};
            }
            Token punc = {PUNC, std::string(1, ch), getStreamState()};
            if (punc.value == ".") {
                if (isDigit(m_Stream.peek())) {
                    m_Stream.next();
                    return {NUMBER, "0." + readWhile(isNumeric), getStreamState(), CT_FLOAT};
                }
            }
            return punc;
    }
}

TokenStream::TokenStream(const fs::path& file_path) : m_Stream{file_path}, m_Path{file_path} {}

void TokenStream::setReturnPoint() {
    m_Cache.Line = m_Stream.Line;
    m_Cache.Pos  = m_Stream.Pos;
    m_Cache.Col  = m_Stream.Col;
}

void TokenStream::restoreCache() {
    m_Stream.Pos  = m_Cache.Pos;
    m_Stream.Line = m_Cache.Line;
    m_Stream.Col  = m_Cache.Col;
}

void TokenStream::expectTypes(std::initializer_list<TokenType>&& types) {
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
    Token cur_tk = readNextTok();

    // Discard junk tokens
    while ((cur_tk.type == PUNC && (cur_tk.value == " " || cur_tk.value == "\t" || cur_tk.value == "\n")) ||
           cur_tk.type == COMMENT) {
        cur_tk = readNextTok();
    }

    if (modify_cur_tk) CurTok = cur_tk;
    return cur_tk;
}

Token TokenStream::peek() {
    setReturnPoint();

    if (m_Stream.eof()) {
        restoreCache();
        return {NONE, "NULL", m_Stream.Line};
    }

    PeekTok = next(false);
    restoreCache();
    return PeekTok;
}

StreamState TokenStream::getStreamState() const {
    return {m_Stream.Line, m_Stream.Pos, m_Stream.Col};
}

bool TokenStream::eof() const {
    return CurTok.type == NONE;
}
