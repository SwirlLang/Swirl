#include <stdio.h>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif


#ifndef LOGGER_H_LAMBDA_CODE
#define LOGGER_H_LAMBDA_CODE

enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERR,
    CRITICAL
};

class Logger {
public:
    LogLevel logLevel;

    static void log(const char* message) {
        std::cout << "" << message << std::endl;
    }

    static void logToFile(const char* _logMessage) {
        fprintf(stdout, "%s\n", _logMessage);
    }
};

#endif