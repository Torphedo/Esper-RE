#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <ibxm.h>
#include <stdbool.h>
#include <stdint.h>

/// @file ibxm_reader.h
/// This is a wrapper around the IBXM library, which provides decoding for
/// several module file formats (.MOD, .XM, and .S3M).

typedef struct {
    struct module* module;
    struct replay* replay;
    int32_t* mixbuf; // Sample buffer for IBXM to output into
    uint32_t mixbuf_size;

    // Current position in reading data from the IBXM sample buffer
    uint32_t mixbuf_read_pos;
    // Amount of available data in the IBXM sample buffer
    uint32_t mixbuf_usable;

    // Sample size requested by the caller for output.
    uint32_t sample_size;

    // Sample rate requested by the caller (handled by IBXM)
    uint32_t sample_rate;
    uint32_t length_samples; // Length of the module
    bool initialized;
}ibxm_reader;

/// @brief Prepare to decode audio from an IBXM-supported file
/// @param module_data Contents of the module file
/// @param data_size Size of the module data
/// @param sample_rate Output sample rate, in Hz
/// @param sample_size Size of output samples, in bytes. Valid values are
///                    4 (32-bit) and 2 (16-bit), both are integer formats.
/// @return IBXM context
ibxm_reader ibxm_reader_create(const void* module_data, uint32_t data_size, uint32_t sample_rate, uint32_t sample_size);

/// @brief Read interleaved stereo samples from the module
/// @param ctx Sample reader context
/// @param frames_out Buffer to receive interleaved stereo samples
/// @param frame_count The number of interleaved stereo frames to read (a frame is a pair of left and right samples)
/// @return The number of frames that were actually read
uint32_t ibxm_reader_read_frames(ibxm_reader* ctx, uint16_t* frames_out, int64_t frame_count);

/// @brief Free all internal IBXM resources
void ibxm_reader_destroy(ibxm_reader* ctx);

#ifdef __cplusplus
}
#endif
