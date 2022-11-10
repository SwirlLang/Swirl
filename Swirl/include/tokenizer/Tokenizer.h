#include <iostream>
#include <vector>
#include <limits>
#include <array>
#include <functional>
#include <cstring>
#include <sstream>

#include <definitions/definitions.h>
#include <tokenizer/InputStream.h>
#include <utils/utils.h>
#include <map>

#ifndef SWIRL_TokenStream_H
#define SWIRL_TokenStream_H

#define SWIRL_TokenStream_H

const defs DEF{};

class TokenStream {
    std::string m_Ret;
    std::string m_Rax;
    InputStream m_Stream;
    std::array<const char*, 2> m_PeekTk{};
    std::array<const char*, 2> m_Cur = {};
public:
    std::array<const char*, 2> p_CurTk = {"", ""};

    explicit TokenStream(InputStream& _stream, bool _debug = false) : m_Stream(_stream) {}

    bool isKeyword(const std::string& _str) { return std::find(DEF.keywords.begin(), DEF.keywords.end(), _str) != DEF.keywords.end(); }

    static bool isDigit(char _chr) { return std::string("1234567890").find(std::string(1, _chr)) != std::string::npos; }
    static uint8_t isId(char chr) { return isIdStart(chr) || std::string("\"?!-<>=0123456789").find(chr) != std::string::npos; }
    static bool isIdStart(char _chr) {
        return std::string("_ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz").find(std::string(1, _chr)) != std::string::npos;
    }

    static bool isPunctuation(char chr) { return std::string("();,{}[]").find(chr) >= 0; }
    static bool isOpChar(char _chr) { return std::string("!=*&<>-/").find(_chr) != std::string::npos; }
    static bool isWhiteSpace(char _chr) { return std::string(" \t\n").find(_chr) != std::string::npos; }

    std::string readWhile(const std::function<bool (char)>& delimiter) {
        std::string ret;
        while (!m_Stream.eof() && delimiter(m_Stream.peek())) {
            ret += m_Stream.next();
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
            } else if (chr == '\\')
                is_escaped = true;
            else if (chr == _end)
                break;
            else
                ret += chr;
        }
        return ret;
    }

    std::array<const char*, 2> readString() {
        m_Ret = readEscaped('"');
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
        auto number = readWhile([](char ch) -> uint8_t {
            if (ch == '.') {
                if (has_decim) return false;
                has_decim = true;
                return true;
            } return isDigit(ch);
        });

        m_Ret = std::to_string(parseDouble(number));
        return {"NUMBER", m_Ret.c_str()};
    }

    std::array<const char*, 2> readNextTok() {
        if (m_Stream.eof()) return {nullptr, nullptr};
        auto chr = m_Stream.peek();
        if (chr == '"') return readString();
        if (isDigit(chr)) return readNumber();
        if (isIdStart(chr)) return readIdent();

        m_Ret = std::string(1, m_Stream.next());
        if (isPunctuation(chr) ) return {
                    "PUNC",
                    m_Ret.c_str()
            };

        std::cout << chr;
        if (isOpChar(chr)) {
            std::cout << "Is op char! " << chr << "\n";
            m_Rax = readWhile(isOpChar);
            return {
                    "OPERATOR",
                    m_Rax.c_str()
            };
        }

        m_Stream.raiseException(strcat((char*)"Failed to handle token ", &chr));
    }

    std::array<const char*, 2> next() {
        p_CurTk = readNextTok();
        return p_CurTk;
    }

    bool eof() { return m_Stream.eof(); }

    std::map<const char*, std::size_t> getStreamState() {
        std::map<const char*, std::size_t> stream_state;
        stream_state["LINE"] = m_Stream.getLine();
        stream_state["POS"] = m_Stream.getPos();
        stream_state["COL"] = m_Stream.getCol();
        return stream_state;
    }
};

#endif
