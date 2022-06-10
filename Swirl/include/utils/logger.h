#include <stdio.h>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#ifndef LOGGER_H_Swirl
#define LOGGER_H_Swirl

enum LogLevel
{
    DEBUG,
    INFO,
    WARNING,
    ERR,
    CRITICAL
};

class Logger
{
};

#endif