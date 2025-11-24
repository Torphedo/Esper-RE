#pragma once
#include <stdio.h>

#include <common/int.h>
#include <common/file.h>

#ifdef __cplusplus
extern "C" {
#endif

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

void wav_write_headers(u32 sample_rate, u8 channels, u16 sample_size, u16 format, u32 audio_size, FILE* outfile);
void wav_write_audio(u32 sample_rate, u8 channels, u16 sample_size, u16 format, const void* audio, u32 audio_size, FILE* outfile);

#ifdef __cplusplus
}
#endif
