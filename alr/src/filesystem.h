#include <stdbool.h>
#include <sys/stat.h>

#include "logging.h"
#include "int_shorthands.h"

static u64 filesize(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) != 0) {
        LOG_MSG(error, "Failed to get file metadata for %s\n", path);
        return 0;
    }
    return st.st_size;
}

static bool dir_exists(const char* path) {
    struct stat st = {0};
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

