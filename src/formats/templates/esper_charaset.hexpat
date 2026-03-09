#include <std/mem.pat>

struct String {
    char txt[while($[$] != 0)]; // Null terminated string
    char nullterm [[color("000000")]]; // The null terminator
} [[format_read("stringtostring")]];

fn stringtostring(String s) {
    return s.txt;
};

struct Header {
    u32 file_size;
    u32 entry_count;
    u32 string_pool_offset;
    padding[4];
};

Header header @ 0x0;

// Rebase a pointer to be within the resource buffer
fn text_ptr(u32 ptr) {
    if (ptr > header.file_size) {
        // Give it a "base" of of -1 * ptr, so it points at 0.
        // Then we can make it always 0xC, which is known to always
        // be 4 bytes of 0.
        return (-1 * ptr) + 0xC;
    }
    return header.string_pool_offset;
};


struct chara_entry {
    String* filename_offset : u32 [[pointer_base("text_ptr"), color("FF0000")]];
    String* name_offset : u32 [[pointer_base("text_ptr"), color("00FF00")]];
    String* name_offset2 : u32 [[pointer_base("text_ptr"), color("00FF00")]];
    u16 unk1;
    u16 unk2;
    u16 unk4;
    u16 unk5;
    u32 unk6;
    u16 unk7[4];
    u32 unk8;
    padding[4];
    u32 unk9;
    padding[4];
    u16 unkA[3];
    u16 jump_skill_id; // 750 = Jump, 751 = Cartwheel
    u16 unkB[2];
    String* sfx_filename_offset : u32 [[pointer_base("text_ptr"), color("0000FF")]];
    u16 unkC[2];
    padding[8];
};

chara_entry entries[header.entry_count] @ $;