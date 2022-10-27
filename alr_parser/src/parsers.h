#pragma once
#include <stdio.h>


// Reads only the header and uses it to dump the rest of the ALR chunks to
// separate files on disk.
int split_alr(char* alr_filename);


// Parses an ALR 1 block at a time. This will output any readable files it
// can find, such as textures.
void parse_by_block(char* alr_filename);
void texture_description(FILE* alr_stream, unsigned int texture_buffer_ptr);
void generic_skip_block(FILE* alr, unsigned int texture_buffer_ptr);
