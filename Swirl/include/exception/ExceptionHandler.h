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

    bool m_HasErrors = false;
public:
    unsigned int NumberOfErrors = 0;
    /*
     * Exception format:-
     *
     * [ERROR]: In file <PATH>
     * Line <LINE_NUM>, Col <COL>
     * <CONTENT>
     * ~~~~^^^^^
     *     <msg>
     * */
    void newException(EXCEPTION_TYPE,
                      std::size_t line,
                      std::size_t from,
                      std::size_t to,
                      const std::string&,
                      const std::string& msg
                      );

    void raiseAll();
};

#endif
