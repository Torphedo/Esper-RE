#include "pd_common.h"
// The first character is NUL, so that an index of 0 or sizeof(char_lookup) - 1
// gives a NUL. This lets us clamp the value instead of doing a bounds check.
const char char_lookup[] = "\0000123456789abcdefghijklmnopqrstuvwxyz_";

// Decode 6 characters from the 32-bit integer they're stored in
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