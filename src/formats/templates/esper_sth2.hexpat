struct sth2_header {
    char magic[4];
    u32 size;
    u32 unk1;
    u32 unk2;
    
    padding[16];
    u32 unk3;
    u32 unk4;
    u32 wave_offset;
    
    u32 unk5[5];
};

struct wave_header {
    char magic[4];
    u32 size;
    u32 unk1;
    u32 unk2;
};

sth2_header header @ 0x0;

wave_header wave @ header.wave_offset;
