#pragma once
#include "util/fileclass.hxx"
#include <formats/st00.h>

struct mapdata : fileclass {
    const char* filepath = nullptr;

    // fileclass overrides
    bool load_verify() const noexcept override;

    const char* name_at_idx(u32 idx) const noexcept;
    bool offset_is_reasonable(s32 offset) noexcept;
    st00_t* get_header() {
        return (st00_t*)data;
    }

};
