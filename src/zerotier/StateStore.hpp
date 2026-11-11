#pragma once
#include <ZeroTierOne.h>
namespace embedded {
int readState(const char *root, ZT_StateObjectType type, const uint64_t id[2], void *data, unsigned capacity);
bool writeState(const char *root, ZT_StateObjectType type, const uint64_t id[2], const void *data, int length);
}
