#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <common/int.h>

bool extract_sth2(const u8* data, u32 size, const char* outpath, u32 sample_rate);
bool dump_stx(const char* out_file, const u8* data, u32 size);

#ifdef __cplusplus
}
#endif
