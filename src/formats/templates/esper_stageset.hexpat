#include <std/string.pat>
#include <std/io.pat>

struct String {
    char txt[while($[$] != 0)]; // Null terminated string
    char nullterm [[color("000000")]]; // The null terminator
} [[format_read("stringtostring")]];

fn stringtostring(String s) {
    return s.txt;
};

struct header {
    u32 file_size;
    u32 num_entries;
    u32 string_pool_ptr;
    padding[4];
};

header head @ 0x0;

// Rebase a pointer to be within the resource buffer
fn text_ptr(u32 ptr) {
    return head.string_pool_ptr;
};

struct stage_entry {
    String* official_name : u32 [[pointer_base("text_ptr"), color("00FF00")]];
    u32 name_length;
    String* internal_name : u32 [[pointer_base("text_ptr"), color("FF0000")]];
    u16 stage_num;
    u16 unk2;
    u32 unk3;
};

stage_entry stages[head.num_entries] @ 0x10;
String text[while (!std::mem::eof())] @ head.string_pool_ptr;
