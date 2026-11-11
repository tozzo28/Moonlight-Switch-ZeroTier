#pragma once
#include <borealis/core/logger.hpp>
#include <cstdarg>
#include <cstdio>
// Adapter for the shared embedded transport; UI logging remains Borealis-native.
class Logger {
public:
    static void info(const char *tag, const char *format, ...) {
        char message[1024]; va_list args; va_start(args, format);
        vsnprintf(message, sizeof(message), format, args); va_end(args);
        brls::Logger::info("{}: {}", tag, message);
    }
    static void error(const char *tag, const char *format, ...) {
        char message[1024]; va_list args; va_start(args, format);
        vsnprintf(message, sizeof(message), format, args); va_end(args);
        brls::Logger::error("{}: {}", tag, message);
    }
};
