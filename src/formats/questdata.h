#pragma once
#ifdef __cplusplus
extern "C" {
#endif
// This file has C structs for Phantom Dust QDT files. You can find one in the
// game files at:
//   Assets/Data/questdata/en/questdata.qdt

#include <stdbool.h>
#include <assert.h>
#include <common/int.h>
#include "shut_up_msvc.h"

enum {
    PD_MAX_QUESTS = 256, // Number of quest entries in a QDT
};

typedef struct {
    bool meister: 1;
    bool chunky: 1;
    bool cuff_button: 1;
    bool pH: 1;

    bool edgar: 1;
    bool know: 1;
    bool tsubutaki: 1;
    bool sammah: 1;
}partner_flags;

typedef struct {
    u16 deck_flag;
    union {
        partner_flags flags;
        u8 flags_byte;
    };
    u8 pad; // Probably bitfield for the monsters
    u16 unk;
    u16 time; // In minutes
    char name[0x12];
}deck_entry_header;
static_assert(sizeof(deck_entry_header) == 0x1A);

typedef struct {
    u16 deck_flag;
    union {
        partner_flags flags;
        u8 flags_byte;
    };
    u8 pad; // Probably bitfield for the monsters
    u16 unk;
    u16 time; // In minutes

    char name[0x12];
    u8 pad1[4];
    char aiprog[0x12];
    u8 pad2[10];
}deck_entry;
static_assert(sizeof(deck_entry) == 0x3A);

typedef struct {
    u16 unk1;
    s16 id;
    char name[0x32];
    char desc[0x200]; // The game uses a 512 byte buffer
    u16 pad;
    u16 unk2;
    u16 unk3;
    u16 unk4;
    u16 mission_photo;
    u16 unk5;
    u16 unk6;
    char unk_text[0x32];

    u16 num_players;
    u16 num_enemies;
    u16 player_slots;
    u16 unk7;
    u16 unk8;
    u16 unk9;

    char objective[0xCE];

    deck_entry_header header;
    deck_entry deck[7];
    u16 unkA[2];
    s16 stage;
    u8 unkB[0x1A];
}quest_entry;
static_assert(sizeof(quest_entry) == 0x520);

#ifdef __cplusplus
}
#endif
