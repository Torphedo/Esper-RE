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
}header;

typedef struct {
	unsigned int* pointer_array; // Array of pointers to other data structures in the file
}header_array;

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