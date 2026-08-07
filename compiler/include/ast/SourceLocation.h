#pragma once
#include "lexer/TokenStream.h"


struct SourceLocation {
    StreamState     from;
    StreamState     to;
    sw::FileHandle* source = nullptr;

    [[nodiscard]]
    std::string toString() const {
        return std::format("{}:{}", from.Line, from.Col);
    }
};
