#pragma once
#include <stdbool.h>

#include "arguments.h"

void set_info_mode();

// Writes each distinct section of an ALR to separate files on disk.
bool split_alr(char* alr_filename);

// Parses an ALR 1 block at a time. This will output any readable files it
// can find, such as textures.
bool block_parse_all(char* alr_filename, flags options);
