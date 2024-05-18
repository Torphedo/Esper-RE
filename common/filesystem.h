#pragma once
#include <stdbool.h>

#include "int.h"

u64 filesize(const char* path);

bool file_exists(const char* path);

bool path_is_file(const char* path);
bool path_is_dir(const char* path);
bool path_has_extension(const char* path, const char* extension);

// Create magic values from characters for easier reading
#define MAGIC(a, b, c, d) ((u32)a | ((u32)b << 8) | ((u32)c << 16) | ((u32)d << 24))


bool dir_exists(const char* path);

// Load an entire file into a buffer. Caller is responsible for freeing buffer,
// returns NULL on error.
u8* file_load(const char* path);

bool is_dirsep(char c);

