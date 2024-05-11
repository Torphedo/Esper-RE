#pragma once
#include <stdbool.h>

#include "int.h"

u64 filesize(const char* path);

bool file_exists(const char* path);

bool dir_exists(const char* path);

// Load an entire file into a buffer. Caller is responsible for freeing buffer,
// returns NULL on error.
u8* file_load(const char* path);

bool is_dirsep(char c);

