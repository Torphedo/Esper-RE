#pragma once
#ifdef __cplusplus
extern "C" {
#endif
// This file has C structs for Phantom Dust QDT files. You can find one in the
// game files at:
//   Assets/Data/questdata/en/questdata.qdt

#include <stdbool.h>
#include <assert.h>
#include "data_types.h"
#include "shut_up_msvc.h"

enum {
    PD_MAX_QUESTS = 256, // Number of quest entries in a QDT
};

typedef enum {
    PD_REWARD_NONE = -1,
    PD_REWARD_RANDOM_SKILL = 0,
    PD_REWARD_SPECIFIC_SKILL = 1, // Reward value is skill ID
    PD_REWARD_ARSENAL1 = 2, // Reward value is number of schools
    PD_REWARD_CREDITS = 3,
    PD_REWARD_ARSENAL2 = 4,
    PD_REWARD_SKILL_RARITY = 5,
}pd_reward_type;

typedef struct {
    s16 type;
    s16 value;
}pd_reward;

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
    u16 character;
    u16 costume;

    char name[0x16];
    char aiprog[0x16];
    u16 team; // 0 = player's side
    u16 start_idx;
    u16 ai_param_idx;
    u16 health;
    u16 unk5;
}player_entry;
static_assert(sizeof(player_entry) == 0x3A);

typedef struct {
    u16 unk1; // 0 - 7, set to 0 in placeholder quests
    s16 id;
    char name[50];
    char desc[0x200]; // The game uses a 512 byte buffer
    u16 pad;
    u16 difficulty_rank; // e.g. C, B, A, or S in ASCII
    u16 unk3;
    u16 unk4;
    u16 mission_photo;
    u16 unk5;
    u16 unk6;
    char unk_text[50];

    u16 num_players;
    u16 num_enemies;
    u16 player_slots;
    u16 unk7;
    u16 unk8;
    u16 unk9;

    char main_objective[50];
    u16 shown_objectives[3];
    char bonus_objectives[50][3];

    deck_entry_header header;
    u16 pad2;
    u16 pad3;
    player_entry entries[7];
    u16 stage;
    u16 stage_variant;
    u16 unkA;
    pd_reward rewards[3];
    u8 unkB[10];
}quest_entry;
static_assert(sizeof(quest_entry) == 0x520);

#ifdef __cplusplus
}
#endif
