#pragma once
#include <stdbool.h>

#include "arguments.h"

// Parses an ALR 1 chunk at a time. This will output any readable files it can
// find, such as textures.
bool chunk_parse_all(char* alr_filename, flags options);
