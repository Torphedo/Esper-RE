#include <stdio.h>
#include <stdlib.h>

#include "data_structures.h"

static const char input_name[] = "bin/pc00a.alr";

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
			int result = realloc(header, sizeof(header_t) + (sizeof(unsigned int) * header->pointer_array_size));
			fread(header->pointer_array, sizeof(unsigned int), header->pointer_array_size, file);

			for (int i = 0; i < header->pointer_array_size; i++)
			{
				printf("%u\n", header->pointer_array[i]);
			}

		}
		else {
			printf("Failed to allocate memory for header!\n");
		}
		fclose(file);
	}
	else {
		printf("Unable to open input file %s!\n", input_name);
		return 1;
	}
	return 0;
}
