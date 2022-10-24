// Structs for data structures found in ALR files.


// ALR header. This is primarily used to store pointers to
// other blocks of data throughout the file and avoid excessive
// parsing logic at runtime.
typedef struct {
	unsigned int id; // 11 00 00 00
	unsigned int block_size; // Total size in bytes of the file header block
	unsigned int flags;
	unsigned int unknown_section_ptr; // Pointer to unknown section near end of file
	unsigned int pointer_array_size; // Number of elements in the pointer array
	unsigned int unknown;
	unsigned int pad;
	unsigned int pointer_array[];
}header;
