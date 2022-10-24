#include <stdio.h>
#include <stdlib.h>

#include "data_structures.h"

int main()
{
	header_t *header = malloc(sizeof(header_t));
	if (header != NULL)
	{
		FILE* file = fopen("bin/pc00a.alr", "rb");
		if (file != NULL)
		{
			// Read header
			fread(header, sizeof(header_t), 1, file);
			int result = realloc(header, sizeof(header_t) + (sizeof(unsigned int) * header->pointer_array_size));
			fread(header->pointer_array, sizeof(unsigned int), header->pointer_array_size, file);

			for (int i = 0; i < header->pointer_array_size; i++)
			{
				printf("%u\n", header->pointer_array[i]);
			}

			fclose(file);
		}
	}
}
