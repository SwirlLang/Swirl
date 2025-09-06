#include <algorithm>
#include <string>
#include <definitions.h>
#include <string_view>
#include <lexer/TokenStream.h>

using namespace std::string_view_literals;

bool TokenStream::isKeyword(const std::string& _str) {
    return KeywordSet.contains(_str);
}

bool TokenStream::isDigit(const char chr) {
    return ('0' <= chr && chr <= '9') || chr == '_';
}

bool TokenStream::isHexDigit(const char chr) {
    return ('0' <= chr && chr <= '9') || ('A' <= chr && chr <= 'F') || ('a' <= chr && chr <= 'f') || chr == '_';
}

bool TokenStream::isOctalDigit(const char chr) {
    return ('0' <= chr && chr <= '7') || chr == '_' ; 
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

bool TokenStream::isOpChar(const char _chr) {
    return "+-*/%><=&|.:!"sv.find(_chr) != std::string::npos;
}

std::string TokenStream::readEscaped(const char _end) const {
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

Token TokenStream::readString(const char del) const {
    auto str = readEscaped(del);
    if (!m_Stream.eof()) m_Stream.next();  // escape the '"'
    return {STRING, std::move(str), getStreamState()};
}

Token TokenStream::readOperator() const {
    Token tok = {OP, std::string(1, m_Stream.getCurrentChar()), getStreamState()};
    if (m_Stream.eof()) return tok;
    if (const auto pot_op = std::string(1, m_Stream.getCurrentChar()) + m_Stream.peek(); OperatorSet.contains(pot_op)) {
        m_Stream.next();

        // discard the next operator if it's a comment not at EOF
        if (pot_op == "//") {
            if (m_Stream.eof()) [[unlikely]] return {NONE, "TOKEN:EOF", getStreamState()};
            m_Stream.next();
        }

        return {OP, pot_op, getStreamState()};
    }

    return tok;
}

Token TokenStream::readNextTok() {
    if (m_Stream.eof())
        return {NONE, "TOKEN:EOF", getStreamState()};

    switch (const char ch = m_Stream.next()) {
        case '"': return readString('"');
        case '\'': return readString('\'');
        default:
            if (isOpChar(ch)) {
                Token op = readOperator();
                if (m_Stream.eof()) [[unlikely]] return op;
                if (op.value == ".") {
                    const char next_char = m_Stream.peek();
                    if (!m_isPreviousTokIdent && isDigit(next_char) && next_char != '_')
                        return {NUMBER, "0" + readWhile(isDigit, [](char c) {return c == '_';}), getStreamState(), CT_FLOAT};

                    if (next_char == '.' && !m_Stream.almostEOF() && m_Stream.peekDeeper() == '.') {
                        m_Stream.next(); m_Stream.next();
                        m_isPreviousTokIdent = false;
                        return {OP, "...", getStreamState()};
                    }
                } 
                m_isPreviousTokIdent = false;
                if (op.value == "//") {
                    return {COMMENT, readWhile([](const char c) { return c != '\n'; }), getStreamState()};
                }
                return op;
            }

            m_isPreviousTokIdent = false;
            if (isIdStart(ch)) {
                auto val = readWhile(isId);
                if (KeywordSet.contains(val)) return {KEYWORD, std::move(val), getStreamState()};
                else if (val == "as")         return {OP,      std::move(val), getStreamState()};
                m_isPreviousTokIdent = true;  return {IDENT,   std::move(val), getStreamState()};
            }

            if (isDigit(ch)) {
                if (m_Stream.eof()) return {NUMBER, std::string(1, ch), getStreamState(), CT_INT};
                if (ch == '0') {
                    switch (m_Stream.peek()) {
                        case 'b':
                            m_Stream.next();
                            return {NUMBER, "0" + readWhile(isBinaryDigit, [](char c) {return c == '_';}), getStreamState(), CT_INT};
                        case 'o':
                            m_Stream.next();
                            return {NUMBER, "0" + readWhile(isOctalDigit,  [](char c) {return c == '_';}), getStreamState(), CT_INT};
                        case 'x':
                            m_Stream.next();
                            return {NUMBER, "0" + readWhile(isHexDigit,    [](char c) {return c == '_';}), getStreamState(), CT_INT};
                        default:
                            // TODO: report a syntax error
                            throw std::runtime_error("Invalid character following the `0`.");
                    }
                }
                auto val = readWhile(isDigit, [](char c) {return c == '_';});
                if (m_Stream.eof()) return {NUMBER, std::move(val), getStreamState(), CT_INT};
                if (m_Stream.peek() == '.') {
                    if (m_Stream.almostEOF()) return {NUMBER, std::move(val), getStreamState(), CT_INT};
                    if (isDigit(m_Stream.peekDeeper()) && m_Stream.peekDeeper() != '_') {
                        m_Stream.next();
                        return {NUMBER, val + readWhile(isDigit, [](char c) {return c == '_';}), getStreamState(), CT_FLOAT};
                    }
                }
                return {NUMBER, std::move(val), getStreamState(), CT_INT};
            }

            return {PUNC, std::string(1, ch), getStreamState()};
    }
}

TokenStream::TokenStream(SourceManager& src_man) : m_Stream{src_man} {}

void TokenStream::setReturnPoint() {
    m_Cache.Line = m_Stream.Line;
    m_Cache.Pos  = m_Stream.Pos;
    m_Cache.Col  = m_Stream.Col;
}

void TokenStream::restoreCache() const {
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
    Token cur_tk;
    unsigned char c;

    // Discard junk tokens
    do {
        cur_tk = readNextTok();
        c = static_cast<unsigned char>(cur_tk.value.at(0));
    } while ((cur_tk.type == PUNC && (c <= ' ' || c == 0x7F)) || cur_tk.type == COMMENT);

    if (modify_cur_tk) CurTok = cur_tk;
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
    return {m_Stream.Line, m_Stream.Pos, m_Stream.Col};
}

bool TokenStream::eof() const {
    return CurTok.type == NONE;
}
