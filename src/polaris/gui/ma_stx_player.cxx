#include "ma_stx_player.hxx"
#include <common/logging.h>
#include <formats/stx_tools.h>

ma_stx_player::ma_stx_player(const void* data, u32 size)
    : blocks((stx_audio_block*)data), size(size)
{
    header = *(stx_first_block*)blocks;
    next_block();
}

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
        pOutput = (void*)(uintptr_t(pOutput) + size_to_read);
        player->audio_sample_idx += frames_to_read * 2;

        if (player->audio_sample_idx >= ARRAY_SIZE(player->audio.samples)) {
            player->next_block();
        }
    }
}

void ma_stx_player::next_block() {
    audio_sample_idx = 0;
    audio_block_idx++;
    if (audio_block_idx >= header.header.block_count) {
        audio_block_idx = 0;
    }
    if (audio_block_idx >= header.header.loop_end_block) {
        audio_block_idx = header.header.loop_start_block;
    }

    audio = blocks[audio_block_idx];

    const void* deinterleaved_channels[2] = {
        blocks[audio_block_idx].samples,
        blocks[audio_block_idx].samples + STX_BLOCK_SAMPLES,
    };

    ma_interleave_pcm_frames(ma_format_s16, 2, STX_BLOCK_SAMPLES, deinterleaved_channels, audio.samples);
}

bool ma_stx_player::setup() {
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_s16;   // Set to ma_format_unknown to use the device's native format.
    config.playback.channels = 2;               // Set to 0 to use the device's native channel count.
    config.sampleRate        = (1 + header.channels[0].sample_rate);
    config.dataCallback      = data_callback;   // This function will be called when miniaudio needs more data.
    config.pUserData         = this;   // Can be accessed from the device object (device.pUserData).

    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        return false;
    }
    initialized = true;

    return true;
}

void ma_stx_player::play() {
    if (!initialized) {
        LOG_MSG(warning, "Can't play STX because player isn't initialized!\n");
        return;
    }

    ma_device_start(&device);
}

bool ma_stx_player::teardown() {
    ma_device_uninit(&device);
    initialized = false;

    return true;
}
