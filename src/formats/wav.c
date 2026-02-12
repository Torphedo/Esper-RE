#include "wav.h"
#include <stdlib.h>

void wav_write_headers(u32 sample_rate, u8 channels, u16 sample_size, u16 format, u32 audio_size, FILE* outfile) {
    const wav_header header = {
        .magic = RIFF_MAGIC,
        .size = sizeof(wav_header) + sizeof(wav_fmt_header) + audio_size,
        .wave_tag = WAVE_MAGIC,
    };

    const wav_fmt_header format_header = {
        .fmt_magic = WAV_FMT_MAGIC,
        .chunk_size = 0x10,
        .sample_format = format,
        .channels = channels,
        .samples_per_second = sample_rate,
        .bytes_per_second = sample_rate * sample_size,
        .block_align = sample_size,
        .bits_per_sample = sample_size * 8,
        .sample_chunk_id = WAV_DATA_MAGIC,
        .sample_chunk_size = audio_size,
    };


    fwrite(&header, sizeof(header), 1, outfile);
    fwrite(&format_header, sizeof(format_header), 1, outfile);
}

void wav_write_audio(u32 sample_rate, u8 channels, u16 sample_size, u16 format, const void* audio, u32 audio_size, FILE* outfile) {
    wav_write_headers(sample_rate, channels, sample_size, format, audio_size, outfile);
    fwrite(audio, audio_size, 1, outfile);
}

bool deinterleave_samples(void* samples, u64 buf_size, u8 sample_size) {
    const u8 channels = 2;
    const u64 num_samples = buf_size / sample_size;
    const u64 channel_samples = num_samples / channels;
    const u64 channel_size = buf_size / channels;

    // We copy the interleaved channel 2 samples here
    void* temp = malloc(channel_size);
    if (!temp) {
        return false;
    }

    {
        uintptr_t source = (uintptr_t)samples;
        uintptr_t target = (uintptr_t)temp;

        source += sample_size; // Start on channel 2 sample

        // Copy every other sample (all the channel 2 samples) into the temp buffer contiguously
        for (u64 i = 0; i < channel_samples; i++, source += sample_size * 2, target += sample_size) {
            memcpy((void*)target, (void*)source, sample_size);
        }
    }

    {
        uintptr_t source = (uintptr_t)samples;
        uintptr_t target = source;

        // Copy every other sample backwards, making the channel 1 samples continguous.
        for (u64 i = 0; i < channel_samples; i++, source += sample_size * 2, target += sample_size) {
            memcpy((void*)target, (void*)source, sample_size);
        }

    }

    // Copy the (now tightly packed) channel 2 samples to the 2nd half of the original buffer.
    const uintptr_t channel2 = (uintptr_t)samples + channel_size;
    memcpy((void*)channel2, temp, channel_size);
    free(temp);

    return true;
}

void interleave_samples(const void* const* channels, u8 num_channels, void* output, u64 num_samples, u8 sample_size) {
    u8* out = output;

    for (u64 i = 0; i < num_samples * sample_size; i += sample_size) {
        for (u8 j = 0; j < num_channels; j++) {
            const u8* channel = channels[j];
            channel += i; // Skip to current sample

            // Copy sample and advance
            memcpy(out, channel, sample_size);
            out += sample_size;
        }
    }
}

