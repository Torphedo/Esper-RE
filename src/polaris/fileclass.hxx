#pragma once
#include <common/int.h>

struct fileclass {
    u8* data = nullptr;
    u32 size = 0;

    bool load(const char* path) noexcept;

    /// @brief Optional child class callback to verify file magic
    ///
    /// @return If this returns false, loading is cancelled
    virtual bool load_verify() const noexcept {
        return true; // Default stub
    }

    void unload() noexcept;
    bool save(const char* path) const noexcept;

    /// @brief Frees underlying file data
    ~fileclass() noexcept;
};
