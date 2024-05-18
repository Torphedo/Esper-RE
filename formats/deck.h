#include <common/int.h>

typedef struct {
    u32 header;
    char name[16];
    s16 skills[30];
    u16 school_count;
    u16 meta; // Purpose unknown. Made up of 2 bytes, which seem to always be 0x00 or 0x40.
    u32 mission_clears;
    u32 mission_attempts;
    u32 multiplayer_wins;
    u32 multiplayer_win_rate;
}deck;

