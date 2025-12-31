/// @author Torphedo
/// @author Blasianblazy
/// Parser for Phantom Dust quest definitions

#include <std/mem.pat>

struct String {
    char txt[while($[$] != 0)]; // Null terminated string
    char nullterm [[color("000000")]]; // The null terminator
} [[format_read("stringtostring")]];

fn stringtostring(String s) {
    return s.txt;
};


struct deck_entry_header {
    u16 unk[4];
    char name[0x12];
};

struct deck_entry {
    u16 unk[4];
    char name[0x12];
    padding[4];
    char aiprog[0x12];
    padding[10];
};

struct entry {
    u16 unk1;
    u16 id;
    char name[0x32];
    char desc[0x202];
    u32 unk3;
    u16 unk4;
    u16 unk5;
    padding[4];
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
};


entry entries[256] @ 0x0;
