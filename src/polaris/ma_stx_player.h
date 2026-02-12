#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <common/int.h>
#include <formats/stx.h>

typedef struct {
    bool initialized;
    const stx_audio_block *blocks;
    u32 size;
    u8 channels;

    u32 audio_block_idx;  // The current STX block
    u32 audio_sample_idx; // The current sample position within the block

    // A copy of the first block, used to quickly check sample rate, etc.
    stx_first_block header;

    // A copy of a single block of audio, this is what we read samples from
    stx_audio_block audio;
}ma_stx_player;

/// @brief Set up the STX reader with already-loaded data
/// @param data The STX data
/// @param size The size of the STX data
/// @return STX reader context
ma_stx_player ma_stx_init(void *data, u32 size);

/// @brief Read 16-bit interleaved stereo samples from the STX
/// @param player The STX reader context
/// @param frameCount The number of frames (samples per channel to read)
/// @param samples_out Buffer to read samples into
void ma_stx_read_samples(ma_stx_player* player, u32 frameCount, void* samples_out);

#ifdef __cplusplus
}
#endif
