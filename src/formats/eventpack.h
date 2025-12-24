#pragma once
#include <common/int.h>
#include "shut_up_msvc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    u32 header_size; // This is variable depending on file count
    u32 file_count;
    u32 file_offsets[];
}ak_header;

// After this header there are [file_count] null-terminated strings with the
// filenames

#ifdef __cplusplus
}
#endif
