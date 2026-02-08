#include "ibxm_reader.h"
#include <stdlib.h>
#include <string.h>

u32 ibxm_reader_decode_internal(ibxm_reader* ctx) {
    if (!ctx->initialized) {
        LOG_MSG(warning, "IBXM reader not initialized, skipping!\n");
        return 0;
    }

    const u32 samples_read = replay_get_audio(ctx->replay, ctx->mixbuf, 0) * 2;
    s16* converted = (s16*)ctx->mixbuf;

    switch (ctx->sample_size) {
    case sizeof(s16):
        // For some reason IBXM outputs 32-bit samples, but with mostly 16-bit
        // magnitudes. We can just copy them as 16-bit samples to fix it.
        for (u32 i = 0; i < samples_read; i++) {
            // Casting to 16-bit will cause occasional out-of-bounds 32-bit
            // values to flip sign, which causes unpleasant popping
            const s16 sample = CLAMP(INT16_MIN, ctx->mixbuf[i], INT16_MAX);
            *converted = sample;
            converted++;
        }
        break;
    case sizeof(s32):
        break;
    default:
        LOG_MSG(warning, "Unknown target sample size (%d bytes)!\n", ctx->sample_size);
        break;
    }


    ctx->mixbuf_read_pos = 0;
    ctx->mixbuf_usable = samples_read * ctx->sample_size;
    return samples_read;
}

ibxm_reader ibxm_reader_create(const void* module_data, u32 data_size, u32 sample_rate, u32 sample_size) {
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

    out.mixbuf_size = calculate_mix_buf_len(sample_rate) * sizeof(s32);
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

u32 ibxm_reader_read_frames(ibxm_reader* ctx, u16* frames_out, s64 frame_count) {
    if (!ctx->initialized) {
        LOG_MSG(warning, "IBXM reader not initialized, skipping!\n");
        return 0;
    }

    const u8 frame_size = ctx->sample_size * 2;

    u32 total_frames_read = 0;
    while (frame_count > 0) {
        const s64 remaining = MAX(0, ctx->mixbuf_usable - ctx->mixbuf_read_pos);
        const s64 bytes_read = MIN(remaining, frame_count * frame_size);
        const s64 samples_read = (bytes_read / sizeof(*frames_out));
        const s64 frames_read = (bytes_read / frame_size);

        const void* srcbuf = (const void*)((uintptr_t)ctx->mixbuf + ctx->mixbuf_read_pos);
        memcpy(frames_out, srcbuf, bytes_read);

        // Update all our positions
        ctx->mixbuf_read_pos += bytes_read;
        frame_count -= frames_read;
        frames_out += samples_read;
        total_frames_read += frames_read;

        if (ctx->mixbuf_read_pos >= ctx->mixbuf_usable) {
            memset(ctx->mixbuf, 0, ctx->mixbuf_size);
            const u32 samples_decoded = ibxm_reader_decode_internal(ctx);
            if (samples_decoded == 0) {
                break;
            }
        }
    }

    return total_frames_read;
}

void ibxm_reader_destroy(ibxm_reader* ctx) {
    if (!ctx->initialized) {
        LOG_MSG(warning, "IBXM reader not initialized, skipping!\n");
        return;
    }

    dispose_replay(ctx->replay);
    dispose_module(ctx->module);
    free(ctx->mixbuf);
    memset(ctx, 0, sizeof(*ctx));
}
