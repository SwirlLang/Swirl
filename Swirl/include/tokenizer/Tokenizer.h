#pragma once
#include <iostream>
#include <cstdint>
#include <utility>
#include <functional>
#include <string_view>
#include <unordered_map>

#include <tokenizer/InputStream.h>
#include <utils/utils.h>
#include <tokens/Tokens.h>


using namespace std::string_view_literals;

extern const std::unordered_map<std::string, uint8_t> keywords;
extern std::unordered_map<std::string, int> operators;
extern std::unordered_map<std::size_t, std::string> LineTable;

class TokenStream {
    bool                        m_Debug  = false;  // Debug flag
    StreamState                 m_Cache;           // For caching stream state
    InputStream                 m_Stream;          // InputStream instance
    struct {
        Token particular_tok;
        bool  only_type{};
    }                           m_Filter;


    static bool isKeyword(const std::string& _str) {
        return keywords.contains(_str);
    }

    static bool isDigit(const char chr) {
        return ('0' <= chr && chr <= '9');
    }

    static bool isNumeric(const char chr) {
        return isDigit(chr) || chr == 'x' || chr == '_' || chr == '.' || chr == 'o';
    }

    static bool isIdStart(const char chr) {
        return ('A' <= chr && chr <= 'Z') || ('a' <= chr && chr <= 'z') || chr == '_';
    }

    static bool isId(const char chr) {
        return isIdStart(chr) || isDigit(chr);
    }

    static bool isOpChar(const char _chr) {
        return "+-/*><=&@|."sv.find(_chr) != std::string::npos;
    }

    static bool isWhiteSpace(const char _chr) {
        return " \t\n"sv.find(_chr) != std::string::npos;
    }

    /* Used to start reading a stream of characters till the `_end` char is hit. */
    std::string readEscaped(const char _end) {
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
            }

            else if (chr == '\\') is_escaped = true;
            else if (chr == _end) break;
            else ret += chr;
        }

        return ret;
    }

    Token readString(const char del) {
        auto str = readEscaped(del);
        m_Stream.next();  // escape the '"'
        return {
            STRING,
            std::move(str),
            getStreamState()
        };
    }

    // reads until `pred` evaluates to false
    std::string readWhile(const std::function<bool (char ch)>& pred) {
        std::string ret(1, m_Stream.getCurrentChar());

        while (pred(m_Stream.peek()))
            ret += m_Stream.next();
        return ret;
    }

    Token readOperator() {
        if (const auto pot_op = std::string(1, m_Stream.getCurrentChar()) + m_Stream.peek(); operators.contains(pot_op)) {
            m_Stream.next();
            return {
                OP,
                pot_op,
                getStreamState()
            };
        }

        Token tok = {
            OP,
            std::string(1, m_Stream.getCurrentChar()),
            getStreamState()
        };

        return tok;
    }

    Token readNextTok() {
        if (m_Stream.eof())
            return {NONE, "TOKEN:EOF", m_Stream.Line};

        switch (const char ch = m_Stream.next()) {
            case '"':
                return readString('"');
            case '\'':
                return readString('\'');
            default:
                if (isIdStart(ch)) {
                    auto val = readWhile(isId);
                    return {
                        keywords.contains(val) ? KEYWORD : IDENT,
                        std::move(val),
                        getStreamState()
                    };
                }
                if (isOpChar(ch)) {
                    Token op = readOperator();
                    if (op.value == "//") {
                        return {
                            COMMENT,
                            readWhile([](const char c) {
                                return c != '\n';
                            }),
                            getStreamState()
                        };
                    }

                    return op;
                }

                if (isDigit(ch)) {
                    auto val = readWhile(isNumeric);
                    return {
                        NUMBER,
                        std::move(val),
                        getStreamState(),
                        val.find('.') != std::string::npos ? CT_FLOAT : CT_INT
                    };
                }
                Token punc = {
                    PUNC,
                    std::string(1, ch),
                    getStreamState()
                };

                if (punc.value == ".") {
                    if (isDigit(m_Stream.peek())) {
                        m_Stream.next();
                        return {
                            NUMBER,
                            "0." + readWhile(isNumeric),
                            getStreamState(),
                            CT_FLOAT
                        };
                    }
                }
                return punc;
        }
    }

public:
    Token p_CurTk{UNINIT, ""};
    Token p_PeekTk{UNINIT, ""};


    explicit TokenStream(InputStream _stream, const bool _debug = false) : m_Debug(_debug), m_Stream(std::move(_stream)) {}

    void setReturnPoint() {
        m_Cache.Line = m_Stream.Line;
        m_Cache.Pos  = m_Stream.Pos;
        m_Cache.Col  = m_Stream.Col;
    }

    void restoreCache() {
        m_Stream.Pos  = m_Cache.Pos;
        m_Stream.Line = m_Cache.Line;
        m_Stream.Col  = m_Cache.Col;
    }


    /* An abstraction over readNextTok. */
    Token next(const bool _modifyCurTk = true) {
        Token cur_tk = readNextTok();

        // discard all the junk
        while ((cur_tk.type == PUNC &&
            (cur_tk.value == " " || cur_tk.value == "\t" || cur_tk.value == "\n")) || cur_tk.type == COMMENT) {
            cur_tk = readNextTok();
        }

        while ((cur_tk.type == PUNC &&
            (cur_tk.value == " " || cur_tk.value == "\t" || cur_tk.value == "\n")) || cur_tk.type == COMMENT){
            cur_tk = readNextTok();
        }

        if (_modifyCurTk) p_CurTk = cur_tk;
        return cur_tk;
    }


    /* Return the next token from the m_Stream without consuming it. */
    Token peek() {
        setReturnPoint();
        
        if (m_Stream.eof()) {
            restoreCache();
            return {NONE, "NULL", m_Stream.Line};
        }

        p_PeekTk = next(false);
        restoreCache();
        return p_PeekTk;
    }

    StreamState getStreamState() const {
        return StreamState{
            .Line  = m_Stream.Line,
            .Pos   = m_Stream.Pos,
            .Col   = m_Stream.Col,
        };
    }

    bool eof() const {
        return p_CurTk.type == NONE;
    }
};
