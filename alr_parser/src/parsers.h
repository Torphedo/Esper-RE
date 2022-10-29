#pragma once
#include <stdio.h>
#include <stdbool.h>


// Writes each distinct section of an ALR to separate files on disk.
bool split_alr(char* alr_filename);

// Parses an ALR 1 block at a time. This will output any readable files it
// can find, such as textures.
bool parse_by_block(char* alr_filename, bool info_mode);
void parse_anim_or_model(FILE* alr, unsigned int texture_buffer_ptr, bool info_mode);
void texture_description(FILE* alr_stream, unsigned int texture_buffer_ptr, bool info_mode);
void skip_block(FILE* alr, unsigned int texture_buffer_ptr, bool info_mode);
