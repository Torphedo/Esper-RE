struct sth2_header {
    char magic[4] [[color("0000FF")]];
    u32 size [[color("0000FF")]];
    u32 unk1;
    u32 unk2;

    padding[16];
    u32 unk3;
    u32 real_offset;
    u32 wave_offset;

    u32 unk5[5];
};

sth2_header header @ 0x0;


struct trat_header {
    u32 magic [[color("0000FF")]];
    u32 size [[color("0000FF")]];
    u32 num_entries;
    u32 unk1;
    u32 unk2;
    u32 unk3;
    padding[0x18];
};

struct evnt_header {
    u32 magic;
    u8 unk5[0xFC];
};

struct reat_header {
    char magic[4] [[color("0000FF")]];
    u32 size [[color("0000FF")]];
    u32 num_entries;
    padding[4];
    u32 unk1;
    u32 unk2;
    padding[0x18];
    u32 offsets[num_entries];

    trat_header trat;
    evnt_header evnt;
};

fn real_ptr_rebase(s32 ptr) {
    if (ptr > 0) {
        return header.real_offset;
    } else {
        return 0x70;
    }
};

struct ptr_real_base {
    reat_header* offsets: s32 [[pointer_base("real_ptr_rebase")]];
};

struct real_header {
    char magic[4] [[color("0000FF")]];
    u32 unknown;
    u32 num_offsets;
    padding[4];

    ptr_real_base offsets[num_offsets];
};

struct wave_header {
    char magic[4] [[color("0000FF")]];
    u32 size [[color("0000FF")]];
    u32 num_offsets;
    u32 unk2;
    s32 offsets[num_offsets];
};

real_header real @ header.real_offset;
wave_header wave @ header.wave_offset;
