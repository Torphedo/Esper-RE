#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_structures.h"
#include "parsers.h"

// Thin wrapper around --dump to print out more info
bool info_dump(char* filename, bool info_mode)
{
	block_parse_all(filename, 1);
	return true;
}

// Using a macro so it can be easily iterated over
#define M_OPERATION_COUNT 3

// Function pointer array for different command-line options
bool (*operation_funcs[M_OPERATION_COUNT]) (char*, bool) = {
	split_alr,
	block_parse_all,
	info_dump
};
static const char* operations[M_OPERATION_COUNT] = { "--split", "--dump", "--info"};

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
		if (argc > 2) {
			for (unsigned int i = 0; i < M_OPERATION_COUNT; i++)
			{
				if (!(strcmp(argv[2], operations[i])))
				{
					(*operation_funcs[i]) (input_name, false);
				}
			}
		}
		else {
			(*operation_funcs[0]) (input_name, false);
		}
	}
	else {
		printf("Please provide an input filename.\n");
		system("pause"); // Windows-only and should be replaced.
		exit(1);
	}

	return 0;
}
