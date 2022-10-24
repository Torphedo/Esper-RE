#include <stdio.h>
#include <stdlib.h>

#include "data_structures.h"

int main()
{
	header header;
	FILE* file = fopen("bin/pc00a.alr", "rb");
	if (file != NULL)
	{
		// Read header into easily addressable format
		fread(&header, sizeof(header), 1, file);
		fread(header.pointer_array, sizeof(unsigned int), header.pointer_array_size, file);

		fclose(file);
	}
}
