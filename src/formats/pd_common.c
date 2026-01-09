#include "pd_common.h"

#include <stdio.h>

#include <common/vfile.h>
#include <formats/sth2.h>
#include <formats/stx.h>
#include <formats/wav.h>

bool extract_sth2(const u8* data, u32 size, const char* outpath, u32 sample_rate) {
    FILE* f = fopen(outpath, "wb");
    if (!f) {
        LOG_MSG(error, "I couldn't open the WAV output file '%s'\n", outpath);
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

bool dump_stx(const char* out_file, const u8* data, u32 size) {
    FILE* f = fopen(out_file, "wb");
    if (!f) {
        return false;
    }

    vfile vf = vfile_open((u8*)data, size);

    const stx_first_block* header = VFILE_READ_PTR(stx_first_block, &vf);
    const u16 sample_rate = header->channels[0].sample_rate / 2;
    wav_write_headers(sample_rate, 2, sizeof(u16), WAV_FMT_PCM, 0, f);

    u32 audio_size = 0;
    for (u32 i = 0; i < 2; i++) {
        audio_size = 0;
        vf.pos = header->header.offset.start;

        for (u32 j = 0; j < header->header.block_count - 1; j++) {
            const stx_block_header* block = VFILE_READ_PTR(stx_block_header, &vf);
            u16 channel_size = block->channel_size;
            channel_size = 1008;

            const u16 block_size = channel_size * block->channel_count;
            const u64 next_block = vf.pos + block_size;
            audio_size += block_size;

            if (block->magic != STX_MAGIC) {
                LOG_MSG(warning, "Invalid block magic %X @ 0x%X!\n", block->magic, vf.pos - sizeof(*block));
            }
            if (block->channel_size != 1008) {
                LOG_MSG(warning, "Unexpected channel size %d @ 0x%X\n", block->channel_size, vf.pos - sizeof(*block));
            }
            if (block->channel_count != 2) {
                LOG_MSG(warning, "Unexpected channel count %d @ 0x%X\n", block->channel_count, vf.pos - sizeof(*block));
            }

            // Skip to the appropriate channel & save samples
            vfile_seek(&vf, channel_size * i);
            const u16* samples = (const u16*)vfile_cur(vf);
            fwrite(samples, channel_size, 1, f);

            vf.pos = next_block;
        }
    }

    // Update WAV sizes
    fseek(f, 0, SEEK_SET);
    wav_write_headers(sample_rate, 2, sizeof(u16), WAV_FMT_PCM, audio_size, f);

    fclose(f);
    return true;
}
