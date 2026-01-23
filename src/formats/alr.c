#include "alr.h"
#include <math.h>

bool is_power_of_two(u16 val) {
    // For example:
    // 2^7     = 0x80 = 0b100000000
    // 2^7 - 1 = 0x7F = 0b001111111
    // 0x80 & 0x7F == 0 (false).

    // See https://stackoverflow.com/a/108340
    return !(val & (val - 1));
}

void alr_texture_get_dimensions(texture_entry entry, u16* height_out, u16* width_out) {
    // It would make more sense to check if *either* is 0, but the game only
    // checks that they're *both* 0.
    if (entry.width_direct == 0 && entry.width_direct == 0) {
        *width_out = 1 << entry.width_pwr;
        *height_out = 1 << entry.height_pwr;
    } else {
        *width_out = entry.width_direct + 1;
        *height_out = entry.height_direct + 1;
    }
}

void alr_texture_set_dimensions(texture_entry* entry, u16 height, u16 width) {
    if (is_power_of_two(height) && is_power_of_two(width)) {
        entry->height_pwr = log2(height);
        entry->width_pwr = log2(width);

        // Wipe the other fields so the game doesn't try to use them
        entry->height_direct = 0;
        entry->width_direct = 0;
    } else {
        entry->height_direct = 0;
        entry->width_direct = 0;
        entry->height_direct = height;
        entry->width_direct = width;

        // Wipe the other fields so the game doesn't try to use them
        entry->height_pwr = 0;
        entry->width_pwr = 0;
    }
}
