#pragma once
#include <miniaudio.h>
#include <common/int.h>
#include <formats/stx.h>

struct ma_stx_player {
    bool initialized = false;
    const stx_audio_block* blocks = nullptr;
    u32 size = 0;

    u32 audio_block_idx = 0;
    u32 audio_sample_idx = 0;
    stx_first_block header = {};
    stx_audio_block audio = {};

    ma_device device = {};

    ma_stx_player(const void* data, u32 size);

    void next_block();
    bool setup();
    void play();
    bool teardown();
};
