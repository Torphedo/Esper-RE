/*
 * Implements Impulse Tracker 2.15 module support for Miniaudio, via the it2play
 * library.
*/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <miniaudio.h>
#include <stdbool.h>

/* Like all decoders, this can be safely used as an ma_data_source. */
typedef struct {
    ma_data_source_base ds;
    ma_format format; /* Format of the samples coming out of the module */
    ma_format preferredFormat; /* Format the caller asked for */
    ma_uint32 sample_rate;
    bool song_at_start;
}ma_it2;

/* Decoding backend vtable. This is what you'll plug into ma_decoder_config.pBackendVTables. */
extern ma_decoding_backend_vtable* ma_decoding_backend_it2;

#ifdef __cplusplus
}
#endif
