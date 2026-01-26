#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <common/int.h>

// These structures could be out of date. If you use this file as reference for
// your own code, double-check them against the latest information at:
// https://phantomdust.miraheze.org/wiki/File_Formats/SSB

// Data in this file comes mostly from the wiki page, and most of that SSB
// research comes from Vu. Thanks for the help :)

typedef struct {
    u32 magic; // 20 00 00 00 
    u32 func_table_addr;
    u32 text_addr;  // String array offset, which is always repeated twice for
    u32 text_addr2; // some reason.
    u32 pad;
    u16 unk_bitmask;
    u16 unk_bitmask2; // Often similar to the first
    u32 unk;
    u32 pad2;
}ssb_header;

enum {
    SSB_STACK_SIZE = 0x200,
    SSB_REG_COUNT = 4,
};

typedef enum {
    NOP = 0x00,
    FUNC_TABLE_CALL = 0x01,
    THROW = 0x02,
    CALL = 0x03,
    UNK_NATIVE = 0x04,
    RETURN = 0x05,
    STACK_PUSH = 0x06,
    COND_JMP = 0x07,
    COND_JMP_POP = 0x08,
    IDX_PUSH = 0x09,
}ssb_opcode;

typedef struct {
    u32 text1;
    u32 text2;
    u32 func_offset;
}ssb_functable_entry;

enum {
    // Number of characters encoded in a u32
    ENCODED_CHAR_COUNT = 6,
};

// Useful when decoding larger pieces of text
typedef struct {
    char data[ENCODED_CHAR_COUNT * 2];
    char null_terminator;
}decoded_text;


/// @brief Decode 6 characters from a 32-bit integer.
/// @param output buffer to store decoded characters in
/// @param val value to decode
void decode_single32(char* output, u32 val);

decoded_text decode_double(u32 text1, u32 text2);

/// @brief Encode 6 characters into a 32-bit integer.
///
/// The only allowed characters are alphanumeric ASCII, spaces and underscores,
/// and NUL. Periods and dashes will be converted to underscores.
/// @param text string to encode (returns 0 if NULL)
///
/// @return encoded value representing the input string
u32 encode_single32(char* text);

#ifdef __cplusplus
}
#endif