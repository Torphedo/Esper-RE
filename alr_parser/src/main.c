#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_structures.h"
#include "parsers.h"

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

	parse_by_block(input_name);
	return 0;
}
