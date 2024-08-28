#include <iostream>
#include <map>

#include <tokenizer/InputStream.h>

#ifndef SWIRL_EXCEPTION_H
#define SWIRL_EXCEPTION_H

#define EXCEPTION_STR_LINE "\n--------------------------------------------------------------------------------\n\n"


enum EXCEPTION_TYPE {
    WARNING,
    ERROR
};

class ExceptionHandler {
    std::string m_Errors{};
    std::string m_Warnings{};
//    const TokenStream& m_StreamInst;

    bool m_HasErrors = false;

public:
    ExceptionHandler() = default;
//    explicit ExceptionHandler(const TokenStream& stream_inst): m_StreamInst(stream_inst) {}

    unsigned int NumberOfErrors = 0;

    void newException(EXCEPTION_TYPE,
                      std::size_t line,
                      std::size_t from,
                      std::size_t to,
                      const std::string& msg
                      );

    void raiseAll();
};

#endif
