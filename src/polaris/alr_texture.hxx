#pragma once

#include <common/image.h>
#include <formats/alr.h>

/// @brief Gets a human-readable description of the pixel format
const char* texformat_str(alr_pixel_format format);

/// @brief Convert an ALR texture entry into our standard structure
texture convert_tex(u8* resbuf, texture_entry entry);

/// @brief Update an OpenGL texture to render a texture on the CPU.
///
/// This will re-upload the entire texture to the GPU, even if only the data
/// format or dimensions changed (but not the texture buffer).
void update_gl_tex(texture img, gl_obj texture_id);