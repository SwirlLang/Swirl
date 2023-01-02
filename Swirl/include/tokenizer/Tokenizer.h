#include <iostream>
#include <string_view>
#include <vector>
#include <limits>
#include <array>
#include <functional>
#include <cstring>
#include <string_view>
#include <sstream>
#include <map>

#include <definitions/definitions.h>
#include <tokenizer/InputStream.h>
#include <utils/utils.h>

#ifndef SWIRL_TokenStream_H
#define SWIRL_TokenStream_H

#define SWIRL_TokenStream_H

using namespace std::string_view_literals;

const defs DEF{};
using namespace std::string_view_literals;
class TokenStream {
    bool                                            m_Debug;
    std::string                                     m_Ret;
    std::string                                     m_Rax;
    InputStream                                     m_Stream;
    std::array<const char*, 2>                      m_PeekTk{};
    std::array<const char*, 2>                      m_Cur{};
    std::array<std::array<const char*, 2>, 5>       m_TokReserve{};

public:
    std::array<const char*, 2> p_CurTk{"", ""};

    explicit TokenStream(InputStream& _stream, bool _debug = false) : m_Stream(_stream), m_Debug(_debug) {}

    bool isKeyword(const std::string& _str) {
        return std::find(DEF.keywords.begin(), DEF.keywords.end(), _str) != DEF.keywords.end();
    }

    static bool isDigit(char _chr) {
        return "1234567890"sv.find(_chr) != std::string::npos;
    }

    static uint8_t isId(char chr) {
        return isIdStart(chr) || "\"?!-<>=0123456789"sv.find(chr) != std::string::npos;
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
        return "!=*&<>-/+"sv.find(_chr) != std::string::npos;
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

    std::array<const char*, 2> readString(char del = '"') {
        m_Ret = readEscaped(del);
        return {"STRING", m_Ret.c_str()};
    }

    static double parseDouble( const std::string& s ) {
        std::istringstream i(s);
        double x;
        if (!(i >> x)) { return std::numeric_limits<double>::quiet_NaN(); }
        return x;
    }

    std::array<const char*, 2> readIdent() {
        m_Rax = readWhile(isId);
        return {
                isKeyword(m_Rax) ? "KEYWORD" : "IDENT",
                m_Rax.c_str()
        };
    }

    std::array<const char*, 2> readNumber() {
        static uint8_t has_decim = false;
        std::string number = readWhile([](char ch) {
            if (ch == '.') {
                if (has_decim) return false;
                has_decim = true;
                return true;
            } return isDigit(ch);
        });
        has_decim = false;
        m_Ret = std::to_string(parseDouble(number));
        return {"NUMBER", m_Ret.c_str()};
    }

    std::array<const char*, 2> readNextTok(bool _noIncrement = false) {
        if (m_Stream.eof()) {return {"null", "null"};}
        auto chr = m_Stream.peek();
        if (chr == '"') return readString();
        if (chr == '\'') return readString('\'');
        if (isDigit(chr)) return readNumber();
        if (isIdStart(chr)) return readIdent();

        m_Ret = std::string(1, m_Stream.next());

        if (isOpChar(chr)) {
            m_Rax = chr + readWhile(isOpChar);
            return {
                    "OP",
                    m_Rax.c_str()
            };
        }

        if (isPunctuation(chr) ) return {
                    "PUNC",
                    m_Ret.c_str()
            };

        m_Stream.raiseException(strcat((char*)"Failed to handle token ", &chr));
    }

    std::array<const char*, 2> next(uint8_t _showTNw = false, uint8_t _showTWs = false) {
        p_CurTk = readNextTok();
        if (!_showTWs)
            if (strcmp(p_CurTk[0], "PUNC") == 0 && strcmp(p_CurTk[1], " ") == 0)
                p_CurTk = readNextTok();
        if (!_showTNw)
            if (strcmp(p_CurTk[0], "PUNC") == 0 && strcmp(p_CurTk[1], "\n") == 0)
                p_CurTk = readNextTok();
        if (m_Debug)
            std::cout << "Token Requested:\t" << p_CurTk[0] << "\t  " << p_CurTk[1] << std::endl;
        return p_CurTk;
    }

    std::array<const char*, 2> peek() const {
        return m_PeekTk;
    }

    bool eof() {
        return false;
    }

    std::map<const char*, std::size_t> getStreamState() {
        std::map<const char*, std::size_t> stream_state;
        stream_state["LINE"] = m_Stream.getLine();
        stream_state["POS"] = m_Stream.getPos();
        stream_state["COL"] = m_Stream.getCol();
        return stream_state;
    }
};

#endif