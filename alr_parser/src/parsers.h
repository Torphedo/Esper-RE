#pragma once
#include <stdio.h>


// Writes each distinct section of an ALR to separate files on disk.
int split_alr(char* alr_filename);

// Parses an ALR 1 block at a time. This will output any readable files it
// can find, such as textures.
void parse_by_block(char* alr_filename, unsigned int info_mode);
void texture_description(FILE* alr_stream, unsigned int texture_buffer_ptr);
void skip_block(FILE* alr, unsigned int texture_buffer_ptr);
