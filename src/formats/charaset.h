#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "data_types.h"

typedef struct {
    u32 filename_offset;
    u32 name_offset;
    u32 name_offset2;
    u16 unk1;
    u16 unk2;
    u16 unk3;
    u16 unk4;
    u16 unk5;
    u16 unk6;
    u16 unk7[4];
    u32 unk8;
    u32 padding1;
    u16 unk9;
    u16 unk10;
    u32 padding2;
    u16 unk11[3];
    u16 jump_skill_id; // 750 = Jump, 751 = Cartwheel
    u16 unk12[2];
    u32 sfx_filename_offset;
    u16 unk13[2];
    u32 padding3[2];
}charaset_entry;
static_assert(sizeof(charaset_entry) == 0x4C,  "Wrong charaset entry size!");

typedef struct {
    u32 file_size;
    u32 entry_count;
    u32 string_pool_offset;
    u32 padding;
    charaset_entry entries[];
}charaset_header;
static_assert(sizeof(charaset_header) == 0x10, "Wrong charaset header size!");

#ifdef __cplusplus
}
#endif
