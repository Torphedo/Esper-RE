#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <miniaudio.h>
#include <common/int.h>
#include <formats/stx.h>

typedef struct {
    bool initialized;
    const stx_audio_block *blocks;
    u32 size;

    u32 audio_block_idx;
    u32 audio_sample_idx;
    stx_first_block header;
    stx_audio_block audio;

    ma_device device;
} ma_stx_player;

void ma_stx_next_block(ma_stx_player *player);
ma_stx_player ma_stx_init(void *data, u32 size);
bool ma_stx_setup(ma_stx_player *player);
void ma_stx_play(ma_stx_player *player);
bool ma_stx_teardown(ma_stx_player *player);

#ifdef __cplusplus
}
#endif
