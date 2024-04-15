#include <iostream>
#include <map>
#include <array>
#include <cstdint>
#include <vector>
#include <cstring>
#include <functional>
#include <string_view>
#include <unordered_map>

#include <tokenizer/InputStream.h>
#include <utils/utils.h>
#include <tokens/Tokens.h>


#ifndef SWIRL_TokenStream_H
#define SWIRL_TokenStream_H

#define SWIRL_TokenStream_H


using namespace std::string_view_literals;

extern std::unordered_map<std::string, uint8_t> keywords;
extern std::unordered_map<std::string, uint8_t> operators;

class TokenStream {
private:
    struct StreamState {
        std::size_t Line, Pos, Col;
        std::string CurLn;
    };

    int                         m_Pcnt  = 0;       // paren counter
    bool                        m_Debug  = false;  // Debug flag
    bool                        m_rdfs   = false;  // deprecated string interpolation flag
    bool                        m_trackp = false;  // track parentheses flag
    std::string                 m_Ret;             // temporary cache
    std::string                 m_Rax;             // temporary cache
    Token                       m_lastTok{};
    Token                       m_Cur{};
    StreamState                 m_Cache{};
    InputStream                 m_Stream;          // InputStream instance


    static bool isKeyword(const std::string& _str) {
        return keywords.contains(_str);
    }

    static bool isDigit(char _chr) {
        return "1234567890"sv.find(_chr) != std::string::npos;
    }

    static bool isId(char chr) {
        return isIdStart(chr) || "\"?!-=0123456789"sv.find(chr) != std::string::npos;
    }

    static bool isIdStart(char _chr) {
        return "_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"sv.find(_chr) != std::string::npos;
    }

    static bool isPunctuation(char chr) {
        return "();,{}[]"sv.find(chr) >= 0;
    }

    static bool isOpChar(char _chr) {
        return "+-/*><="sv.find(_chr) != std::string::npos;
    }

    static bool isWhiteSpace(char _chr) {
        return " \t\n"sv.find(_chr) != std::string::npos;
    }

    /* Read and return the characters until the given boolean function evaluates to true. */
    std::string readWhile(const std::function<bool (char)>& delimiter) {
        std::string ret;
        while (!m_Stream.eof()) {
            if (delimiter(m_Stream.peek())) {
                ret += m_Stream.next();
            } else { break; }
        }
        return ret;
    }

    /* Used to start reading a m_Stream of characters till the `_end` char is hit. */
    std::string readEscaped(char _end) {
        uint8_t is_escaped = false;
        std::string ret;

        m_Stream.next();
        while (!m_Stream.eof()) {
            char chr = m_Stream.next();
            if (is_escaped) {
                ret += chr;
                is_escaped = false;
            } else if (chr == '\\') {
                ret += '\\';
                is_escaped = true;
            }
            else if (chr == _end)
                break;
            else
                ret += chr;
        }

        return ret;
    }

    Token readString(char del = '"') {
        m_Ret = readEscaped(del);
        m_Ret.pop_back();
        m_Ret.insert(0, "\"");
        m_Ret.append("\"");
        return {STRING, m_Ret, m_Stream.Line};
    }

    Token readIdent() {
        m_Rax = readWhile(isId);
        return {
                isKeyword(m_Rax) ? KEYWORD : IDENT,
                m_Rax,
                m_Stream.Line
        };
    }

    Token readNumber(const char* neg = "") {
        static uint8_t has_decim = false;
        std::string number = readWhile([](char ch) -> bool {
            if (ch == '.') {
                if (has_decim) return false;
                has_decim = true;
                return true;
            } return isDigit(ch);
        });
        has_decim = false;
        m_Ret = number;
        return {NUMBER, neg + m_Ret, m_Stream.Line};
    }

    /* Consume the next token from the m_Stream. */
    Token readNextTok() {
        setReturnPoint();

        if (m_Stream.eof()) {return {NONE, "null", m_Stream.Line};}
        auto chr = m_Stream.peek();
        if (chr == '"') return readString();
        if (chr == '\'') return readString('\'');
        if (isDigit(chr)) return readNumber();
        if (isIdStart(chr)) return readIdent();

        m_Ret = std::string(1, m_Stream.next());

        if (isOpChar(chr)) {
            m_Rax = chr + readWhile(isOpChar);
            return {
                    OP,
                    m_Rax,
                    m_Stream.Line
            };
        }

        if (isPunctuation(chr) ) return {
                    PUNC,
                    m_Ret,
                    m_Stream.Line
            };


        throw std::runtime_error("[FATAL]: No valid token found");
    }


public:
    Token p_CurTk{_NONE, ""};
    Token p_PeekTk{_NONE, ""};


    explicit TokenStream(InputStream& _stream, bool _debug = false) : m_Stream(_stream), m_Debug(_debug) {}

    void setReturnPoint() {
        m_Cache.Line = m_Stream.Line;
        m_Cache.Pos  = m_Stream.Pos;
        m_Cache.Col  = m_Stream.Col;
        m_Cache.CurLn = m_Stream.LineMap[m_Stream.Line];
    }

    void restoreCache() {
        m_Stream.Pos  = m_Cache.Pos;
        m_Stream.Line = m_Cache.Line;
        m_Stream.Col  = m_Cache.Col;
        m_Stream.LineMap[m_Stream.Line] = m_Cache.CurLn;
    }

    /* An abstraction over readNextTok. */
    Token next(bool _readNewLines = false, bool _readWhitespaces = false, bool _modifyCurTk = true) {
        Token cur_tk = readNextTok();

        // discards comments from the stream
        auto discard_comments = [&cur_tk, this]() -> void {
            if (cur_tk.type == OP && cur_tk.value == "//") {
                while (true) {
                    if (cur_tk.type == PUNC && cur_tk.value == "\n" || m_Stream.eof())
                        break;
                    cur_tk = readNextTok();
                }
            }
        };

        discard_comments();

        // discard whitespaces
        if (!_readWhitespaces)
            while ((cur_tk.type == PUNC && cur_tk.value == " ")) {
                cur_tk = readNextTok();
                discard_comments();
            }

        // discard newlines
        if (!_readNewLines)
            while (cur_tk.type == PUNC && cur_tk.value == "\n") {
                cur_tk = readNextTok();
                discard_comments();
            }

        if (_modifyCurTk) { p_CurTk = cur_tk; }
        return cur_tk;
    }

    /* Return the next token from the m_Stream without consuming it. */
    Token peek() {
        setReturnPoint();
        
        if (m_Stream.eof()) {
            restoreCache();
            return {NONE, "NULL", m_Stream.Line};  // Return token with type NONE and empty value
        }
        p_PeekTk = next(false, false, false);
        restoreCache();
        return p_PeekTk;
    }

    StreamState getStreamState() {
        return StreamState{
            .Line  = m_Stream.Line - 1,
            .Pos   = m_Stream.Pos,
            .Col   = m_Stream.Col,
            .CurLn = m_Stream.LineMap[m_Stream.Line]
        };
    }

    bool eof() const {
        return p_CurTk.type == NONE;
    }

    std::string getLineFromSrc(std::size_t index) {
        return m_Stream.LineMap[index];
    }

};

#endif