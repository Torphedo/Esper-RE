#include <common/int.h>
#include <formats/pd_common.h>

// These structures could be out of date. If you use this file as reference for
// your own code, double-check them against the latest information at:
// https://phantomdust.miraheze.org/wiki/File_Formats/SSB

// Data in this file comes mostly from the wiki page, and most of that SSB
// research comes from Vu314. Thanks for the help :)

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

decoded_text decode_text(ssb_functable_entry val);
