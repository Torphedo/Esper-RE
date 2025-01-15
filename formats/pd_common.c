#include "pd_common.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// The first character is NUL, so that an index of 0 or sizeof(char_lookup) - 1
// gives a NUL. This lets us clamp the value instead of doing a bounds check.

// In the game, alphabetic characters are sometimes interpreted as uppercase
// and sometimes lowercase. Similarly, the underscore is sometimes interpreted
// as a space.
// Lookup table extracted from PDUWP.exe by Vu314
const char char_lookup[] = "\0000123456789abcdefghijklmnopqrstuvwxyz_";

enum {
    // Index of the first number character
    LOOKUP_IDX_NUMBER = 1,

    // Index of the first alphabetic character
    LOOKUP_IDX_ALPHABETIC = 11,

    // The space is always at the end of the lookup table
    LOOKUP_IDX_SPACE = sizeof(char_lookup) - 2,

    ENCODING_BASE = 40,
};

void decode_single32(char* output, u32 encoded_val) {
    if (output == NULL) {
        return; // Not much we can do here.
    }

    // The loop counter represents the position of the character being decoded.
    for (s32 i = 0; i < ENCODED_CHAR_COUNT; i++) {
        // Each character is added to the final value, which is then multiplied
        // by the encoding base. To decode, we find the remainder at each power
        // of the base, and use that as an index into a lookup table. In the
        // end this stores 6 characters in 4 bytes.
        const u8 remainder = encoded_val % ENCODING_BASE;
        encoded_val /= ENCODING_BASE;

        // Out-of-bounds values are clamped to the index of the null terminator
        const u8 idx = MIN(remainder, sizeof(char_lookup) - 1);

        // Characters are decoded in reverse order, so we write them back-to-front
        output[ENCODED_CHAR_COUNT - i - 1] = char_lookup[idx];
    }
}

u32 encode_single32(char* input) {
    if (input == NULL) {
        return 0;
    }

    // If we don't encode exactly 6 characters, the decoder will leave the
    // remaining bytes (in the front!) uninitialized/zero.
    char buffer[ENCODED_CHAR_COUNT] = {0};
    strncpy(buffer, input, sizeof(buffer));

    u32 encoded_val = 0;
    for (u32 i = 0; i < ENCODED_CHAR_COUNT; i++) {
        const char c = buffer[i];

        u8 mapped_char = 0;
        // Numbers and letters are in a convienient order we can use to easily
        // compute the index.
        if (isdigit(c)) {
            mapped_char = LOOKUP_IDX_NUMBER + (c - '0');
        }
        else if (isalpha(c)) {
            mapped_char = LOOKUP_IDX_ALPHABETIC + (tolower(c) - 'a');
        }
        else if (c == ' ' || c == '_' || c == '-') {
            // These 3 characters are encoded to the same value
            mapped_char = LOOKUP_IDX_SPACE;
        }
        // Anything not covered here is encoded as 0

        encoded_val = (encoded_val * ENCODING_BASE) + mapped_char;
    }

    return encoded_val; // All done encoding!
}
