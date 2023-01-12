#include <stdint.h>

#define RGBA8 0x20
#define RGB8  0x18
#define A8    0x08

/* 
   Structs for data structures found in ALR files.
   Because most of the data in these structures has an unknown purpose, blocks
   are referred to by their "ID", which is an unsigned integer at the start of
   each block. For example, a block starting with 00 00 00 11 is known as an
   0x11 block. Each struct uses the naming convention "block_type_0x[#]", and
   block_sub_0x[#] for any sub-structures it contains (such as a dynamically
   sized array of smaller structs).
*/

// ALR header. This is primarily used to store pointers to other
// blocks of data throughout the file (to avoid excessive parsing
// logic at runtime?).
typedef struct {
	unsigned int id; // 11 00 00 00
	unsigned int block_size; // Total size in bytes of the file header block
	unsigned int flags;
	unsigned int unknown_section_ptr; // Pointer to unknown section near end of file
	unsigned int pointer_array_size; // Number of elements in the pointer array
	unsigned int unknown;
	char pad[8];
}header_t;

typedef struct {
	unsigned int* pointer_array; // Array of pointers to other data structures in the file
}header_array;

// The header for an 0x5 ALR block (meshes / animation)
typedef struct {
	unsigned int size;
	float unknown_float; // This often matches the number of frames / vertices
	unsigned short unknown_settings1;
	unsigned short array_width_1; // The number of bytes in each element of the second array
	unsigned int ArraySize1; // The block can have up to 3 arrays of numbered floats
	unsigned int ArraySize2;
	unsigned int ArraySize3; // This might actually be padding, since there's no third settings field.
	unsigned short unknown_settings2;
	unsigned short array_width_2;
}anim_header;

typedef struct {
	float index; // I don't know why they would use floats for indices, but it seems like that's what they did...
	float X;
}anim_array_type1;

typedef struct {
	float index;
	float X;
	float Y;
}anim_array_type2;

typedef struct {
	float index;
	float X;
	float Y;
	float Z;
}anim_array_type3;

// The header of an 0x10 ALR block.
typedef struct {
	unsigned int size;
	unsigned int DDS_count;
	unsigned int texture_count;
}texture_block_header;

// A block with just enough information to reconstruct a TGA header and attach pixel data to it.
typedef struct {
	unsigned int index;
	char filename[32];
	char padding[8];
	float unknown[2]; // This always seems to be 1.0f
	unsigned int width;
	unsigned int height;
}texture_header;

typedef struct {
    uint16_t width;
    uint16_t height;
    uint32_t flags; // Unknown
    uint32_t mipmap_count;
    uint32_t unknown; // Often 4 or 8, sometimes counts up from 13?
    uint32_t pad;
}dds_meta;

typedef struct {
    uint32_t index;
    uint32_t mipmap_count;
    uint32_t offset;
    uint32_t bits_per_pixel;
}texture_meta;

// Unknown array
typedef struct {
	uint32_t flags;    // Unknown, always 01 00 04 00 so far
	uint32_t data_ptr; // A pointer to some data in the buffer at the end of the file
	uint32_t pad;      // Always 0 so far.
    uint32_t unknown;
	uint32_t unknown2;
    uint32_t ID; // This is the same in all variations of the same model, and never duplicates. Search for this to find ALRs in memory.
    uint32_t unknown3;
}block_sub_0x15;

// This block of data is only found once, right after the header. Unlike the rest
// of the data in the main ALR data section, this block is not listed in the header's
// pointer array. This implies that it could be some sort of metadata about the
// rest of the file.
typedef struct {
    uint32_t id; // 15 00 00 00
    uint32_t block_size;
    uint32_t struct_array_size;
}block_0x15_header;
