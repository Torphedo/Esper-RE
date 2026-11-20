#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
// Structs for 412-byte ".eft" files in Assets/Data/Effect/

// Reportedly, only the 2nd field is actually read by game code.
// TODO: Independently verify this
typedef struct {
    uint32_t unk1;
    uint32_t unk2;
    uint32_t unk3;
    uint32_t unk4;
    uint32_t unk5;
}eft_high_entry;
static_assert(sizeof(eft_high_entry) == 0x14);

typedef struct {
    int16_t unk[14]; // The first 4 are usually real values, the rest are -1
}eft_low_entry;
static_assert(sizeof(eft_low_entry) == 0x1C);

typedef struct {
    eft_high_entry entries[12];
    uint32_t unk; // Maybe padding
    eft_low_entry entries2[6];
}eft_file;
static_assert(sizeof(eft_file) == 412);


// Structs for the unique file /Assets/Data/Effect/e.i
typedef struct {
    uint16_t pack_count;
    uint16_t entry_count;
}eft_index_header;
static_assert(sizeof(eft_index_header) == 4);

typedef struct {
    uint16_t effect_id;
    uint16_t pack_id;
    uint32_t offset;
    uint32_t size;
}eft_index_entry;
static_assert(sizeof(eft_index_entry) == 0xC);

enum {
    EFT_EI_SIZE = 0x1744,
    EFT_EI_NUM_ENTRIES = 496,
};

typedef struct {
    eft_index_header header;
    eft_index_entry entries[EFT_EI_NUM_ENTRIES];
}eft_ifile;
static_assert(sizeof(eft_ifile) == EFT_EI_SIZE);

#ifdef __cplusplus
}
#endif
