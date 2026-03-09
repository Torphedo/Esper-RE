#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "data_types.h"

enum {
    WAV_HEADER_SIZE = 0x2C,
    RIFF_MAGIC = MAGIC('R','I','F','F'),
    WAVE_MAGIC = MAGIC('W','A','V','E'),
    WAV_FMT_MAGIC = MAGIC('f','m','t',' '),
    WAV_DATA_MAGIC = MAGIC('d','a','t','a'),

    WAV_FMT_PCM = 1,
    WAV_FMT_FLOAT = 3,
};

typedef struct {
    u32 magic; // 'RIFF'
    u32 size; // Filesize - 8
    u32 wave_tag; // 'WAVE'
}wav_header;

typedef struct {
    u32 fmt_magic; // "fmt "
    u32 chunk_size;
    u16 sample_format; // WAV_FMT_*
    u16 channels;
    u32 samples_per_second;
    u32 bytes_per_second;
    u16 block_align;
    u16 bits_per_sample;
    u32 sample_chunk_id;
    u32 sample_chunk_size;
}wav_fmt_header;

/// @brief Write all WAV metadata to a file
///
/// After this all you have to do is write your audio samples, and you'll have a valid file.
/// @param sample_rate Audio sample rate in Hz
/// @param channels Number of channels
/// @param sample_size Audio sample size in bytes
/// @param format The audio sample format, e.g. WAV_FMT_PCM
/// @param audio_size Size in bytes your audio data will be
/// @param outfile File to write WAV header to
void wav_write_headers(u32 sample_rate, u8 channels, u16 sample_size, u16 format, u32 audio_size, FILE* outfile);

/// @brief Write a complete WAV file
///
/// See @ref wav_write_headers for details on other parameters.
/// @param audio Buffer containing your audio samples
void wav_write_audio(u32 sample_rate, u8 channels, u16 sample_size, u16 format, const void* audio, u32 audio_size, FILE* outfile);

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

#ifdef __cplusplus
}
#endif
