#include "miniaudio_it2play.h"
#include <it_d_rm.h>

#include <stdio.h>
#include <string.h> /* For memset(). */
#include <sys/stat.h>

/* This is defined out of order because the read function needs it */
static ma_result ma_it2_ds_get_data_format(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate, ma_channel* pChannelMap, size_t channelMapCap)
{
    ma_it2* pIT2 = (ma_it2*)pDataSource;

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

    if (!pIT2) {
        return MA_INVALID_OPERATION;
    }

    const ma_format format = pIT2->format;
    if (pFormat) {
        *pFormat = format;
    }

    const ma_uint32 channels = 2;
    if (pChannels) {
        *pChannels = channels;
    }

    if (pSampleRate) {
        *pSampleRate = pIT2->sample_rate;
    }

    if (pChannelMap) {
        ma_channel_map_init_standard(ma_standard_channel_map_default, pChannelMap, channelMapCap, channels);
    }

    return MA_SUCCESS;
}

static ma_result ma_it2_ds_read(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead)
{
    ma_it2* pIT2 = (ma_it2*)pDataSource;

    if (pFramesRead) {
        *pFramesRead = 0;
    }

    if (!pIT2 || frameCount == 0) {
        return MA_INVALID_ARGS;
    }

    ma_format format;
    ma_uint32 channels;
    ma_it2_ds_get_data_format(pIT2, &format, &channels, NULL, NULL, 0);
    DriverMix(frameCount, pFramesOut);
    ma_uint64 totalFramesRead = frameCount;
    if (pFramesRead) {
        *pFramesRead = totalFramesRead;
    }

    ma_result result = MA_SUCCESS;  /* Must be initialized to MA_SUCCESS. */
    if (!pIT2->song_at_start && Song.CurrentOrder == 0) {
        result = MA_AT_END;
    }

    if (Song.CurrentOrder != 0) {
        pIT2->song_at_start = false;
    }


    return result;
}

static ma_result ma_it2_ds_seek(ma_data_source* pDataSource, ma_uint64 frameIndex)
{
    ma_it2* pIT2 = (ma_it2*)pDataSource;
    if (!pIT2) {
        return MA_INVALID_ARGS;
    }

    return MA_UNAVAILABLE;
}

static int16_t getOrderEnd(int16_t currOrder)
{
    int16_t orderEnd = Song.Header.OrdNum - 1;
    if (orderEnd > 0)
    {
        int16_t i = currOrder;
        for (; i < orderEnd; i++)
        {
            if (Song.Orders[i] == 255)
                break;
        }

        orderEnd = i;
        if (orderEnd > 0)
            orderEnd--;
    }
    else
    {
        orderEnd = 0;
    }

    return orderEnd;
}

static ma_result ma_it2_ds_get_length(ma_data_source* pDataSource, ma_uint64* pLength)
{
    ma_it2* pIT2 = (ma_it2*)pDataSource;
    if (!pLength) {
        return MA_INVALID_ARGS;
    }
    *pLength = 0;   /* Safety. */

    if (!pIT2) {
        return MA_INVALID_ARGS;
    }

    // Taken from it2play.c
    const double dSamplesPerTick = (Driver.MixFrequency * 2.5) / Song.Tempo;
    const ma_uint32 numOrders = getOrderEnd(0);

    // From https://fileformats.fandom.com/wiki/Impulse_tracker#Patterns
    const ma_uint32 maxRowsPerPattern = 200;

    const ma_int64 length = dSamplesPerTick * Song.CurrentSpeed * numOrders * maxRowsPerPattern;
    if (length < 0) {
        return MA_ERROR;
    }
    *pLength = (ma_uint64)length;

    return MA_SUCCESS;
}

static ma_data_source_vtable g_ma_it2_ds_vtable =
{
    ma_it2_ds_read,
    ma_it2_ds_seek,
    ma_it2_ds_get_data_format,
    NULL, /* onGetCursor */
    ma_it2_ds_get_length,
    NULL,   /* onSetLooping */
    0       /* flags */
};

