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
    u32 unk1;
    padding[8];
    u32 unk2;
    u32 unk3;
    u32 unk4;
    u32 unk5;
    u32 unk6; // Usually 0
    u32 unk7; // Usually 1
    u32 sample_rate;
    u32 unk8; // Usually 1
    u32 unk9; // Usually 0x10
    
    s32 unkA;
    s32 unkB;
    
    u32 clip_size; // Size in bytes
    u32 unkD[44];
    u32 clip_idx;
    u32 unkE;
    padding[8];
};

struct trat_evnt {
    trat_header trat;
    evnt_header evnt[trat.num_entries];
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

    trat_evnt entries[num_entries];
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