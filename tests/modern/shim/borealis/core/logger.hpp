#pragma once
namespace brls {
struct Logger {
    template<class... Args> static void info(const char*, Args&&...) {}
    template<class... Args> static void error(const char*, Args&&...) {}
    template<class... Args> static void debug(const char*, Args&&...) {}
};
}
