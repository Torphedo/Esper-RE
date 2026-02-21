#include "miniaudio_ibxm.h"

#include <stdio.h>
#include <string.h> /* For memset(). */
#include <sys/stat.h>

/* This is defined out of order because the read function needs it */
static ma_result ma_ibxm_ds_get_data_format(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap)
{
    ma_ibxm* pIBXM = (ma_ibxm*)pDataSource;

    /* Defaults for safety. */
    if (pFormat) {
        *pFormat = ma_format_unknown;
    }
    if (pChannels) {
        *pChannels = 0;
    }
    if (pSampleRate) {
        *pSampleRate = 0;
    }
    if (pChannelMap) {
        memset(pChannelMap, 0, sizeof(*pChannelMap) * channelMapCap);
    }

    if (!pIBXM) {
        return MA_INVALID_OPERATION;
    }

    ma_format format;
    switch (pIBXM->reader.sample_size) {
        case 2:
            format = ma_format_s16;
            break;
        case 4:
            format = ma_format_s32;
            break;
        default:
            format = ma_format_unknown;
            break;
    }

    if (pFormat) {
        *pFormat = format;
    }

    const ma_uint32 channels = 2;
    if (pChannels) {
        *pChannels = channels;
    }

    if (pSampleRate) {
        *pSampleRate = pIBXM->reader.sample_rate;
    }

    if (pChannelMap) {
        ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap, channels);
    }

    return MA_SUCCESS;
}

