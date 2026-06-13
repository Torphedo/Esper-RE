#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <assert.h>
/// @file data_types.h
/// @brief Enum representing primitive data types
/// This is usually used to describe binary structures at runtime in your code.

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

/// Can only be used on arrays with compile-time known sizes
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

/// Round a number up to any boundary
#define ALIGN_UP(x, bound) ((x) + ((bound) - ((x) % (bound))))

// sys/param.h defines these on some platforms
#ifndef MAX
/// Return the larger of 2 values
    #define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
/// Return the smaller of 2 values
    #define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/// @brief Create a 32-bit "magic number" from 4 bytes (usually ASCII)
/// This is useful for parsing file formats, where files are often identified
/// by 4 ASCII bytes like 'DDS ' or 'SARC' or 'IWAD'
#define MAGIC(a, b, c, d) ((u32)a | ((u32)b << 8) | ((u32)c << 16) | ((u32)d << 24))


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
} data_type;

static const u8 data_type_sizes[] = {
    sizeof(s8), sizeof(u8),
    sizeof(s16), sizeof(u16),
    sizeof(s32), sizeof(u32),
    sizeof(float), sizeof(double),
    0,
};

static const char *data_type_names[] = {
    "s8", "u8", "s16", "u16", "s32", "u32", "float", "double", "[invalid type]"
};

static_assert(ARRAY_SIZE(data_type_sizes) == ARRAY_SIZE(data_type_names), "Array size mismatch!");

static u8 sizeof_type(data_type T) {
    if (T >= ARRAY_SIZE(data_type_sizes)) {
        return data_type_sizes[ARRAY_SIZE(data_type_sizes) - 1];
    }
    return data_type_sizes[T];
}

static const char *nameof_type(data_type T) {
    if (T >= ARRAY_SIZE(data_type_names)) {
        return data_type_names[ARRAY_SIZE(data_type_names) - 1];
    }
    return data_type_names[T];
}

#ifdef __cplusplus
}
#endif