#include <stdio.h>
#include <stdlib.h>

#include "data_structures.h"

static const char input_name[] = "bin/pc00a.alr";

void alloc_fail_exit(FILE* file_to_close)
{
	fclose(file_to_close);
	printf("Failed to allocate memory for block!\n");
	return 1;
}

int main()
{
	FILE* file = fopen(input_name, "rb");
	if (file != NULL)
	{
		header_t* header = malloc(sizeof(header_t));
		if (header != NULL)
		{
			// Read header
			fread(header, sizeof(header_t), 1, file);
			if (realloc(header, sizeof(header_t) + sizeof(unsigned int) * header->pointer_array_size) == NULL)
			{
				alloc_fail_exit(file);
			}
			fread(header->pointer_array, sizeof(unsigned int), header->pointer_array_size, file);

			for (int i = 0; i < header->pointer_array_size; i++)
			{
				// TODO: Jump to each pointer in the array and read data until the next pointer
				// Dumping these larger sections of data as separate files may help RE efforts.
				printf("%u\n", header->pointer_array[i]);
			}
		}
		else {
			alloc_fail_exit(file);
		}
		block_type_0x15* block_0x15 = malloc(sizeof(block_type_0x15));
		if (block_0x15 != NULL)
		{
			// Read 0x15 block that comes after the header (metadata?)
			fread(block_0x15, sizeof(block_type_0x15), 1, file);
			if (realloc(block_0x15, sizeof(block_type_0x15) + sizeof(block_sub_0x15) * block_0x15->array_size) == NULL)
			{
				alloc_fail_exit(file);
			}
			fread(block_0x15->data, sizeof(block_sub_0x15), block_0x15->array_size, file);
		}
		else {
			alloc_fail_exit(file);
		}
		fclose(file);
	}
	else {
		printf("Unable to open input file %s!\n", input_name);
		return 1;
	}
	return 0;
}
