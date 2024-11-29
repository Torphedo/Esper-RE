#pragma once

extern "C" {
    #include <common/image.h>
    #include <formats/alr.h>
}

/// @brief Gets a human-readable description of the pixel format
const char* texformat_str(alr_pixel_format format);

/// @brief Convert an ALR texture entry into our standard structure
texture convert_tex(u8* resbuf, resource_entry entry);

gl_obj init_gl_tex(texture img);

void update_gl_tex(texture img, gl_obj texture_id);