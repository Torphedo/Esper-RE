#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/// @brief Generate an STX from a miniaudio-supported audio file
///
/// The supported file formats are WAV, MP3, FLAC, MOD, XM, and S3M.
/// @param inpath Path of supported file
/// @param stx_path Path to save the STX
/// @return Whether the generation succeeded.
bool ma_generate_stx(const char* inpath, const char* stx_path);

#ifdef __cplusplus
}
#endif
