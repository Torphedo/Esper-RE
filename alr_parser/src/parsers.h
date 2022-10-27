#pragma once
#include <stdio.h>


// Reads only the header and uses it to dump the rest of the ALR chunks to
// separate files on disk.
int dump_chunks(char* alr_filename);


// Parses an ALR 1 block at a time. This will output any readable files it
// can find, such as textures.
void parse_by_block(char* alr_filename);
void texture_description(FILE* alr_stream, unsigned int texture_buffer_ptr);
void dummy_function(FILE* alr, unsigned int texture_buffer_ptr);

// Array of function pointers. When an ALR block is read, it executes a function
// using its ID as an index into this array. This is basically just a super
// efficient switch statement for all blocks.
void (*function_ptrs[23]) (FILE*, unsigned int);
