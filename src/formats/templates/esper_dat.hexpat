#include <std/io.pat>
#include <std/string.pat>
#include <std/sys.pat>

struct String {
    char txt[while($[$] != 0)]; // Null terminated string
    char nullterm [[color("000000")]]; // The null terminator
};

struct vec3f {
    float x, y, z;
};

fn safeSize(s32 x) {
    return (x < 0) ? 0 : x;
};

fn isPositive(s32 x) {
    return (x < 0) ? 0 : 1;
};

// ALR animation structs copied from ALR template
struct animation_key<auto size> {
    match (size) {
        (3 | 5 | 7):  {u8 frame [[color("7F7F7F")]];}
        (8): {u32 frame [[color("7F7F7F")]];}
        (12 | 16):  {float frame [[color("7F7F7F")]];}
    } 
    
    match (size) {
        (3):  {u16 x;}
        (5):  {u16 x, y;}
        (7):  {u16 x, y, z;}
        (8):  {u32 x [[color("007F00")]];}
        (12): {float x, y;}
        (16): {float x, y, z;}
    }
};

// ID 0x5
struct chunk_animation {
    float length;
    u16 joint_idx;
    u16 rotation_key_size [[color("7F0000")]];
    u32 translation_key_count [[color("007F00")]];
    u32 rotation_key_count [[color("00007F")]];
    u32 scale_key_count [[color("000000")]];
    u8 unk1 [[color("000000")]];
    u8 unk2 [[color("000000")]];
    u16 translation_key_size [[color("007F00")]];

    animation_key<translation_key_size> translation_keys[translation_key_count];
    animation_key<rotation_key_size> rotation_keys[rotation_key_count];
    // animation_key<translation_key_size> scale_keys[scale_key_count];
};

struct chunk_header {
    u32 id [[color("000000")]];
    u32 size [[color("000000")]];
};

struct chunk_generic {
    chunk_header header;
    match (header.id) {
        (0x5): chunk_animation animations;
        (0x00 | 0xD): padding[0];
        // Do nothing
        (_): padding[header.size] [[color("FF0000")]];
    }

    if (header.size == 0) {
        break;
    }

    $ = addressof(header) + header.size;
};

struct area_header {
    s32 unk1; // Looks like an offset
    s32 unk2; // Looks like a count
    s32 MD00_offset;

    s32 unk3; // Always -1, so far
    s32 ssb_offset;
    s32 dynlight_alr_offset;
    s32 ssb_offset2;

    s32 unkC[7]; // Always -1, so far
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


// Offsets in this header are set to -1 if the thing they point to doesn't exist
// in that file. In st00, they're set to 0 instead.
// Some offsets point to things that only exist in st09 area files, and are set
// to -1 or 0 in all other files.
struct st00_t {
    s32 ps00_offset [[color("00FF00")]];
    // Only used in st09 area files. Only set to 1, 2, 3, or 9.
    s32 unk1 [[color("FF0000")]];
    // Points to the byte after the 'PS01' magic (if it exists).
    s32 ps01_offset [[color("FFFF00")]];
    // Only used in st24
    s32 ps02_offset [[color("00FF00")]];
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
    // Often also matches the file size for some reason.
    s32 ps02_count [[color("00FF00")]];
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

struct MainHeader {
    u8 magic[4];
    if (magic[0] == 'A') {
        area_header header;
        chunk_generic chunks[while ($ < header.MD00_offset)] @ $;
        if (header.ssb_offset > 0) {
            u32 ssbSize = header.dynlight_alr_offset - header.ssb_offset;
            u8 ssbBuf[ssbSize] @ header.ssb_offset;
        }
        if (header.ssb_offset2 > 0) {
            u32 ssbSize2 = header.filesize - header.ssb_offset2;
            u8 ssbBuf2[ssbSize2] @ header.ssb_offset2;
        }
    }
    else if (magic[0] == 'S') {
        st00_t header;
        ps01_entry ps00_entries[safeSize(header.ps00_count)] @ header.ps00_offset;
        ps01_entry ps01_entries[safeSize(header.ps01_count)] @ header.ps01_offset;

        String names[safeSize(header.nm00_count)] @ header.nm00_offset;

        oc00_vertbuf vertbuf00[while ($ < header.OC01_offset - 4)] @ header.OC00_offset;

        oc00_vertbuf vertbuf01[while ($ < header.OC02_offset - 4)] @ header.OC01_offset;

        // ef00_header ef00a @ header.EF00_offset1;
        ef00_header ef00b @ header.EF00_offset2;

        // We wrap the struct in an array of size 0 or 1. If the offset exists,
        // it's an array of size 1. Otherwise, it's an array of size 0 and isn't placed.
        cp00_t cp00a[isPositive(header.CP00_offset1)] @ header.CP00_offset1;
        cp00_t cp00b[isPositive(header.CP00_offset2)] @ header.CP00_offset2;
        cp00_t cp00c[isPositive(header.CP00_offset3)] @ header.CP00_offset3;
        cp00_t cp00d[isPositive(header.CP00_offset4)] @ header.CP00_offset4;
        cp00_t cp00e[isPositive(header.CP00_offset5)] @ header.CP00_offset5;
    }
};



MainHeader header @ 0x0;