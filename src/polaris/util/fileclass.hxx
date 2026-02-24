#pragma once
#include <common/int.h>

struct fileclass {
    u8* data = nullptr;
    u32 size = 0;

    virtual bool load(const char* path) noexcept;
    virtual bool load_verify() const noexcept {
        return true;
    }

    void unload() noexcept;
    bool save(const char* path) const noexcept;
    ~fileclass() noexcept;
};
