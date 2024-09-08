#pragma once
#include <common/int.h>

enum {
    PD_ENCODED_CHAR_COUNT = 12,
};

void decode_single32(char* output, u32 val);
