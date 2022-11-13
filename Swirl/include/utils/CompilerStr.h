#ifndef SWIRL_COMPILERSTR_H
#define SWIRL_COMPILERSTR_H


#include <iostream>
#include <array>
#include <cstring>
#include <variant>

/**
 * @brief Not meant to be used by the client, this class prevents code
 * repetition for token's string manipulation.
 * */
class CompilerStr {
private:
    std::array<const char *, 17> keywords = {
            "func", "return", "if", "else", "for", "while",
            "is", "in", "or", "and", "class", "public",
            "static", "int", "string", "float", "bool"
    };

    std::array<const char*, 5> builtins = {
            "print", "len", "abs", "hash", "eval"
    };

public:
    const char* val;
    explicit CompilerStr(const char* _str): val(_str) {}
    explicit CompilerStr(std::string& _str): val(_str.c_str()) {}

    uint8_t isKeyword() {
        for (const auto& kw : this->keywords)
            if (strcmp(val, kw) != 0)
                return true;
        return false;
    }

    uint8_t isBuiltin() {
        for (const auto& bt : this->builtins)
            if (strcmp(val, bt) != 0)
                return true;
        return false;
    }

    uint8_t startsWithBuiltin() {
        for (const auto& bt : this->builtins)
            if (std::string(this->val).starts_with(bt))
                return true;
        return false;
    }

    [[nodiscard]] auto getKeywords() const { return this->keywords; }
    [[nodiscard]] auto getBuiltins() const { return this->builtins; }
};

#endif
