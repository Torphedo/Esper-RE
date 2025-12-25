/*
This implements a data source that decodes module files via IBXM.

This object can be plugged into any `ma_data_source_*()` API, or used as a
custom decoding backend. See the custom_decoder example.
*/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <miniaudio.h>
#include "ibxm_reader.h"

typedef struct {
    ma_data_source_base ds;     /* The IBXM decoder can be used independently as a data source. */
    ma_format format;
    ibxm_reader reader;
}ma_ibxm;

/* Decoding backend vtable. This is what you'll plug into ma_decoder_config.pBackendVTables. No user data required. */
extern ma_decoding_backend_vtable* ma_decoding_backend_ibxm;

#ifdef __cplusplus
}
#endif
