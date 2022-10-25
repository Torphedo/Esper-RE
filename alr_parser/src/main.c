#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_structures.h"

static char* input_name;

int main(int argc, char* argv[])
{
	// Parse command-line arguments.
	if (argc > 1)
	{
		input_name = malloc(strlen(argv[1]));
		if (input_name != NULL) {
			strcpy(input_name, argv[1]);
		}
		else {
			printf("Failed to allocate memory for input filename!\n");
			exit(0);
		}
	}
	else {
		printf("Please provide an input fileame.\n");
		system("pause"); // Windows-only and should be replaced.
		exit(1);
	}


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

			// Write each large block of data to a separate file. 
			// size - 1 is because we can't determine the size of the last block easily.
			for (int i = 0; i < header.pointer_array_size - 1; i++)
			{
				// This is fine because there's no user input and the filename is never > 15 characters
				char filename[15];
				sprintf(filename, "%u.bin", i);
				FILE* dump = fopen(filename, "wb");

				// Length between the current and next pointer
				size_t size = pointers.pointer_array[i + 1] - pointers.pointer_array[i];
				char* buffer = malloc(size);
				if (buffer != NULL) {
					fseek(file, pointers.pointer_array[i], SEEK_SET);
					fread(buffer, size, 1, file);
					fwrite(buffer, size, 1, dump);
					free(buffer);
				}

				fclose(dump);
			}
		}
		else {
			printf("Failed to allocate for pointer array!\n");
		}
		fclose(file);
	}
	return 0;
}
