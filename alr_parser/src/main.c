#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_structures.h"
#include "parsers.h"

// Using a macro so it can be easily iterated over
#define M_OPERATION_COUNT 2

void (*operation_funcs[M_OPERATION_COUNT]) (char*) = {
	split_alr,
	parse_by_block
};
static const char* operations[M_OPERATION_COUNT] = { "--split", "--dump" };

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
					(*operation_funcs[i]) (input_name);
				}
			}
		}
		(*operation_funcs[0]) (input_name);
	}
	else {
		printf("Please provide an input filename.\n");
		system("pause"); // Windows-only and should be replaced.
		exit(1);
	}

	return 0;
}
