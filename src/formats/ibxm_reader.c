#include "ibxm_reader.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/// Return the larger of 2 values
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/// Return the smaller of 2 values
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/// Returns low or high bound if @p val is out of bounds, otherwise return @p val
#define CLAMP(low, val, high) (((val) < (low)) ? (low) : MIN((val), (high)))

uint32_t ibxm_reader_decode_internal(ibxm_reader* ctx) {
    if (!ctx->initialized) {
        printf("%s(): IBXM reader not initialized, skipping!\n", __func__);
        return 0;
    }

    const uint32_t samples_read = replay_get_audio(ctx->replay, ctx->mixbuf, 0) * 2;
    int16_t* converted = (int16_t*)ctx->mixbuf;

    switch (ctx->sample_size) {
    case sizeof(int16_t):
        // For some reason IBXM outputs 32-bit samples, but with mostly 16-bit
        // magnitudes. We can just copy them as 16-bit samples.
        for (uint32_t i = 0; i < samples_read; i++) {
            // Casting to 16-bit will cause out-of-bounds 32-bit values to flip
            // sign, which causes unpleasant popping. Just clip it.
            // This seems sketchy, but it's what IBXM does in their official
            // example "xm2wav.c".
            const int16_t sample = CLAMP(INT16_MIN, ctx->mixbuf[i], INT16_MAX);
            *converted = sample;
            converted++;
        }
        break;
    case sizeof(int32_t):
        break; // No conversion needed
    default:
        printf("%s(): Unknown target sample size (%d bytes)!\n", __func__, ctx->sample_size);
        break;
    }

    ctx->mixbuf_read_pos = 0;
    ctx->mixbuf_usable = samples_read * ctx->sample_size;
    return samples_read;
}

ibxm_reader ibxm_reader_create(const void* module_data, uint32_t data_size, uint32_t sample_rate, uint8_t sample_size) {
    struct data data = {module_data, data_size};
    ibxm_reader out = {
        .sample_size = sample_size,
        .sample_rate = sample_rate,
    };

    char message[128] = {0};
    out.module = module_load(&data, message);
    if (!out.module) {
        goto module_create_error;
    }

    out.replay = new_replay(out.module, sample_rate, 0);
    if (!out.replay) {
        goto replay_create_error;
    }

    out.length_samples = replay_calculate_duration(out.replay);

    out.mixbuf_size = calculate_mix_buf_len(sample_rate) * sizeof(int32_t);
    out.mixbuf = calloc(1, out.mixbuf_size);
    if (!out.mixbuf) {
        goto error;
    }
    out.initialized = true;

    replay_seek( out.replay, 0 );

    ibxm_reader_decode_internal(&out);
    return out;

error: // Destroy everything
    dispose_replay(out.replay);
replay_create_error: // Destroy module
    dispose_module(out.module);
module_create_error: // Just wipe output
    memset(&out, 0, sizeof(out));
    return out;
}

uint32_t ibxm_reader_read_frames(ibxm_reader* ctx, uint16_t* frames_out, int64_t frame_count) {
    if (!ctx->initialized) {
        printf("%s(): IBXM reader not initialized, skipping!\n", __func__);
        return 0;
    }

    const uint8_t frame_size = ctx->sample_size * 2;

    uint32_t total_frames_read = 0;
    while (frame_count > 0) {
        const int64_t remaining = MAX(0, ctx->mixbuf_usable - ctx->mixbuf_read_pos);
        const int64_t bytes_read = MIN(remaining, frame_count * frame_size);
        const int64_t samples_read = (bytes_read / sizeof(*frames_out));
        const int64_t frames_read = (bytes_read / frame_size);

        const void* srcbuf = (const void*)((uintptr_t)ctx->mixbuf + ctx->mixbuf_read_pos);
        memcpy(frames_out, srcbuf, bytes_read);

        // Update all our positions
        ctx->mixbuf_read_pos += bytes_read;
        frame_count -= frames_read;
        frames_out += samples_read;
        total_frames_read += frames_read;

        if (ctx->mixbuf_read_pos >= ctx->mixbuf_usable) {
            memset(ctx->mixbuf, 0, ctx->mixbuf_size);
            const uint32_t samples_decoded = ibxm_reader_decode_internal(ctx);
            if (samples_decoded == 0) {
                break;
            }
        }
    }

    return total_frames_read;
}

void ibxm_reader_destroy(ibxm_reader* ctx) {
    if (!ctx->initialized) {
        printf("%s(): IBXM reader not initialized, skipping!\n", __func__);
        return;
    }

    dispose_replay(ctx->replay);
    dispose_module(ctx->module);
    free(ctx->mixbuf);
    memset(ctx, 0, sizeof(*ctx));
}