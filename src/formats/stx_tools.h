#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <common/int.h>

/// @brief De-interleave an interleaved audio buffer
///
/// Afterwards, the buffer will contain all the samples for the first channel,
/// followed by all samples for the second channel. This function assumes only 2
/// channels.
/// @param samples Buffer containing interleaved samples
/// @param buf_size Size of the buffer
/// @param sample_size Size of each sample
bool deinterleave_samples(void* samples, u64 buf_size, u8 sample_size);

/// @brief Interleave samples from several audio channels
///
/// @param channels An array of pointers to each of your separate channels
/// @param num_channels The number of channel pointers in the array
/// @param output The buffer to output interleaved samples to
/// @param num_samples The number of samples to be copied *from each channel*.
///                    This means the total size copied will be
///                    @ref num_channels * @ref num_samples * @ref sample_size.
/// @param sample_size The size in bytes of each audio sample
/// @return
void interleave_samples(const void* const* channels, u8 num_channels, void* output, u64 num_samples, u8 sample_size);

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
