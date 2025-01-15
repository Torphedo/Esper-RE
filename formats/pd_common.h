#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <common/int.h>

enum {
    PD_ENCODED_CHAR_COUNT = 12,
};

/// @brief Decode 6 characters from a 32-bit integer.
/// @param output buffer to store decoded characters in
/// @param val value to decode
void decode_single32(char* output, u32 val);

/// @brief Encode 6 characters into a 32-bit integer.
///
/// The only allowed characters are alphanumeric ASCII, spaces and underscores,
/// and NUL. Periods and dashes will be converted to underscores.
/// @param text string to encode (returns 0 if NULL)
///
/// @return encoded value representing the input string
u32 encode_single32(char* text);

#ifdef __cplusplus
}
#endif
