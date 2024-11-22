#pragma once
#include <tokenizer/Tokenizer.h>
#include <tokens/Tokens.h>


namespace Tester {

constexpr auto RED = "\033[0;31m";
constexpr auto GREEN = "\033[0;32m";
constexpr auto WHITE = "\033[0;37m";

inline std::string TestSubjectSrc = R"(
return\n\n   fn\t\t
ident\t=
+-+-+2()
)";


inline void SW_ASSERT(const bool condition, const std::string_view message) {
    if (!condition)
        std::cout << RED << "[⨯] Tests Failed!\n\t" << WHITE << message << std::endl;
}

inline void testTokenStream() {
    TokenStream stream{InputStream(TestSubjectSrc)};
    std::array  expected_res = {
        Token{KEYWORD, "return"},
        Token{KEYWORD, "fn"},
        Token{IDENT, "ident"},
        Token{OP, "="},
        Token{OP, "+"},
        Token{OP, "-"},
        Token{OP, "+"},
        Token{OP, "-"},
        Token{OP, "+"},
        Token{NUMBER, "2"}
    };

    std::size_t index{};
    while (!stream.eof()) {
        if (stream.p_CurTk.type != UNINIT)
            SW_ASSERT(
                (stream.p_CurTk.type == expected_res[index].type && stream.p_CurTk.value == expected_res[index].value),
                std::format(
                    "EXPECTED: Token({}, {})\n\tGOT     : Token({}, {})",
                    to_string(expected_res[index].type),
                    expected_res[index].value,
                    to_string(stream.p_CurTk.type),
                    stream.p_CurTk.value
                    )
                );

        stream.next();
        index = stream.p_CurTk.type != UNINIT ? index + 1 : index;
    }
}

}