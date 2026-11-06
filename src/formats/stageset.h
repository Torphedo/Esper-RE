#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#include "data_types.h"

typedef struct {
    u32 name_offset;
    u32 short_name_offset;
    u32 internal_name_offset;
    u16 stage_id;
    u16 variant_id;
    u32 visibility;
}stageset_entry;
static_assert(sizeof(stageset_entry) == 0x14,  "Wrong stageset entry size!");

typedef struct {
    u32 file_size;
    u32 entry_count;
    u32 string_pool_offset;
    u32 padding;
    stageset_entry entries[];
}stageset_header;
static_assert(sizeof(stageset_header) == 0x10, "Wrong stageset header size!");

#ifdef __cplusplus
}
#endif
