#include "miniaudio_stx.h"
#include <stdlib.h>
#include <miniaudio.h>

#include <common/vfile.h>
#include <common/file.h>
#include <formats/stx.h>

#include "miniaudio_ibxm.h"

void data_callback(void* ctx, void* audioOut, u32 frameCount) {
// void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    ma_decoder* decoder = (ma_decoder*)ctx;

    ma_decoder_read_pcm_frames(decoder, audioOut, frameCount, NULL);
}

bool ma_generate_stx(const char* inpath, const char* stx_path) {
    void* stx_data = NULL;
    u32 stx_size = 0;

    const u16 sample_rate = STX_PC_SAMPLE_RATE;
    ma_decoder_config cfg = ma_decoder_config_init(ma_format_s16, 2, sample_rate);
    cfg.ppCustomBackendVTables = &ma_decoding_backend_ibxm;
    cfg.customBackendCount = 1;
    cfg.pCustomBackendUserData = NULL;

    ma_decoder decoder = {};
    ma_result ma_res = ma_decoder_init_file(inpath, &cfg, &decoder);
    if (ma_res != MA_SUCCESS) {
        LOG_MSG(error, "Failed to setup audio decoder!\n");
        return false;
    }

    ma_uint64 sample_count = 0;
    ma_decoder_get_length_in_pcm_frames(&decoder, &sample_count);
    generate_stx(data_callback, &decoder, sample_count, &stx_data, &stx_size);

    ma_decoder_uninit(&decoder);

    FILE* out = fopen(stx_path, "wb");
    if (out) {
        fwrite(stx_data, stx_size, 1, out);
        fclose(out);
    }
    free(stx_data);
}
