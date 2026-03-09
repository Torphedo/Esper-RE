#include "sth2.h"
#include <stdbool.h>
#include <stdio.h>

#include "wav.h"

bool extract_sth2(const u8* data, u32 size, const char* outpath, u32 sample_rate) {
    FILE* f = fopen(outpath, "wb");
    if (!f) {
        printf("I couldn't open the WAV output file '%s'\n", outpath);
        return false;
    }

    u32 pos = 0;
    const sth2_header* header = (const sth2_header*)data;
    pos = header->wave_offset;

    const sth2_wave_header* pd_wave_header = (sth2_wave_header*)(data + pos);
    const u32 offsets_size = pd_wave_header->num_offsets * sizeof(*pd_wave_header->offsets);
    pos += offsets_size;

    const u8* audio = (data + pos);
    const u32 header_size = sizeof(*pd_wave_header) + offsets_size;
    const u32 audio_size = pd_wave_header->size - header_size;

    wav_write_audio(sample_rate, 1, sizeof(u16), WAV_FMT_PCM, audio, audio_size, f);
    fclose(f);

    return true;
}
