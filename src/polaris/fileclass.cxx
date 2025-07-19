#include "fileclass.hxx"
#include <stdio.h>
#include <stdlib.h>
#include <common/file.h>
#include <common/logging.h>

bool fileclass::load(const char* path) noexcept {
    size = file_size(path);
    if (!size) {
        return false;
    }

    data = file_load(path);
    if (!data) {
        return false;
    }

    if (!load_verify()) {
        unload();
        return false;
    }

    return true;
}

void fileclass::unload() noexcept {
    free(data);
    data = nullptr;
    size = 0;
}

bool fileclass::save(const char* path) const noexcept {
    FILE* f = fopen(path, "rb");
    if (!f) {
        LOG_MSG(error, "Couldn't open new file \"%s\"", path);
        return false;
    }

    bool result = true;
    if (fwrite(data, size, 1, f) != 1) {
        LOG_MSG(error, "Failed to save the entire %d bytes to new file \"%s\"\n", size, path);
        result = false;
    }

    fclose(f);
    return result;
}

fileclass::~fileclass() noexcept {
    free(data);
}
