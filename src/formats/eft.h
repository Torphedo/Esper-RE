#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
// Structs for 412-byte ".eft" files in Assets/Data/Effect/

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
    uint16_t unk1;
    uint8_t unk2[2];
    uint32_t unk3[3];
}eft_iheader;
static_assert(sizeof(eft_iheader) == 0x10);

typedef struct {
    uint32_t idx; // Counts up by 1 per entry
    uint16_t unk1;
    uint16_t unk2; // Counts up by 1 or 2 per entry
    uint16_t unk3;
    uint16_t unk4;
}eft_ientry;
static_assert(sizeof(eft_ientry) == 0xC);

enum {
    EFT_EI_SIZE = 0x1744,
    EFT_EI_NUM_ENTRIES = 495,
};

typedef struct {
    eft_iheader header;
    eft_ientry entries[EFT_EI_NUM_ENTRIES];
}eft_ifile;
static_assert(sizeof(eft_ifile) == EFT_EI_SIZE);

#ifdef __cplusplus
}
#endif