#include "mapdata.hxx"
#include <string>
#include <common/logging.h>

bool mapdata::load_verify() const noexcept {
    if (!data) {
        return false;
    }
    const u32 magic = *(u32*)data;
    const bool is_st00 = (magic == st00_magic);
    const bool is_area = strncmp((char*)data, "AR0", 3) == 0;
    if (!is_st00 && !is_area) {
        LOG_MSG(error, "\"%s\" doesn't seem to be a .dat map file (invalid magic 0x%x)\n", filepath, magic);
        return false;
    }
    return true;
}

const char* mapdata::name_at_idx(u32 idx) const noexcept {
    const auto* header = (st00_t*)data;
    if (idx >= header->nm00_count) {
        return "";
    }

    u32 i = 0;
    const char* txt = (const char*)(data + header->nm00_offset);
    while (idx > i) {
        txt += strlen(txt) + 1;
        i++;
    }

    return txt;
}

bool mapdata::offset_is_reasonable(s32 offset) noexcept {
    if (offset <= 0 || offset > size) {
        return false;
    }
    return true;
}
