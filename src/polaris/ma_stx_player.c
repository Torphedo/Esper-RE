#include "ma_stx_player.h"
#include <string.h>
#include <formats/stx_tools.h>

void ma_stx_next_block(ma_stx_player* p) {
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

void ma_stx_read_samples(ma_stx_player* player, u32 frameCount, void* samples_out) {
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

ma_stx_player ma_stx_init(void* data, u32 size) {
    ma_stx_player out = {
        .blocks = (stx_audio_block*)data,
        .size = size,
        .channels = 2,
        .initialized = true,
    };
    out.header = *(stx_first_block*)out.blocks;
    return out;
}