ma_result ma_it2_onInitMemory(void* pUserData, const void* pData, size_t dataSize, const ma_decoding_backend_config* pConfig, const ma_allocation_callbacks* pAllocationCallbacks, ma_it2* pIT2) {
    if (!pIT2) {
        return MA_INVALID_ARGS;
    }
    if (!pData || !dataSize) {
        return MA_INVALID_ARGS;
    }

    pIT2->format = ma_format_s16;
    pIT2->preferredFormat = pConfig->preferredFormat;

    if (pConfig != NULL && (pConfig->preferredFormat != pIT2->format)) {
        // return MA_FORMAT_NOT_SUPPORTED;
    }

    ma_data_source_config dataSourceConfig = ma_data_source_config_init();
    dataSourceConfig.vtable = &g_ma_it2_ds_vtable;
    ma_result result = ma_data_source_init(&dataSourceConfig, &pIT2->ds);

    if (result != MA_SUCCESS) {
        return result;
    }

    /* We can now initialize the decoder. */
    pIT2->sample_rate = 44100;
    if (!Music_Init(pIT2->sample_rate, sizeof(pIT2->mixbuf),  DRIVER_HQ)) {
        return MA_ERROR;
    }

    ma_uint8 res = Music_LoadFromData(pData, dataSize);
    if (res != LOAD_OK) {
        return MA_INVALID_FILE;
    }

    Music_PlaySong(0);

    return MA_SUCCESS;
}

MA_API void ma_it2_uninit(ma_it2* pIT2, const ma_allocation_callbacks* pAllocationCallbacks)
{
    if (!pIT2) {
        return;
    }

    Music_FreeSong();
    Music_Close();
    ma_data_source_uninit(&pIT2->ds);
}

/*
The code below defines the vtable that you'll plug into your `ma_decoder_config` object.
*/
ma_result ma_decoding_it2_onInitMemory(void* pUserData, const void* pData, size_t dataSize, const ma_decoding_backend_config* pConfig, const ma_allocation_callbacks* pAllocationCallbacks, ma_data_source** ppBackend) {
    ma_it2* pIT2 = (ma_it2*)ma_malloc(sizeof(*pIT2), pAllocationCallbacks);

    if (!pIT2) {
        return MA_OUT_OF_MEMORY;
    }
    memset(pIT2, 0, sizeof(*pIT2));

    *ppBackend = pIT2;
    ma_result result = ma_it2_onInitMemory(pUserData, pData, dataSize, pConfig, pAllocationCallbacks, pIT2);
    if (result != MA_SUCCESS) {
        ma_free(pIT2, pAllocationCallbacks);
        return result;
    }

    return MA_SUCCESS;
}

ma_result ma_decoding_it2_onInitFile(void* pUserData, const char* pFilePath, const ma_decoding_backend_config* pConfig, const ma_allocation_callbacks* pAllocationCallbacks, ma_data_source** ppBackend) {
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

    ma_decoding_it2_onInitMemory(pUserData, data, size, pConfig, pAllocationCallbacks, ppBackend);

    ma_free(data, pAllocationCallbacks);

    return MA_SUCCESS;
}

static void ma_decoding_backend_uninit__it2(void* pUserData, ma_data_source* pBackend, const ma_allocation_callbacks* pAllocationCallbacks)
{
    ma_it2_uninit(pBackend, pAllocationCallbacks);
    ma_free(pBackend, pAllocationCallbacks);
}

/*
IBXM's API only supports input from memory (not callback-based IO), so we
only support initialization from memory. Miniaudio says it will automatically
support file initialization using a wrapper. It probably also provides a wrapper
for callback-based IO.
 */
static ma_decoding_backend_vtable ma_gDecodingBackendVTable_it2 =
{
    NULL, /* onInit() */
    ma_decoding_it2_onInitFile, /* onInitFile() */
    NULL, /* onInitFileW() */
    ma_decoding_it2_onInitMemory,
    ma_decoding_backend_uninit__it2
};
ma_decoding_backend_vtable* ma_decoding_backend_it2 = &ma_gDecodingBackendVTable_it2;
