#include "sth2.h"
#include <stdbool.h>
#include <stdio.h>

#include <common/vfile.h>

#include <formats/wav.h>

bool extract_sth2(const u8* data, u32 size, const char* outpath, u32 sample_rate) {
    FILE* f = fopen(outpath, "wb");
    if (!f) {
        printf("I couldn't open the WAV output file '%s'\n", outpath);
        return false;
    }

    vfile vf = vfile_open((u8*)data, size);
    sth2_header header = VFILE_READ(sth2_header, &vf);

    vf.pos = header.wave_offset;
    const sth2_wave_header pd_wave_header = VFILE_READ(sth2_wave_header, &vf);
    const u32 offsets_size = pd_wave_header.num_offsets * sizeof(*pd_wave_header.offsets);
    vfile_seek(&vf, offsets_size);

    const u32 header_size = sizeof(pd_wave_header) + offsets_size;
    const u32 audio_size = pd_wave_header.size - header_size;
    const u8* audio = vfile_cur(vf);

    wav_write_audio(sample_rate, 1, sizeof(u16), WAV_FMT_PCM, audio, audio_size, f);
    fclose(f);

    return true;
}
