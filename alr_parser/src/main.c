#include <stdio.h>
#include <stdlib.h>

#include "data_structures.h"

static const char input_name[] = "bin/pc00a.alr";

int main()
{
	FILE* file = fopen(input_name, "rb");
	if (file != NULL)
	{
		header header;
		header_array pointers;
		// Read header
		fread(&header, sizeof(header), 1, file);
		pointers.pointer_array = malloc(header.pointer_array_size * sizeof(unsigned int));
		if (pointers.pointer_array != NULL)
		{
			fread(pointers.pointer_array, sizeof(unsigned int), header.pointer_array_size, file);

			for (int i = 0; i < header.pointer_array_size; i++)
			{
				// TODO: Jump to each pointer in the array and read data until the next pointer
				// Dumping these larger sections of data as separate files may help RE efforts.
				printf("%u\n", pointers.pointer_array[i]);
			}
		}
		else {
			printf("Failed to allocate for pointer array!\n");
		}
	}

	fclose(file);
	return 0;
}
