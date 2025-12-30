#pragma once
#include <common/int.h>
/// @file data_types.h
/// @brief Enum representing primitive data types
/// This is usually used to describe binary structures at runtime in your code.

// Copied from ImGuiDataType, but with string and bool removed.
typedef enum {
    DATA_TYPE_S8,
    DATA_TYPE_U8,
    DATA_TYPE_S16,
    DATA_TYPE_U16,
    DATA_TYPE_S32,
    DATA_TYPE_U32,
    DATA_TYPE_FLOAT,
    DATA_TYPE_DOUBLE,
    DATA_TYPE_COUNT,
}data_type;

static const u8 data_type_sizes[] = {
    sizeof(s8), sizeof(u8),
    sizeof(s16), sizeof(u16),
    sizeof(s32), sizeof(u32),
    sizeof(float), sizeof(double),
    0,
};

static const char* data_type_names[] = {
    "s8", "u8", "s16", "u16", "s32", "u32", "float", "double", "[invalid type]"
};

static u8 sizeof_type(data_type T) {
    return data_type_sizes[MIN(T, ARRAY_SIZE(data_type_sizes) - 1)];
}

static const char* nameof_type(data_type T) {
    return data_type_names[MIN(T, ARRAY_SIZE(data_type_sizes) - 1)];
}
