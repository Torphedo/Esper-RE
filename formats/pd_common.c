#include "pd_common.h"
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

// The first character is NUL, so that an index of 0 or sizeof(char_lookup) - 1
// gives a NUL. This lets us clamp the value instead of doing a bounds check.

// In the game, alphabetic characters are sometimes interpreted as uppercase
// and sometimes lowercase. Similarly, the underscore is sometimes interpreted
// as a space.
const char char_lookup[] = "\0000123456789abcdefghijklmnopqrstuvwxyz_";

enum {
    // Index of the first alphabetic character
    LOOKUP_IDX_ALPHABETIC = 11,

    // The space is always at the end of the lookup table
    LOOKUP_IDX_SPACE = sizeof(char_lookup) - 2,
};

void decode_single32(char* output, u32 val) {
    // We use a signed loop counter so we can check for underflows.
    // The loop counter represents the position of the character being decoded.
    for (s32 i = (PD_ENCODED_CHAR_COUNT / 2) - 1; i >= 0; i--) {
        // Each character code is multiplied by 40 and added to the encoded
        // value to store 6 characters in 4 bytes. To decode, we find the
        // remainder at each power of 40, and that's our character code.
        const u8 remainder = val % 40;
        val -= remainder;
        val /= 40;

        // The value stored per character is actually an index into a lookup
        // table. (This is how the game does it so we're copying them)

        // Clamping to sizeof() - 1 instead of strlen() - 1 is intentional. This
        // means that an out-of-bounds remainder will get a NUL character.
        const u8 idx = CLAMP(0, remainder, sizeof(char_lookup) - 1);
        output[i] = char_lookup[idx]; // Store decoded character
    }
}

u32 encode_single32(char* string) {
    if (string == NULL) {
        return 0;
    }

    // Encoding a character essentially fills the last position with a character
    // and moves the rest back. If we don't encode exactly 6 characters, we
    // won't push the first character back to the first position, and it can
    // end up as a null terminator (breaking the decoding). To solve this, we
    // use a temporary buffer so there's always 6 characters.
    char text[6] = {0};
    strncpy(text, string, sizeof(text));

    u32 val = 0;
    for (u32 i = 0; i < 6; i++) {
        const char c = text[i];
        // Move on to the next power of 40
        val *= 40;

        if (isdigit(c)) {
            char buf[2] = {c}; // Use temp buffer to ensure it's null-terminated
            const u64 char_val = atoi(buf);
            // Numbers are at the start of the lookup table, we can use this to
            // our advantage by using the value to compute the index. The game
            // probably uses the value directly, we only need to add 1 because
            // we added a NUL as the first entry for convenient decoding.
            val += char_val + 1;
        }
        else if (isalpha(c)) {
            // ASCII characters start at a known index, so our index is the
            // distance from 'a' plus that.
            val += LOOKUP_IDX_ALPHABETIC + (tolower(c) - 'a');
        }
        else if (c == ' ' || c == '_' || c == '-' || c == '.') {
            // The last entry in the lookup table can be interpreted as a space
            // or underscore. For convenience, we'll turn characters that
            // aren't in the table but might get used as spacing to a space.
            val += LOOKUP_IDX_SPACE;
        } else {
            // This character isn't in the lookup table, skip it.
            continue;
        }
    }

    return val; // All done encoding!
}
