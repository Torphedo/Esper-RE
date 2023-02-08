#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    bool info_mode: 1;
    bool dump_images: 1;
    bool silent: 1;
    bool split: 1;
    bool dds: 1;
    bool tga: 1;
    bool layout: 1;
    bool animation: 1;
    uint8_t : 0; // Pads out struct to next boundary
    char* filename;
}flags;



// Pass in arguments from main(), and it will return a bitfield based on a bunch of manually specified flags
flags parse_arguments(int argc, char* argv[]);
