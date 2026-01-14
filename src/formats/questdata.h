#pragma once
#ifdef __cplusplus
extern "C" {
#endif
// This file has C structs for Phantom Dust QDT files. You can find one in the
// game files at:
//   Assets/Data/questdata/en/questdata.qdt

#include <common/int.h>
#include "shut_up_msvc.h"

typedef struct {
    u16 unk[4];
    char name[0x12];
}deck_entry_header;
static_assert(sizeof(deck_entry_header) == 0x1A);

typedef struct {
    u16 unk[4];
    char name[0x12];
    u8 pad1[4];
    char aiprog[0x12];
    u8 pad2[10];
}deck_entry;
static_assert(sizeof(deck_entry) == 0x3A);

typedef struct {
    u16 unk1;
    u16 id;
    char name[0x32];
    char desc[0x202];
    u32 unk3;
    u16 unk4;
    u16 unk5;
    u32 padding;
    char unk_text[0x32];

    u16 num_players;
    u16 num_enemies;
    u16 player_slots;
    u16 thumbnail_id;
    u16 unk6;
    u16 unk7;

    char objective[0xCE];

    deck_entry_header header;
    deck_entry deck[7];
    u8 unk8[0x20];
}quest_entry;
static_assert(sizeof(quest_entry) == 0x520);

#ifdef __cplusplus
}
#endif