static ma_result ma_ibxm_ds_read(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
{
    ma_ibxm* pIBXM = (ma_ibxm*)pDataSource;

    if (pFramesRead) {
        *pFramesRead = 0;
    }

    if (!pIBXM || frameCount == 0) {
        return MA_INVALID_ARGS;
    }

    ma_format format;
    ma_uint32 channels;
    ma_ibxm_ds_get_data_format(pIBXM, &format, &channels, NULL, NULL, 0);

    ma_uint64 totalFramesRead = ibxm_reader_read_frames(&pIBXM->reader, pFramesOut, frameCount);
    if (pFramesRead) {
        *pFramesRead = totalFramesRead;
    }

    ma_result result = MA_SUCCESS;  /* Must be initialized to MA_SUCCESS. */
    if (totalFramesRead == 0) {
        result = MA_AT_END;
    }

    return result;
}

static ma_result ma_ibxm_ds_seek(ma_data_source* pDataSource, ma_uint64 frameIndex)
{
    ma_ibxm* pIBXM = (ma_ibxm*)pDataSource;
    if (!pIBXM) {
        return MA_INVALID_ARGS;
    }

    replay_seek(pIBXM->reader.replay, frameIndex);
    return MA_SUCCESS;
}

static ma_result ma_ibxm_ds_get_length(ma_data_source* pDataSource, ma_uint64* pLength)
{
    ma_ibxm* pIBXM = (ma_ibxm*)pDataSource;
    if (!pLength) {
        return MA_INVALID_ARGS;
    }
    *pLength = 0;   /* Safety. */

    if (!pIBXM) {
        return MA_INVALID_ARGS;
    }

    const ma_int64 length = replay_calculate_duration(pIBXM->reader.replay);
    if (length < 0) {
        return MA_ERROR;
    }
    *pLength = (ma_uint64)length;

    return MA_SUCCESS;
}

static ma_data_source_vtable g_ma_ibxm_ds_vtable =
{
    ma_ibxm_ds_read,
    ma_ibxm_ds_seek,
    ma_ibxm_ds_get_data_format,
    NULL, /* onGetCursor */
    ma_ibxm_ds_get_length,
    NULL,   /* onSetLooping */
    0       /* flags */
};

ma_result ma_ibxm_onInitMemory(void* pUserData, const void* pData, size_t dataSize, const ma_decoding_backend_config* pConfig, const ma_allocation_callbacks* pAllocationCallbacks, ma_ibxm* pIBXM) {
    if (!pIBXM) {
        return MA_INVALID_ARGS;
    }
    if (!pData || !dataSize) {
        return MA_INVALID_ARGS;
    }

    pIBXM->format = ma_format_s16;

    if (pConfig != NULL && (pConfig->preferredFormat == ma_format_s32 || pConfig->preferredFormat == ma_format_s16)) {
        pIBXM->format = pConfig->preferredFormat;
    } else {
        return MA_FORMAT_NOT_SUPPORTED;
    }

    ma_data_source_config dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &g_ma_ibxm_ds_vtable;
    ma_result result = ma_data_source_init(&dataSourceConfig, &pIBXM->ds);

    if (result != MA_SUCCESS) {
        return result;
    }

    /* We can now initialize the decoder. */
    const ma_uint32 sample_rate = 44100;
    const ma_uint8 sample_size = ma_get_bytes_per_sample(pIBXM->format);
    pIBXM->reader = ibxm_reader_create(pData, dataSize, sample_rate, sample_size);

    if (!pIBXM->reader.initialized) {
        return MA_INVALID_FILE;
    }

    return MA_SUCCESS;
}

MA_API void ma_ibxm_uninit(ma_ibxm* pIBXM, const ma_allocation_callbacks* pAllocationCallbacks)
{
    if (!pIBXM) {
        return;
    }

    ibxm_reader_destroy(&pIBXM->reader);
    ma_data_source_uninit(&pIBXM->ds);
}

/*
The code below defines the vtable that you'll plug into your `ma_decoder_config` object.
*/
ma_result ma_decoding_ibxm_onInitMemory(void* pUserData, const void* pData, size_t dataSize, const ma_decoding_backend_config* pConfig, const ma_allocation_callbacks* pAllocationCallbacks, ma_data_source** ppBackend) {
    ma_ibxm* pIBXM = (ma_ibxm*)ma_malloc(sizeof(*pIBXM), pAllocationCallbacks);

    if (!pIBXM) {
        return MA_OUT_OF_MEMORY;
    }
    memset(pIBXM, 0, sizeof(*pIBXM));

    ma_result result = ma_ibxm_onInitMemory(pUserData, pData, dataSize, pConfig, pAllocationCallbacks, pIBXM);
    if (result != MA_SUCCESS) {
        ma_free(pIBXM, pAllocationCallbacks);
        return result;
    }

    *ppBackend = pIBXM;
    return MA_SUCCESS;
}

ma_result ma_decoding_ibxm_onInitFile(void* pUserData, const char* pFilePath, const ma_decoding_backend_config* pConfig, const ma_allocation_callbacks* pAllocationCallbacks, ma_data_source** ppBackend) {
    struct stat st = {0};
    if (stat(pFilePath, &st) != 0) {
        return MA_IO_ERROR;
    }

    FILE* f = fopen(pFilePath, "rb");
    if (!f) {
        return MA_IO_ERROR;
    }

    // Tracker files are generally very small, so just load the whole thing
    const ma_uint64 size = st.st_size;
    void* data = ma_malloc(size, pAllocationCallbacks);
    fread(data, size, 1, f);
    fclose(f);

    ma_result res = ma_decoding_ibxm_onInitMemory(pUserData, data, size, pConfig, pAllocationCallbacks, ppBackend);
    ma_free(data, pAllocationCallbacks);

    if (res == MA_SUCCESS) {
        return MA_SUCCESS;
    } else {
        return res;
    }
}

static void ma_decoding_backend_uninit__ibxm(void* pUserData, ma_data_source* pBackend, const ma_allocation_callbacks* pAllocationCallbacks)
{
    ma_ibxm_uninit(pBackend, pAllocationCallbacks);
    ma_free(pBackend, pAllocationCallbacks);
}

/*
IBXM's API only supports input from memory (not callback-based IO), so we
only support initialization from memory. Miniaudio says it will automatically
support file initialization using a wrapper. It probably also provides a wrapper
for callback-based IO.
 */
static ma_decoding_backend_vtable ma_gDecodingBackendVTable_ibxm =
{
    NULL, /* onInit() */
    ma_decoding_ibxm_onInitFile, /* onInitFile() */
    NULL, /* onInitFileW() */
    ma_decoding_ibxm_onInitMemory,
    ma_decoding_backend_uninit__ibxm
};
ma_decoding_backend_vtable* ma_decoding_backend_ibxm = &ma_gDecodingBackendVTable_ibxm;
