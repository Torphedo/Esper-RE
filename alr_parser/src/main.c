#include <stdio.h>
#include <stdlib.h>

struct {
	unsigned int id; // 11 00 00 00
	unsigned int block_size; // Total size in bytes of the file header block
	unsigned int flags;
	unsigned int unknown_section_ptr; // Pointer to unknown section near end of file
	unsigned int pointer_array_size; // Number of elements in the pointer array
	unsigned int unknown;
	unsigned int pad;
	unsigned int* pointer_array;
}header;

int main()
{
	FILE* file = fopen("bin/pc00a.alr", "rb");
	if (file != NULL)
	{
		// Read header into easily addressable format
		fread(&header, sizeof(header), 1, file);
		header.pointer_array = malloc(sizeof(unsigned int) * header.pointer_array_size);
		if (header.pointer_array != NULL)
		{
			// Rewind file pointer so the pointer array can be read to the newly allocated block
			fseek(file, ftell(file) - sizeof(unsigned int*), SEEK_SET);
			fread(header.pointer_array, sizeof(unsigned int), header.pointer_array_size, file);
		}


		fclose(file);
	}
}
