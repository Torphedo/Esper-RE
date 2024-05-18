#pragma once
#include "alr_interface.h"

// Interface to call parse_alr() with to trigger texture replacement
const alr_interface replace_interface = {
    .chunk_handlers = {0},
    .tex_handler = replace_texture
};

