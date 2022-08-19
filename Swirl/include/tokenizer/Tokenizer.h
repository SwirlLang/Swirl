#include <iostream>
#include <vector>
#include <regex>
#include <limits>

#include <definitions/definitions.h>
#include <tokenizer/InputStream.h>

#ifndef SWIRL_TOKENIZER_H
#define SWIRL_TOKENIZER_H

#define SWIRL_TOKENIZER_H
#define I_FLAG std::regex_constants::ECMAScript | std::regex_constants::icase

class Tokenizer {
    InputStream m_Stream;
    std::string keywords = "func return if else for while"
                           " is in or and class public"
                           " static int string float bool";
public:
    explicit Tokenizer(InputStream& input): m_Stream(input) {}
    uint8_t isKeyword(std::string& _x) { return  keywords.find(" " + _x + " ") >= 0; }

    static uint8_t isDigit(char _x) { return std::regex_match(std::string(1, _x), std::regex("[0-9]", I_FLAG)); }
    static uint8_t isIdStart(char chr) {
        std::string valid_srt = "a b c d e f g h i j k l m n o q r s t u v w x y z _";
        return valid_srt.find(chr) != std::string::npos;
    }

    static uint8_t isId(char chr) { return isIdStart(chr) || std::string("\"?!-<>=0123456789").find(chr) >= 0; }
    static uint8_t isOpChar(char chr) { return std::string("+!-*%=&|<>/").find(chr) >= 0; }
    static uint8_t isPunctuation(char chr) { return std::string("();,{}[]").find(chr) >= 0; }
    static uint8_t isWhiteSpace(std::string& _x) { return std::string(" \n\t").find(_x) >= 0; }

    template<typename any>
    std::string readWhile(any pd) {
        std::string ret;
        while (!m_Stream.eof() && pd(m_Stream.peek()))
            ret += m_Stream.peek();
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
        return {"STRING", readEscaped('"').c_str()};
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
        return {"number", std::to_string(parseDouble(number)).c_str()};
    }

    std::array<const char*, 2> readNextTok() {
        if (m_Stream.eof()) return {nullptr, nullptr};
        auto chr = m_Stream.peek();
        if (chr == '"') return readString();
        if (isDigit(chr)) return readNumber();
        if (isIdStart(chr)) return readIdent();
        if (isPunctuation(chr)) return {
            "punc",
            std::string(1, m_Stream.next()).c_str()
        };

        if (isOpChar(chr)) return {
            "operator",
            readWhile(isOpChar).c_str()
        };

        m_Stream.raiseException(strcat("Failed to handle token ", &chr));
    }
};

#endif
