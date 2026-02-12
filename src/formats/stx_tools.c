#include "stx_tools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <miniaudio.h>

#include <common/vfile.h>
#include <common/file.h>
#include <formats/stx.h>

#include "miniaudio_ibxm.h"

bool deinterleave_samples(void* samples, u64 buf_size, u8 sample_size) {
    const u8 channels = 2;
    const u64 num_samples = buf_size / sample_size;
    const u64 channel_samples = num_samples / channels;
    const u64 channel_size = buf_size / channels;

    // We copy the interleaved channel 2 samples here
    void* temp = malloc(channel_size);
    if (!temp) {
        return false;
    }

    {
        uintptr_t source = (uintptr_t)samples;
        uintptr_t target = (uintptr_t)temp;

        source += sample_size; // Start on channel 2 sample

        // Copy every other sample (all the channel 2 samples) into the temp buffer contiguously
        for (u64 i = 0; i < channel_samples; i++, source += sample_size * 2, target += sample_size) {
            memcpy((void*)target, (void*)source, sample_size);
        }
    }

    {
        uintptr_t source = (uintptr_t)samples;
        uintptr_t target = source;

        // Copy every other sample backwards, making the channel 1 samples continguous.
        for (u64 i = 0; i < channel_samples; i++, source += sample_size * 2, target += sample_size) {
            memcpy((void*)target, (void*)source, sample_size);
        }

    }

    // Copy the (now tightly packed) channel 2 samples to the 2nd half of the original buffer.
    const uintptr_t channel2 = (uintptr_t)samples + channel_size;
    memcpy((void*)channel2, temp, channel_size);
    free(temp);

    return true;
}

void interleave_samples(const void* const* channels, u8 num_channels, void* output, u64 num_samples, u8 sample_size) {
    u8* out = output;

    for (u64 i = 0; i < num_samples * sample_size; i += sample_size) {
        for (u8 j = 0; j < num_channels; j++) {
            const u8* channel = channels[j];
            channel += i; // Skip to current sample

            // Copy sample and advance
            memcpy(out, channel, sample_size);
            out += sample_size;
        }
    }
}

bool generate_stx(const u8* data, s64 size, void** stx_buf_out, u32* stx_size_out) {
    const u16 sample_rate = STX_PC_SAMPLE_RATE;
    bool result = false;

    ma_decoder_config cfg = ma_decoder_config_init(ma_format_s16, 2, sample_rate);
    cfg.ppCustomBackendVTables = &ma_decoding_backend_ibxm;
    cfg.customBackendCount = 1;
    cfg.pCustomBackendUserData = NULL;

    ma_decoder ma_decoder;
    ma_result ma_res = ma_decoder_init_memory(data, size, &cfg, &ma_decoder);

    if (ma_res != MA_SUCCESS) {
        LOG_MSG(error, "Failed to setup audio decoder!\n");
        return false;
    }

    ma_uint64 sample_count = 0;
    ma_decoder_get_length_in_pcm_frames(&ma_decoder, &sample_count);
    sample_count *= 2;

    const s64 stx_size = stx_size_from_sample_count(sample_count);
    void* stx_data = calloc(1, stx_size);
    if (!stx_data) {
        LOG_MSG(error, "Failed to allocate %d bytes to generate STX\n");
        goto end;
    }

    vfile vf = vfile_open(stx_data, stx_size);

    // Write header
    const u32 num_blocks = stx_num_blocks_from_samples(sample_count);
    VFILE_WRITE(stx_block_header, &vf, stx_block_create(num_blocks, 0));

    // Write channel metadata
    for (u32 i = 0; i < STX_MAX_CHANNELS; i++) {
        stx_channel channel = {
            .sample_rate = sample_rate,
        };
        if (i < 2) {
            channel.volume = 0x7F;
            channel.pan[i] = 0x7F;
        }
        VFILE_WRITE(stx_channel, &vf, channel);
    }

    // Write channel names
    vf.pos = offsetof(stx_first_block, channel_names);
    strcpy((char*)vfile_cur(vf), "left");
    vf.pos += STX_CHANNEL_NAME_SIZE;
    strcpy((char*)vfile_cur(vf), "right");

    // Skip to audio data
    vf.pos = STX_FIRST_OFFSET;

    // Decode & de-interleave samples
    for (u32 i = 1; i < num_blocks; i++) {
        stx_block_header block = stx_block_create(num_blocks, i);
        VFILE_WRITE(stx_block_header, &vf, block);
        void* samples = vfile_cur(vf);

        ma_decoder_read_pcm_frames(&ma_decoder, samples, STX_BLOCK_SAMPLES, NULL);
        deinterleave_samples(samples, STX_TOTAL_BLOCK_SAMPLES * sizeof(u16), sizeof(u16));

        vfile_seek(&vf, STX_TOTAL_BLOCK_SAMPLES * sizeof(u16));
    }

    result = true;

    end:
    ma_decoder_uninit(&ma_decoder);
    *stx_buf_out = stx_data;
    *stx_size_out = stx_size;
    return result;
}

bool generate_stx_from_file(const char* inpath, const char* stx_path) {
    s64 in_size = file_size(inpath);
    u8* indata = file_load(inpath);
    if (!indata) {
        return false;
    }

    void* stx_data = NULL;
    u32 stx_size = 0;
    generate_stx(indata, in_size, &stx_data, &stx_size);
    free(indata);

    FILE* out = fopen(stx_path, "wb");
    if (out) {
        fwrite(stx_data, stx_size, 1, out);
        fclose(out);
    }
    free(stx_data);
}
