/*
This implements a data source that decodes module files via it2play.

This object can be plugged into any `ma_data_source_*()` API, or used as a
custom decoding backend. See the custom_decoder example.
*/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <miniaudio.h>
#include <it_music.h>

typedef struct {
    ma_data_source_base ds;     /* The decoder can be used independently as a data source. */
    ma_format format;
    ma_format preferredFormat;
    ma_uint32 sample_rate;
    bool song_at_start;
    ma_uint8 mixbuf[1024 * 32];
}ma_it2;

/* Decoding backend vtable. This is what you'll plug into ma_decoder_config.pBackendVTables. No user data required. */
extern ma_decoding_backend_vtable* ma_decoding_backend_it2;

#ifdef __cplusplus
}
#endif
