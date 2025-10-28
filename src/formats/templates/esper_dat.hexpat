struct String {
    char txt[while($[$] != 0)];
    char nullterm [[color("000000")]];
};

// Offsets in this header are set to -1 if the thing they point to doesn't exist
// in that file. In st00, they're set to 0 instead.
// Some offsets point to things that only exist in st09 area files, and are set
// to -1 or 0 in all other files.
struct st00_t {
    u32 magic [[color("FF0000")]];
    s32 chunk_size [[color("00FF00")]];
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
    s32 unk7 [[color("FF0000")]];
    s32 unk8[7] [[color("FF0000")]];
    s32 nm00_offset [[color("00FFFF")]];
    s32 string_count [[color("00FFFF")]];

    // These all seem to be offsets to chunks of data
    s32 OC00_offset [[color("00AF00")]];
    s32 OC01_offset [[color("00AF00")]];
    s32 OC02_offset [[color("00AF00")]];
	s32 OE00_offset [[color("0000FF")]];
	s32 EF00_offset;
	
	s32 OE02_offset [[color("0000FF")]];
	s32 OA00_offset;
	s32 OA01_offset;
	
	s32 OA02_offset;
	s32 unk9[2];
	
	s32 OB00_offset;
    s32 unkA[4] [[color("FF0000")]];
    s32 CP00_offset1 [[color("CF00CF")]];
    s32 CP00_offset2 [[color("CF00CF")]];
    s32 CP00_offset3 [[color("CF00CF")]];
    s32 unkB[2] [[color("FF0000")]];
    s32 CP00_offset4 [[color("CF00CF")]];
    s32 CP00_offset5 [[color("CF00CF")]];
    s32 unkC[31] [[color("FF0000")]];
};

struct vec3f {
	float x;
	float y;
	float z;
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

struct oc00_t {
    u32 magic [[color("FF0000")]];
    u8 unk1[4] [[color("FF0000")]];
    u32 size [[color("00FF00")]];
    padding[4];
    float unk_pos[10] [[color("0000FF")]];
    u16 unk2[4] [[color("FF0000")]];
};

struct oc00_entry {
    u16 unk[4] [[color("FF0000")]];
    vec3f pos [[color("0000FF")]];
};

st00_t header @0x0;


ps01_entry ps00_entries[header.ps00_count] @ header.chunk_size;
ps01_entry ps01_entries[header.ps01_count] @ header.ps01_offset;

String names[header.string_count] @ header.nm00_offset [[color("5050FF")]];

oc00_t oc00 @ header.OC00_offset - 4;
oc00_entry oc00_entries[oc00.unk2[0]] @ $;


oc00_t oc01 @ header.OC01_offset - 4;
oc00_entry oc01_entries[oc01.unk2[0]] @ $;


oc00_t oc02 @ header.OC02_offset - 4;
// oc00_entry oc02_entries[oc02.num_entries] @ $;