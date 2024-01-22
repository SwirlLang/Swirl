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
    // Just as a data struct for representing the state of the tokenizer.
    struct State {
        std::size_t Pos, Line, Col;
        State(
                std::size_t pos = 0,
                std::size_t line = 0,
                std::size_t col = 0
                ): Pos(pos), Line(line), Col(col) {}
    };

    int                                          m_Pcnt  = 0;       // paren counter
    bool                                         m_Debug  = false;  // Debug flag
    bool                                         m_rdfs   = false;  // deprecated string interpolation flag
    bool                                         m_trackp = false;  // track parentheses flag
    std::string                                  m_Ret;             // temporary cache
    std::string                                  m_Rax;             // temporary cache
    InputStream                                  m_Stream;          // input stream instance
    Token                                        m_lastTok{};
    Token                                        m_Cur{};
    std::array<State, 3>                         m_Cache = {};


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
        return operators.contains(std::string(1, _chr));
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

    /* Used to start reading a stream of characters till the `_end` char is hit. */
    std::string readEscaped(char _end, char apnd = 0) {
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
        if (!apnd) ret += apnd;
        return ret;
    }

    Token readString(char del = '"', bool _format = false) {
        m_Ret = readEscaped(del);
        m_Ret.pop_back();
        m_Ret.insert(0, "\"");
        m_Ret.append("\"");
        if (_format) { m_Ret.insert(0, "f"); m_rdfs = false; }
        return {STRING, m_Ret};
    }

    Token readIdent(bool apndF = false) {
        m_Rax = readWhile(isId);
        if (apndF) m_Rax.insert(0, "f");
        return {
                isKeyword(m_Rax) ? KEYWORD : IDENT,
                m_Rax
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
        return {NUMBER, neg + m_Ret};
    }

    /* Consume the next token from the stream. */
    Token readNextTok(bool _noIncrement = false) {
        setReturnPoint(0);

        if (m_Stream.eof()) {return {NONE, "null"};}
        auto chr = m_Stream.peek();
        if (chr == '"') return readString();
        if (chr == '\'') return readString('\'');
        if (isDigit(chr)) return readNumber();
        if (isIdStart(chr)) return readIdent();

        m_Ret = std::string(1, m_Stream.next());

        if (isOpChar(chr)) {
//            if (chr == '-') {
//                m_Stream.next();
//                if (isDigit(m_Stream.peek())) {
//                    Token ret = readNumber("-");
//                    m_Stream.backoff();
//                    return ret;
//                }
//            }

            m_Rax = chr + readWhile(isOpChar);
            return {
                    OP,
                    m_Rax
            };
        }

        if (isPunctuation(chr) ) return {
                    PUNC,
                    m_Ret
            };


        throw std::runtime_error("[FATAL]: No valid token found");
    }


public:
    Token p_CurTk{_NONE, ""};
    Token p_PeekTk{_NONE, ""};

    explicit TokenStream(InputStream& _stream, bool _debug = false) : m_Stream(_stream), m_Debug(_debug) {}

    /* Enabling this flag makes the tokenizer track the current open parentheses and omit its closing counterpart
     * from the stream when it is encountered. Eases the process of parser recovering from an error. */
    inline void trackParen() {
        m_trackp = true;
        m_Pcnt = 1;
    }


    /* Stores the state of the tokenizer in one of the three locations of the cache array
     * Id's work as defined below :-
     * 0: previous token
     * 1: succeeding token (peek)
     * 2: custom return point */
    void setReturnPoint(uint8_t id) {
        m_Cache[id] = {m_Stream.Pos, m_Stream.Line, m_Stream.Col};
    }

    void restoreCache(uint8_t id) {
        m_Stream.Pos  = m_Cache[id].Pos;
        m_Stream.Line = m_Cache[id].Line;
        m_Stream.Col  = m_Cache[id].Col;

    }

    /* An abstraction over readNextTok. */
    Token next(bool _readNewLines = false, bool _readWhitespaces = false, bool _modifyCurTk = true) {
        Token cur_tk = readNextTok();

        if (m_trackp) {
            if (cur_tk.type == PUNC) {
                if (cur_tk.value == "(") m_Pcnt++;
                if (cur_tk.value == ")") {
                    m_Pcnt--;
                    if (m_Pcnt == 0) { cur_tk = readNextTok(); m_trackp = false; }

                }
            }
        }

        if (!_readWhitespaces)
            while (cur_tk.type == PUNC && cur_tk.value == " ")
                cur_tk = readNextTok();
        if (!_readNewLines)
            while (cur_tk.type == PUNC && cur_tk.value == "\n")
                cur_tk = readNextTok();

        if (_modifyCurTk) { p_CurTk = cur_tk; }
        return cur_tk;
    }

    /* Return the next token from the stream without consuming it. */
    Token peek() {
        setReturnPoint(1);
        if (m_Stream.eof()) {
            restoreCache(1);
            return {NONE, "NULL"};  // Return token with type NONE and empty value
        }
        p_PeekTk = next(false, false, false);
        restoreCache(1);
        return p_PeekTk;
    }

    /* Return the previous token, then restore the state of the stream. */
    Token previous() {
        Token pre;
        setReturnPoint(2);
        restoreCache(0);
        pre = next();
        restoreCache(2);
        return pre;
    }

    bool eof() const {
        return p_CurTk.type == NONE;
    }
};

#endif
