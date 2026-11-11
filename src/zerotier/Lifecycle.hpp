#pragma once
#include <cstdint>
#include <string>
// Public interface for applications that do not need VNet internals.
namespace embedded {
struct Options {
    bool enabled = false;
    std::string network;
    std::string peer;
};
inline bool parseNetwork(const std::string& text, uint64_t& value) {
    if (text.size() != 16) return false;
    uint64_t result = 0;
    for (char c : text) {
        unsigned digit;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else return false;
        result = (result << 4) | digit;
    }
    if (!result) return false;
    value = result;
    return true;
}
inline bool parsePeer(const std::string& text, uint32_t& value) {
    uint32_t result = 0;
    unsigned octet = 0, digits = 0, parts = 0;
    for (char c : text + ".") {
        if (c == '.') {
            if (!digits || ++parts > 4) return false;
            result = (result << 8) | octet;
            digits = octet = 0;
        } else {
            if (c < '0' || c > '9' || ++digits > 3) return false;
            octet = octet * 10 + c - '0';
            if (octet > 255) return false;
        }
    }
    if (parts != 4 || !result || (result >> 24) == 127 || (result >> 24) >= 224) return false;
    value = result;
    return true;
}
struct Status {
    bool running;
    int networkStatus;
    uint64_t node;
    uint32_t ip;
};
Status status();
unsigned streamPacketSize(const std::string& address);
bool start(const Options& options = {});
void stop();
}
