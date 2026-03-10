struct String {
    char txt[while($[$] != 0)]; // Null terminated string
    char nullterm [[color("000000")]]; // The null terminator
};

struct vec3f {
    float x, y, z;
};

// Offsets in this header are set to -1 if the thing they point to doesn't exist
// in that file. In st00, they're set to 0 instead.
// Some offsets point to things that only exist in st09 area files, and are set
// to -1 or 0 in all other files.
struct st00_t {
    u32 magic [[color("FF0000")]];
    s32 ps00_offset [[color("00FF00")]];
    // Only used in st09 area files. Only set to 1, 2, 3, or 9.
    s32 unk1 [[color("FF0000")]];
    // Points to the byte after the 'PS01' magic (if it exists).
    s32 ps01_offset [[color("FFFF00")]];
    // Only used in st24, where it's set to 0x140. This is a common header size
    // in some other files, but st24 has a header size of -1 for some reason.
    s32 unk2 [[color("FF0000")]];
    // In area files only, points to an ALR-like structure starting with u32(0x20).
    s32 unk_area [[color("FF0000")]];
    // In area files only, points to an 0x9 ALR chunk (followed by an 0x0 chunk).
    s32 unk_area2 [[color("FF0000")]];
    // Only used in st09_02.dat, where it points to an 0x20 chunk.
    s32 unk_area3 [[color("FF0000")]];
    // Only used in st00, where the 2nd value points to the end of the file
    s32 unk3[4] [[color("FF0000")]];
    s32 ps00_count [[color("FFFF00")]];
    // Only used in st00
    s32 unk4 [[color("FF0000")]];
    s32 ps01_count [[color("FFFF00")]];
    // Usually points to the end of the file (== file size), or -1.
    // Has some other unrelated value in st00 and st24.
    s32 unk5 [[color("FF0000")]];
    s32 unk6[7] [[color("FF0000")]];
    s32 nm00_offset [[color("00FFFF")]];
    s32 nm00_count [[color("00FFFF")]];

    // These all seem to be offsets to chunks of data
    s32 OC00_offset [[color("00AF00")]];
    s32 OC01_offset [[color("00AF00")]];
    s32 OC02_offset [[color("00AF00")]];
    s32 OE00_offset [[color("0000FF")]];
    s32 EF00_offset1;
    
    s32 OE02_offset [[color("0000FF")]];
    s32 OA00_offset;
    s32 OA01_offset;
    
    s32 OA02_offset;
    s32 unk7[2];
    
    s32 OB00_offset;
    s32 unk8 [[color("FF0000")]];
    s32 EF00_offset2;
    s32 SP00_offset;
    s32 MD00_offset;
    s32 CP00_offset1 [[color("CF00CF")]];
    s32 CP00_offset2 [[color("CF00CF")]];
    s32 CP00_offset3 [[color("CF00CF")]];
    s32 unk9[2] [[color("FF0000")]];
    s32 CP00_offset4 [[color("CF00CF")]];
    s32 CP00_offset5 [[color("CF00CF")]];
    
    s32 unkA[4];
    s32 ssb_offset;
    s32 unkB[8];
    s32 dynlight_alr_offset;
    s32 footer_offset;
    s32 ssb_end_offset;
    s32 unkC[14];
    u32 filesize;
};

struct ps01_entry {
    u32 object_id [[color("FF0000")]];
    u32 unk2 [[color("FF0000")]];
    u32 pad [[color("000000")]];
    // Object position
    vec3f pos [[color("0000FF")]];
    // Euler rotation in radians
    vec3f rotation [[color("00FF00")]];
};

struct oc00_vertbuf {
    u8 unk1;
    s8 type;
    u8 unk2;
    s8 unk3;
    u32 size;
    padding[4];
    
    u8 buf[size];
};

struct oc00_t {
    u32 magic [[color("FF0000")]];
};

struct oc00_entry {
    u16 unk[4] [[color("FF0000")]];
    vec3f pos [[color("0000FF")]];
};

struct ef00_entry {
    u8 unk1[4];
    float unk2[22];
};

struct ef00_header {
    u32 magic;
    u16 num_entries;
    u16 size;
    ef00_entry entries[num_entries];
};

struct cp00_entry {
    vec3f player_pos;
    float unk;
    float unk2;
    vec3f capsules[3];
    float unk3; // Often NaN (FF FF FF FF)
    u32 unk4;
    u32 unk5;
    u32 unk6;
    u32 unk7;
    float unk8;
};

struct cp00_t {
    u32 magic;
    u16 num_entries;
    u16 array_size;
    cp00_entry entries[num_entries];
};

st00_t header @ 0x0;

ps01_entry ps00_entries[header.ps00_count] @ header.ps00_offset;
ps01_entry ps01_entries[header.ps01_count] @ header.ps01_offset;
String names[header.nm00_count] @ header.nm00_offset;

oc00_t oc00 @ header.OC00_offset - 4;
oc00_vertbuf vertbuf00[while ($ < header.OC01_offset - 4)] @ $;

oc00_t oc01 @ header.OC01_offset - 4;
oc00_vertbuf vertbuf01[while ($ < header.OC02_offset - 4)] @ $;

oc00_t oc02 @ header.OC02_offset - 4;
// oc00_entry oc02_entries[oc02.num_entries] @ $;

ef00_header ef00a @ header.EF00_offset1;
ef00_header ef00b @ header.EF00_offset2;

cp00_t cp00a @ header.CP00_offset1;
cp00_t cp00b @ header.CP00_offset2;
cp00_t cp00c @ header.CP00_offset3;
cp00_t cp00d @ header.CP00_offset4;
cp00_t cp00e @ header.CP00_offset5;
