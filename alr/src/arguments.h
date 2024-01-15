#pragma once
#include <stdbool.h>

typedef enum {
    info_only,
    replacetex,
    dumptex,
    split,
    animation, // Unused at the moment, should be added back soon.
}program_mode;

typedef struct {
    char* filename;
    program_mode mode;
    bool silent;
}flags;

// Pass in arguments from main(), and it will return the above struct with the
// settings from the user
flags parse_arguments(int argc, char** argv);

