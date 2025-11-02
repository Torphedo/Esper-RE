// SSB file format for use in ImHex hex editor.
// By Torphedo, Nov. 2 2025.
#include <std/io.pat>
#include <std/string.pat>

struct String {
    char txt[while($[$] != 0)]; // Null terminated string
    char nullterm [[color("000000")]]; // The null terminator
} [[format_read("stringtostring")]];

fn stringtostring(String s) {
    return s.txt;
};

struct ssb_header {
    u32 header_size;
    u32 func_table_addr;
    u32 string_pool_ptr; // This is always repeated twice for some reason.
    u32 string_pool_ptr2;
    u32 pad;
    u16 unk1;
    u16 unk2; // Often similar to the first
    u32 unk3;
    u32 pad2;
};

struct ssb_text {
    u32 text1;
    u32 text2;
} [[format_read("formatSSBText")]];

fn decode_ssb_text(u32 encoded_val) {
    const str char_lookup = "\x000123456789abcdefghijklmnopqrstuvwxyz_";
    const u32 ENCODING_BASE = 40;
    const u32 ENCODED_CHAR_COUNT = 6;

    str output = "000000";
    for (s32 i = 0, i < ENCODED_CHAR_COUNT, i+= 1) {
        // Each character is added to the final value, which is then multiplied
        // by the encoding base. To decode, we find the remainder at each power
        // of the base, and use that as an index into a lookup table.
        // In the end this stores 6 characters in 4 bytes.
        const u8 remainder = encoded_val % ENCODING_BASE;
        encoded_val /= ENCODING_BASE;

        // Out-of-bounds values are clamped to the index of the null terminator
        u8 idx = sizeof(char_lookup) - 1;
        if (remainder < idx) {
            idx = remainder;
        }

        // Characters are decoded in reverse order, so we write them back-to-front
        output[ENCODED_CHAR_COUNT - i - 1] = char_lookup[idx];
    }

    return std::format("{}", output);
};

fn formatSSBText(ssb_text txt) {
    str text1 = decode_ssb_text(txt.text1);
    str text2 = decode_ssb_text(txt.text2);

    return std::format("{}{}", text1, text2);
};

struct ssb_functable_entry {
    ssb_text text;
    u32 func_offset;
} [[format_read("formatFuncName")]];

fn formatFuncName(ssb_functable_entry entry) {
    return std::format("{}", entry.text);
};

ssb_header header @ 0x0;
const u32 num_functions = (header.string_pool_ptr - header.func_table_addr) / sizeof(ssb_functable_entry);
const u32 num_instrs = (header.func_table_addr - $) / 4;
u32 instructions[num_instrs] @ $;

ssb_functable_entry functions[num_functions] @ header.func_table_addr;
String text[while (!std::mem::eof())] @ header.string_pool_ptr;