#include "wav.h"

void wav_write_audio(u32 sample_rate, u8 channels, u16 sample_size, u16 format, const void* audio, u32 audio_size, FILE* outfile) {
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
    fwrite(audio, audio_size, 1, outfile);
}

