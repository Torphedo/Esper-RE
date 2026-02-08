#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <ibxm.h>
#include <common/int.h>

/// @file ibxm_reader.h
/// This is a wrapper around the IBXM library, which provides decoding for
/// several module file formats (.MOD, .XM, and .S3M).

typedef struct {
    struct module* module;
    struct replay* replay;
    s32* mixbuf; // Sample buffer for IBXM to output into
    u32 mixbuf_size;

    // Current position in reading data from the IBXM sample buffer
    u32 mixbuf_read_pos;
    // Amount of available data in the IBXM sample buffer
    u32 mixbuf_usable;

    // Sample size requested by the caller for output.
    u32 sample_size;

    // Sample rate requested by the caller (handled by IBXM)
    u32 sample_rate;
    u32 length_samples; // Length of the module
    bool initialized;
}ibxm_reader;

/// @brief Prepare to decode audio from an IBXM-supported file
/// @param module_data Contents of the module file
/// @param data_size Size of the module data
/// @param sample_rate Output sample rate, in Hz
/// @param sample_size Size of output samples, in bytes. Valid values are
///                    4 (32-bit) and 2 (16-bit), both are integer formats.
/// @return IBXM context
ibxm_reader ibxm_reader_create(const void* module_data, u32 data_size, u32 sample_rate, u32 sample_size);

/// @brief Read interleaved stereo samples from the module
/// @param ctx Sample reader context
/// @param frames_out Buffer to receive interleaved stereo samples
/// @param frame_count The number of interleaved stereo frames to read (a frame is a pair of left and right samples)
/// @return The number of frames that were actually read
u32 ibxm_reader_read_frames(ibxm_reader* ctx, u16* frames_out, s64 frame_count);

/// @brief Free all internal IBXM resources
void ibxm_reader_destroy(ibxm_reader* ctx);

#ifdef __cplusplus
}
#endif
