#include "StateStore.hpp"
#include <cstdio>
#include <cerrno>
namespace embedded {
namespace {
bool statePath(char *path, size_t capacity, const char *root, ZT_StateObjectType type, const uint64_t id[2]) {
    // Identity and planet are queried with ID zero at startup, then stored
    // using their real IDs. Their filenames must therefore ignore the ID.
    switch (type) {
    case ZT_STATE_OBJECT_IDENTITY_PUBLIC:
        snprintf(path, capacity, "%s/identity.public", root);
        return true;
    case ZT_STATE_OBJECT_IDENTITY_SECRET:
        snprintf(path, capacity, "%s/identity.secret", root);
        return true;
    case ZT_STATE_OBJECT_PLANET:
        snprintf(path, capacity, "%s/planet", root);
        return true;
    case ZT_STATE_OBJECT_NETWORK_CONFIG:
    case ZT_STATE_OBJECT_PEER:
    case ZT_STATE_OBJECT_MOON:
        if (!id) return false;
        snprintf(path, capacity, "%s/%u-%016llx-%016llx", root, (unsigned)type,
                 (unsigned long long)id[0], (unsigned long long)id[1]);
        return true;
    default:
        return false;
    }
}
}
int readState(const char *root, ZT_StateObjectType type, const uint64_t id[2], void *data, unsigned capacity) {
    char path[256];
    if (!statePath(path, sizeof(path), root, type, id)) return -1;
    FILE *file = fopen(path, "rb");
    if (!file) return -1;
    int result = -1;
    if (fseek(file, 0, SEEK_END) == 0) {
        long size = ftell(file);
        rewind(file);
        if (size >= 0 && (unsigned long)size <= capacity && fread(data, 1, size, file) == (size_t)size) {
            result = size;
        }
    }
    fclose(file);
    return result;
}
bool writeState(const char *root, ZT_StateObjectType type, const uint64_t id[2], const void *data, int length) {
    char path[256], temporary[272];
    if (!statePath(path, sizeof(path), root, type, id)) return false;
    if (length < 0) return remove(path) == 0 || errno == ENOENT;
    snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    FILE *file = fopen(temporary, "wb");
    if (!file) return false;
    bool ok = fwrite(data, 1, length, file) == (size_t)length;
    if (fflush(file)) ok = false;
    if (fclose(file)) ok = false;
    if (!ok) { remove(temporary); return false; }
    return rename(temporary, path) == 0;
}
}
