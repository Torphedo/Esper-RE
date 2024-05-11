#pragma once
#include <stdbool.h>

#include "int.h"

typedef struct {
    char* input_path;
    char* output_path;
    u32 mode;
    bool silent;
}flags;

// Pass in arguments from main(), and it will return the above struct with the
// settings from the user
flags parse_arguments(int argc, char** argv, char* args[], u32 args_count);

