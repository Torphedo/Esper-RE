//
// Created by torph on 9/8/24.
//

#include "pd_common.h"
const char char_lookup[] = "0123456789abcdefghijklmnopqrstuvwxyz_";

// Decode 6 characters from the 32-bit integer they're stored in
void decode_single32(char* output, u32 val) {
    // This is signed so we can double-check it didn't overflow somehow
    s8 cur_char_idx = (PD_ENCODED_CHAR_COUNT / 2) - 1;
    while (val > 0) {
        // Each character code is multiplied by 40 and added to the encoded
        // value to store 6 characters in 4 bytes. To decode, we find the
        // remainder at each power of 40, and that's our character code.
        const u8 remainder = val % 40;
        val -= remainder;
        val /= 40;

        if (cur_char_idx < 0) {
            // We'll probably never hit this, but just in case...
            break;
        }

        // The value stored per character is actually an index into a lookup
        // table. (This is how the game does it so we're copying them)
        output[cur_char_idx] = char_lookup[remainder - 1];
        cur_char_idx--;
    }

}

