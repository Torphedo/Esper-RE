#include "stx.h"
#include <stdbool.h>
#include <stdio.h>

#include <common/logging.h>
#include <common/vfile.h>

#include "wav.h"

stx_block_header stx_block_create(u16 total_num_blocks, u16 idx) {
    const bool is_first_block = (idx == 0);
    const u16 offset = (is_first_block) ? STX_FIRST_OFFSET : sizeof(stx_block_header);
    stx_block_header out = {
        .magic = STX_MAGIC,
        .offset = {
            .next_block = offset,
        },
        .channel_count = 2,
        .build_day = STX_PC_BUILD_DAY,
        .build_month = STX_PC_BUILD_MONTH,
        .build_year = STX_PC_BUILD_YEAR,
        .block_count = total_num_blocks,
        .block_idx = idx,
        // In vanilla files, the first block has this set to 0.
        .channel_size = STX_TOTAL_BLOCK_SAMPLES,
        .block_number = (u16)(idx + 1),
        .loop_start_block = 1,
        .loop_end_block = (u16) MAX(0, total_num_blocks - 1),
    };

    return out;
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

void ma_stx_next_block(stx_reader* p) {
    p->audio_sample_idx = 0;
    p->audio_block_idx++;

    // Loop back to the start when we hit the end of the loop or end of the file
    if (p->audio_block_idx >= p->header.header.loop_end_block) {
        p->audio_block_idx = p->header.header.loop_start_block;
    }

    if (p->audio_block_idx >= p->header.header.block_count) {
        p->audio_block_idx = 0;
    }
    p->audio = p->blocks[p->audio_block_idx]; // Load next audio block

    const void* deinterleaved_channels[2] = {
        p->blocks[p->audio_block_idx].samples,
        p->blocks[p->audio_block_idx].samples + STX_BLOCK_SAMPLES,
    };

    interleave_samples(deinterleaved_channels, ARRAY_SIZE(deinterleaved_channels), p->audio.samples, STX_BLOCK_SAMPLES, 2);
}

void stx_read_samples(stx_reader* player, u32 frameCount, void* samples_out) {
    s64 frames_remaining = frameCount;
    while (frames_remaining > 0) {
        const s32 block_frames_left = (STX_TOTAL_BLOCK_SAMPLES - player->audio_sample_idx) / 2;
        const s32 frames_to_read = MIN(frames_remaining, block_frames_left);
        const s16* samples  = player->audio.samples + player->audio_sample_idx;
        const s64 size_to_read = frames_to_read * player->channels * sizeof(*samples);

        memcpy(samples_out, samples, size_to_read);

        frames_remaining -= frames_to_read;
        samples_out = (void*)((uintptr_t)samples_out + size_to_read);
        player->audio_sample_idx += frames_to_read * player->channels;

        if (player->audio_sample_idx >= ARRAY_SIZE(player->audio.samples)) {
            ma_stx_next_block(player);
        }
    }
}

stx_reader stx_reader_init(void* data, u32 size) {
    stx_reader out = {
        .blocks = (stx_audio_block*)data,
        .size = size,
        .channels = 2,
        .initialized = true,
    };
    out.header = *(stx_first_block*)out.blocks;
    return out;
}
