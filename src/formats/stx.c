#include "stx.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

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

    const stx_audio_block* audio_blocks = (stx_audio_block*)(data + STX_FIRST_OFFSET);

    const stx_first_block* header = (stx_first_block*)data;
    const u16 sample_rate = header->channels[0].sample_rate / 2;
    wav_write_headers(sample_rate, 2, sizeof(s16), WAV_FMT_PCM, 0, f);

    u32 audio_size = 0;
    for (u32 channelIdx = 0; channelIdx < 2; channelIdx++) {
        audio_size = 0;
        for (u32 j = 0; j < header->header.block_count - 1; j++) {
            const stx_audio_block* block = &audio_blocks[j];
            const u16 channel_size = STX_BLOCK_SAMPLES;
            const u16 block_size = channel_size * sizeof(s16) * block->header.channel_count;
            audio_size += block_size;
            const u32 offset = STX_FIRST_OFFSET + (sizeof(*block) * j);

            if (block->header.magic != STX_MAGIC) {
                printf("%s(): Invalid block magic %X @ 0x%X!\n", __func__, block->header.magic, offset);
            }
            if (block->header.channel_count != 2) {
                printf("%s(): Unexpected channel count %d @ 0x%X\n", __func__, block->header.channel_count, offset);
            }

            // Skip to the appropriate channel & save samples
            const s16* samples = &block->samples[channel_size * channelIdx];
            fwrite(samples, channel_size * sizeof(*samples), 1, f);
        }
    }

    // Update WAV sizes
    fseek(f, 0, SEEK_SET);
    wav_write_headers(sample_rate, 2, sizeof(u16), WAV_FMT_PCM, audio_size, f);

    fclose(f);
    return true;
}

bool generate_stx(audio_source_cb read_samples, void* ctx, u64 sample_count, void** stx_buf_out, u32* stx_size_out) {
    sample_count *= 2;

    const s64 stx_size = stx_size_from_sample_count(sample_count);
    u8* stx_data = calloc(1, stx_size);
    if (!stx_data) {
        printf("%s(): Failed to allocate %d bytes to generate STX\n", __func__, stx_size);
        return false;
    }

    stx_first_block* header = (stx_first_block*)stx_data;

    // Write header
    const u32 num_blocks = stx_num_blocks_from_samples(sample_count);
    header->header = stx_block_create(num_blocks, 0);

    // Write channel metadata
    const u16 sample_rate = STX_PC_SAMPLE_RATE;
    for (u32 i = 0; i < STX_MAX_CHANNELS; i++) {
        stx_channel* channel = &header->channels[i];
        channel->sample_rate = sample_rate;
        if (i < 2) {
            channel->volume = 0x7F;
            channel->pan[i] = 0x7F;
        }
    }

    // Write channel names
    strcpy(header->channel_names[0], "left");
    strcpy(header->channel_names[1], "right");

    // Decode & de-interleave samples
    stx_audio_block* audio_blocks = (stx_audio_block*)(stx_data);
    for (u32 i = 1; i < num_blocks; i++) {
        stx_audio_block* block = &audio_blocks[i];
        block->header = stx_block_create(num_blocks, i);

        (read_samples)(ctx, block->samples, STX_BLOCK_SAMPLES);
        deinterleave_samples(block->samples, STX_TOTAL_BLOCK_SAMPLES * sizeof(u16), sizeof(u16));
    }

    end:
    *stx_buf_out = stx_data;
    *stx_size_out = stx_size;
    return true;
}


void stx_reader_next_block(stx_reader* p) {
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
            stx_reader_next_block(player);
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
