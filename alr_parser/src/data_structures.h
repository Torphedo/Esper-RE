#include <stdint.h>

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
	unsigned int settings; // Appears to indicate what type of data is upcoming
	unsigned int ArraySize1; // The block can have up to 3 arrays of numbered floats
	unsigned int ArraySize2;
	unsigned int ArraySize3;
	unsigned int settings2;
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

typedef struct {
	uint8_t index;
	char unknown[6]; // The 5th byte is often 0x0C or 0x0D, and the 6th byte is always 0xFF.
}anim_array_type4;

// The header of an 0x10 ALR block.
typedef struct {
	unsigned int size;
	unsigned int image_array_size1;
	unsigned int image_array_size2;
}texture_block_header;

// A block with just enough information to reconstruct a TGA header and attach pixel data to it.
typedef struct {
	unsigned int index;
	char filename[32];
	char padding[8];
	float unknown[2]; // This always seems to be 1.0f
	short width; // This was probably written as an int, but the TGA spec uses shorts here, so it should be fine.
	char pad[2];
	short height;
	char pad2[2];
}texture_header;

// Unknown array
typedef struct {
	unsigned int flags;
	unsigned int unknown; // In my old documentation, this is labelled "DataSec_start".
						  // I don't know what this means.
	char pad[4];
	unsigned int unknown2;
	char pad2[4];
	unsigned int ID; // This is the same in all variations of the same model, and has
					 // has no duplicates in game memory. Search for this to find ALRs in memory.
	unsigned int unknown3;
}block_sub_0x15;

// This block of data is only found once, right after the header. Unlike the rest
// of the data in the main ALR data section, this block is not listed in the header's
// pointer array. This implies that it could be some sort of metadata about the
// rest of the file.
typedef struct {
	unsigned int id; // 15 00 00 00
	unsigned int block_size; // Total size in bytes of this block
	unsigned int array_size;
	block_sub_0x15 data[];
}block_type_0x15;
