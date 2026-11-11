#pragma once
#include <string>
// Only used by discarded gs_init() code in host launch/parser tests.
class Settings {
public:
    static Settings &instance() { static Settings value;return value; }
    std::string key_dir() const { return "/tmp/unused-key"; }
};
