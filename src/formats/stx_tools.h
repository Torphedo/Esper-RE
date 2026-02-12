#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <common/int.h>

/// @brief Generate an STX from a supported audio file
///
/// The supported file formats are WAV, MP3, and FLAC.
/// @param inpath Path of supported file
/// @param stx_path Path to save the STX
/// @return Whether the generation succeeded.
bool generate_stx_from_file(const char* inpath, const char* stx_path);

#ifdef __cplusplus
}
#endif
