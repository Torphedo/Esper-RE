#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <common/int.h>

/// @brief Generate an STX from a supported audio file
///
/// The supported file formats are WAV, MP3, and FLAC.
/// @param data Contents of the input audio file
/// @param size Size of audio file
/// @param stx_buf_out Pointer to receive the generated STX buffer
/// @param stx_size_out Location to receive the size of the generated STX
/// @return Whether the generation succeeded. If this is true, the output pointer is not NULL.
bool generate_stx(const u8* data, s64 size, void** stx_buf_out, u32* stx_size_out);

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
