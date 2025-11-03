#include <hex/core.pat>
#include <std/mem.pat>

struct String {
    char txt[while($[$] != 0)]; // Null terminated string
    char nullterm [[color("000000")]]; // The null terminator
} [[format_read("stringtostring")]];

fn stringtostring(String s) {
    return s.txt;
};

struct ak_header {
    u32 header_size;
    u32 file_count;
    u32 file_offsets[file_count];
    String filenames[file_count];
};

struct ak_file<auto file_start, auto file_end> {
    u8 file_data[file_end - file_start] @ file_start;
};

ak_header header @ 0;

for (u32 i = 0, i < header.file_count, i += 1) {
    u32 file_end = std::mem::size();
    if (i + 1 < header.file_count) {
        file_end = header.file_offsets[i + 1];
    }

    // We aren't allowed to construct variable size arrays in loops.
    // So instead, we use template arguments to construct a type of the correct
    // size each iteration.
    ak_file<header.file_offsets[i], file_end> file @ $;
    hex::core::add_virtual_file(header.filenames[i].txt, file.file_data);
};