#include <malloc.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

#include "logging.h"
#include "int.h"

u64 filesize(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) != 0) {
        LOG_MSG(error, "Failed to get file metadata for %s\n", path);
        return 0;
    }
    return st.st_size;
}

bool file_exists(const char* path) {
    struct stat st = {0};
    u32 result = stat(path, &st);

    // File should be a regular file (S_IFREG) and stat() should return success
    bool is_regular_file = ((st.st_mode & S_IFMT) == S_IFREG);
    return ((result == 0) && is_regular_file);
}

bool path_is_file(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISREG(st.st_mode);
}

bool path_is_dir(const char* path) {
    struct stat st = {0};
    if (stat(path, &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool path_has_extension(const char* path, const char* extension) {
    uint32_t pos = strlen(path);
    uint16_t ext_length = strlen(extension);

    // File extension is longer than input string.
    if (ext_length > pos) {
        return false;
    }
    return (strncmp(&path[pos - ext_length], extension, ext_length) == 0);
}


bool dir_exists(const char* path) {
    struct stat st = {0};
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

u8* file_load(const char* path) {
    if (!file_exists(path)) {
        LOG_MSG(error, "Requested file %s does not exist.\n", path);
        return NULL;
    }
    u32 size = filesize(path);
    u8* buffer = calloc(1, size);
    FILE* resource = fopen(path, "rb");

    // Clean up & return NULL if anything goes wrong
    if (resource == NULL || buffer == NULL) {
        fclose(resource);
        free(buffer);
        return NULL;
    }

    // Read the entire file and return its contents
    fread(buffer, size, 1, resource);
    fclose(resource);
    return buffer;
}

bool is_dirsep(char c) {
    return (c == '/' || c == '\\');
}

