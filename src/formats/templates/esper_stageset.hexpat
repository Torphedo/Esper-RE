// Supports Phantom Dust stage set files, found in Assets/Data/setup/stageset
// @author Torphedo

#include <std/mem.pat>

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
    // e.g. "Spiral Highway"
    String* full_name : u32 [[pointer_base("text_ptr"), color("00FF00")]];
    
    // e.g. "Highway"
    String* short_name : u32 [[pointer_base("text_ptr"), color("FF0000")]];
    
    // Name seen in debug menus
    String* internal_name : u32 [[pointer_base("text_ptr"), color("FF0000")]];
    
    // Number of the stage ALR/DAT/etc. file to load
    u16 stage_num;
    
    u16 variant_num;
    
    // Set to 1 for stages visible to the player
    u32 is_public;
};

stage_entry stages[head.num_entries] @ 0x10;
String text[while (!std::mem::eof())] @ head.string_pool_ptr;