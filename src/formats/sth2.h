#pragma once
#include <common/int.h>

typedef struct {
    u32 magic;
    u32 size;
    u32 unk1;
    u32 unk2;

    u8 pad[16];
    u32 unk3;
    u32 unk4;
    u32 wave_offset;

    u32 unk5[5];
}sth2_header;

typedef struct {
    u32 magic;
    u32 size;
    u32 num_offsets;
    u32 unk2;
    u32 offsets[];
}sth2_wave_header;

enum {
    PD_SAMPLE_RATE_UWP = 22050,
    WAV_HEADER_SIZE = 0x2C,
};

typedef struct {
    u32 magic;
    u32 size;
}wav_header;

typedef struct {
    u32 magic; // 'WAVE'
    u32 fmt_magic; // "fmt "
    u32 chunk_size;
    u16 sample_format;
    u16 channels;
    u32 samples_per_second;
    u32 bytes_per_second;
    u16 block_align;
    u16 bits_per_sample;
    u32 sample_chunk_id;
    u32 sample_chunk_size;
}wav_fmt_header;

static wav_header wav_header_default(u32 size) {
    const wav_header out = {MAGIC('R', 'I', 'F', 'F'), WAV_HEADER_SIZE + size};
    return out;
}

static wav_fmt_header pd_wav_format_header(u32 audio_size) {
    wav_fmt_header out = {
        .magic = MAGIC('W', 'A', 'V', 'E'),
        .fmt_magic = MAGIC('f', 'm', 't', ' '),
        .chunk_size = 0x10,
        .sample_format = 1,
        .channels = 1,
        .samples_per_second = PD_SAMPLE_RATE_UWP,
        .bytes_per_second = PD_SAMPLE_RATE_UWP * sizeof(u16),
        .block_align = 2,
        .bits_per_sample = sizeof(u16) * 8,
        .sample_chunk_id = MAGIC('d', 'a', 't', 'a'),
        .sample_chunk_size = audio_size,
    };

    return out;
}