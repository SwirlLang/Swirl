#include <iostream>
#include <vector>
#include <limits>
#include <array>
#include <functional>

#include <tokenizer/InputStream.h>
#include <utils/utils.h>

#ifndef SWIRL_TokenStream_H
#define SWIRL_TokenStream_H

#define SWIRL_TokenStream_H

class TokenStream {
    std::string m_Ret;
    std::string m_Rax;
    InputStream m_Stream;
    std::string keywords = "func return if else for while"
                           " is in or and class public"
                           " static int string float bool";
public:
    explicit TokenStream(InputStream& _stream, bool _debug = false) : m_Stream(_stream) {}
    bool isKeyword(const std::string& _str) { return keywords.find(_str) != std::string::npos; }
    static bool isDigit(char _chr) { return std::string("1 2 3 4 5 6 7 8 9 0").find(std::string(1, _chr)) != std::string::npos; }
    static uint8_t isId(char chr) { return isIdStart(chr) || std::string("\"?!-<>=0123456789").find(chr) != std::string::npos; }
    static bool isIdStart(char _chr) {
        return std::string("_ a b c d e f g h i j k l m n o p q r s t u v w x y z").find(std::string(1, _chr)) != std::string::npos;
    }

    static uint8_t isPunctuation(char chr) { return std::string("();,{}[]").find(chr) >= 0; }
    static bool isOpChar(char _chr) { return std::string("/=>*-%").find(_chr) != std::string::npos; }
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

    double parseDouble( const std::string& s ) {
        std::istringstream i(s);
        double x;
        if (!(i >> x)) { return std::numeric_limits<double>::quiet_NaN(); }
        return x;
    }

    std::array<const char*, 2> readIdent() {
        auto ident = readWhile(isId);
        return {
                isKeyword(ident) ? "ident" : "var",
                ident.c_str()
        };
    }

    std::array<const char*, 2> readNumber() {
        static uint8_t has_decim = false;
        auto number = readWhile([this](char ch) -> uint8_t {
            if (ch == '.') {
                if (has_decim) return false;
                has_decim = true;
                return true;
            } return isDigit(ch);
        });

        m_Ret = std::to_string(parseDouble(number));
        return {"number", m_Ret.c_str()};
    }

    std::array<const char*, 2> readNextTok() {
        if (m_Stream.eof()) return {nullptr, nullptr};
        auto chr = m_Stream.peek();
        if (chr == '"') return readString();
        if (isDigit(chr)) return readNumber();
        if (isIdStart(chr)) return readIdent();

        m_Ret = std::string(1, m_Stream.next());
        if (isPunctuation(chr)) return {
                    "punc",
                    m_Ret.c_str()
            };

        if (isOpChar(chr)) {
            m_Rax = readWhile(isOpChar);
            return {
                    "operator",
                    m_Rax.c_str()
            };
        }

        m_Stream.raiseException(strcat("Failed to handle token ", &chr));
    }
};

#endif
