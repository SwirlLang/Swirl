#include <iostream>
#include <map>
#include <tuple>
#include <array>
#include <vector>
#include <cstring>
#include <functional>
#include <string_view>

#include <definitions/definitions.h>
#include <tokenizer/InputStream.h>
#include <utils/utils.h>
#include <tokens/Tokens.h>

#ifndef SWIRL_TokenStream_H
#define SWIRL_TokenStream_H

#define SWIRL_TokenStream_H


const defs DEF{};
using namespace std::string_view_literals;

class TokenStream {
    bool                                            m_Debug  = 0;
    bool                                            m_rdfs   = 0;
    std::string                                     m_Ret;
    std::string                                     m_Rax;
    InputStream                                     m_Stream;
    Token                                           m_PeekTk = {_NONE, ""};
    Token                                           m_lastTok{};
    Token                                           m_Cur{};
    std::array<std::size_t, 3>                      m_CacheState{};
public:
    Token p_CurTk{_NONE, ""};
    Token p_PeekTk{_NONE, ""};

    explicit TokenStream(InputStream& _stream, bool _debug = false) : m_Stream(_stream), m_Debug(_debug) {}

    static bool isKeyword(const std::string& _str) {
        return std::find(DEF.keywords.begin(), DEF.keywords.end(), _str) != DEF.keywords.end();
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
        return "!=*&<>-/+^%"sv.find(_chr) != std::string::npos;
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
        return {STRING, m_Ret.c_str()};
    }

    Token readMacro() {
        m_Stream.next(true);
        if (m_Stream.next(true) == 't') {
            m_Ret.replace(0, 6, "typedef");
        }

        m_Ret = readEscaped('\n');
        return {MACRO, m_Ret.c_str()};
    }

    Token readIdent(bool apndF = false) {
        m_Rax = readWhile(isId);
        if (apndF) m_Rax.insert(0, "f");
        return {
                isKeyword(m_Rax) ? KEYWORD : IDENT,
                m_Rax.c_str()
        };
    }

    Token readNumber() {
        static uint8_t has_decim = false;
        std::string number = readWhile([](char ch) {
            if (ch == '.') {
                if (has_decim) return false;
                has_decim = true;
                return true;
            } return isDigit(ch);
        });
        has_decim = false;
        m_Ret = number;
        return {NUMBER, m_Ret.c_str()};
    }

    void setReturnPoint() {
        m_CacheState = {m_Stream.Pos, m_Stream.Line, m_Stream.Col};
    }

    void restoreCache() {
        m_Stream.Pos = m_CacheState[0];
        m_Stream.Line = m_CacheState[1];
        m_Stream.Col  = m_CacheState[2];
    }

    Token readNextTok(bool _noIncrement = false) {
        if (m_Stream.eof()) {return {NONE, "null"};}
        auto chr = m_Stream.peek();
        if (chr == '"') return readString();
        if (chr == '\'') return readString('\'');
        if (chr == '#') return readMacro();
        if (isDigit(chr)) return readNumber();

        if (chr == 'f') {
            m_Stream.next();
            chr = m_Stream.peek();
            if (chr == '"' || chr == '\'')
                return readString(chr, true);
            else return readIdent(true);
        }

        if (isIdStart(chr)) return readIdent();

        m_Ret = std::string(1, m_Stream.next());

        if (isOpChar(chr)) {
            m_Rax = chr + readWhile(isOpChar);
            return {
                    OP,
                    m_Rax.c_str()
            };
        }

        if (isPunctuation(chr) ) return {
                    PUNC,
                    m_Ret.c_str()
            };
    }

    Token next(const bool _showTNw = false, const bool _showTWs = false) {
        p_CurTk.type == _NONE ? p_CurTk = readNextTok() : p_CurTk = m_PeekTk;
        m_PeekTk = readNextTok();

        if (!_showTWs)
            if (m_PeekTk.type == PUNC && m_PeekTk.value == " ")
                m_PeekTk = readNextTok();

        if (!_showTNw)
            if (m_PeekTk.type == PUNC && m_PeekTk.value == "\n")
                m_PeekTk = readNextTok();

        if (m_Debug)
            std::cout << "Token Requested:\t" << p_CurTk.type << "\t  " << p_CurTk.value << std::endl;

        return p_CurTk;
    }

    Token peek() const {
        return m_PeekTk;
    }

    bool eof() {
        return false;
    }

    std::map<const char*, std::size_t> getStreamState() {
        std::map<const char*, std::size_t> stream_state;
        stream_state["POS"] = m_Stream.Pos;
        stream_state["LINE"] = m_Stream.Line;
        stream_state["COL"] = m_Stream.Col;
        return stream_state;
    }

    void resetState() {
        m_Stream.reset();
    }
};

#endif