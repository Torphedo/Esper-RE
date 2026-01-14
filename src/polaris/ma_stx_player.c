#include "ma_stx_player.h"
#include <common/logging.h>
#include <formats/stx_tools.h>

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    ma_stx_player* player = (ma_stx_player*)pDevice->pUserData;

    s64 frames_remaining = frameCount;
    while (frames_remaining > 0) {
        const s32 block_frames_left = (STX_TOTAL_BLOCK_SAMPLES - player->audio_sample_idx) / 2;
        const s32 frames_to_read = MIN(frames_remaining, block_frames_left);
        const s16* samples  = player->audio.samples + player->audio_sample_idx;
        const s64 size_to_read = frames_to_read * 2 * sizeof(*samples);

        ma_copy_pcm_frames(pOutput, samples, frames_to_read, ma_format_s16, 2);

        frames_remaining -= frames_to_read;
        pOutput = (void*)((uintptr_t)pOutput + size_to_read);
        player->audio_sample_idx += frames_to_read * 2;

        if (player->audio_sample_idx >= ARRAY_SIZE(player->audio.samples)) {
            ma_stx_next_block(player);
        }
    }
}

void ma_stx_next_block(ma_stx_player* p) {
    p->audio_sample_idx = 0;
    p->audio_block_idx++;
    if (p->audio_block_idx >= p->header.header.loop_end_block) {
        p->audio_block_idx = p->header.header.loop_start_block;
    }

    if (p->audio_block_idx >= p->header.header.block_count) {
        p->audio_block_idx = 0;
    }
    p->audio = p->blocks[p->audio_block_idx];

    const void* deinterleaved_channels[2] = {
        p->blocks[p->audio_block_idx].samples,
        p->blocks[p->audio_block_idx].samples + STX_BLOCK_SAMPLES,
    };

    ma_interleave_pcm_frames(ma_format_s16, 2, STX_BLOCK_SAMPLES, deinterleaved_channels, p->audio.samples);
}

ma_stx_player ma_stx_init(void* data, u32 size) {
    ma_stx_player out = {
        .blocks = (stx_audio_block*)data,
        .size = size,
    };
    out.header = *(stx_first_block*)out.blocks;
    ma_stx_next_block(&out);
    return out;
}
bool ma_stx_setup(ma_stx_player* player) {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_s16;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = 2;               // Set to 0 to use the device's native channel count.
    config.sampleRate        = (1 + player->header.channels[0].sample_rate);
    config.dataCallback      = data_callback;   // This function will be called when miniaudio needs more data.
    config.pUserData         = player;   // Can be accessed from the device object (device.pUserData).

    if (ma_device_init(NULL, &config, &player->device) != MA_SUCCESS) {
        return false;
    }
    player->initialized = true;
    ma_stx_next_block(player);

    return true;
}

void ma_stx_play(ma_stx_player* player) {
    if (!player->initialized) {
        LOG_MSG(warning, "Can't play STX because player isn't initialized!\n");
        return;
    }

    ma_device_start(&player->device);
}

bool ma_stx_teardown(ma_stx_player* player) {
    ma_device_uninit(&player->device);
    memset(player, 0, sizeof(*player));

    return true;
}
