#include <ios>
#include <iostream>
#include <map>
#include <tuple>
#include <array>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <functional>
#include <string_view>

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
    bool                                            m_Debug  = false;
    bool                                            m_rdfs   = false;
    std::string                                     m_Ret;
    std::string                                     m_Rax;
    InputStream                                     m_Stream;
    Token                                           m_lastTok{};
    Token                                           m_Cur{};
    std::array<std::size_t, 3>                      m_CacheState{};
public:
    Token p_CurTk{_NONE, ""};
    Token p_PeekTk{_NONE, ""};

    explicit TokenStream(InputStream& _stream, bool _debug = false) : m_Stream(_stream), m_Debug(_debug) {}

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
        return "_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"sv.find(
                _chr
        ) != std::string::npos;
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

    std::string readWhile(const std::function<bool (char)>& delimiter) {
        std::string ret;
        while (!m_Stream.eof()) {
            if (delimiter(m_Stream.peek())) {
                ret += m_Stream.next();
            } else {break;}
        }
        return ret;
    }

 
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

    Token readMacro() {
//        m_Stream.next(true);
//        if (m_Stream.next(true) == 't') {
//            m_Ret.replace(0, 6, "typedef");
//        }

        m_Ret = readEscaped('\n');
        return {MACRO, m_Ret};
    }

    Token readIdent(bool apndF = false) {
        m_Rax = readWhile(isId);
        if (apndF) m_Rax.insert(0, "f");
        return {
                isKeyword(m_Rax) ? KEYWORD : IDENT,
                m_Rax
        };
    }

    Token readNumber() {
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
        return {NUMBER, m_Ret};
    }

    void setReturnPoint() {
        m_CacheState = {m_Stream.Pos, m_Stream.Line, m_Stream.Col};
//        std::cout << "POS: " << m_CacheState[0] << " LINE: " << m_CacheState[1] << " COL: " << m_CacheState[2] << std::endl;
    }

    void restoreCache() {
        m_Stream.Pos  = m_CacheState[0];
        m_Stream.Line = m_CacheState[1];
        m_Stream.Col  = m_CacheState[2];
//        std::cout << "Restoring cache -> " << "POS: " << m_CacheState[0] << " LINE: " << m_CacheState[1] << " COL: " << m_CacheState[2] << std::endl;

    }

    Token readNextTok(bool _noIncrement = false) {
        if (m_Stream.eof()) {return {NONE, "null"};}
        auto chr = m_Stream.peek();
        if (chr == '"') return readString();
        if (chr == '\'') return readString('\'');
//        if (chr == '#') return readMacro();
        if (isDigit(chr)) return readNumber();
        if (isIdStart(chr)) return readIdent();

        m_Ret = std::string(1, m_Stream.next());

        if (isOpChar(chr)) {
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
    }


    Token next(bool _showTNw = false, bool _showTWs = false, bool _modifyCurTk = true) {
        Token cur_tk = readNextTok();

        if (!_showTWs)
            while (cur_tk.type == PUNC && cur_tk.value == " ")
                cur_tk = readNextTok();
        if (!_showTNw)
            while (cur_tk.type == PUNC && cur_tk.value == "\n")
                cur_tk = readNextTok();

        if (_modifyCurTk) { p_CurTk = cur_tk; }


        return cur_tk;
    }

    Token peek() {
        setReturnPoint();
        if (m_Stream.eof()) {
            restoreCache();
            return {NONE, "NULL"};  // Return token with type NONE and empty value
        }
        p_PeekTk = next(false, false, false);
        restoreCache();
        return p_PeekTk;
    }


    bool eof() const {
        return p_CurTk.type == NONE;
    }

//    std::map<const char*, std::size_t> getStreamState() const {
//        std::map<const char*, std::size_t> stream_state;
//        stream_state["POS"] = m_Stream.Pos;
//        stream_state["LINE"] = m_Stream.Line;
//        stream_state["COL"] = m_Stream.Col;
//        return stream_state;
//    }
//
//    void resetState() {
//        m_Stream.reset();
//    }
};

#endif
