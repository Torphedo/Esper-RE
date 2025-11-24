#include "pd_common.h"

#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>

#include <common/vfile.h>
#include <formats/sth2.h>
#include <formats/stx.h>
#include <formats/wav.h>

// The first character is NUL, so that an index of 0 or sizeof(char_lookup) - 1
// gives a NUL. This lets us clamp the value instead of doing a bounds check.

// In the game, alphabetic characters are sometimes interpreted as uppercase
// and sometimes lowercase. Similarly, the underscore is sometimes interpreted
// as a space.
// Lookup table extracted from PDUWP.exe by Vu
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

decoded_text decode_double(u32 text1, u32 text2) {
    decoded_text name = {0};
    decode_single32(name.data, text1);
    decode_single32(&name.data[6], text2);

    return name;
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

bool extract_sth2(const u8* data, u32 size, const char* outpath, u32 sample_rate) {
    FILE* f = fopen(outpath, "wb");
    if (!f) {
        LOG_MSG(error, "I couldn't open the WAV output file '%s'\n", outpath);
        return false;
    }

    vfile vf = vfile_open((u8*)data, size);
    sth2_header header = VFILE_READ(sth2_header, &vf);

    vf.pos = header.wave_offset;
    const sth2_wave_header pd_wave_header = VFILE_READ(sth2_wave_header, &vf);
    const u32 offsets_size = pd_wave_header.num_offsets * sizeof(*pd_wave_header.offsets);
    vfile_seek(&vf, offsets_size);

    const u32 header_size = sizeof(pd_wave_header) + offsets_size;
    const u32 audio_size = pd_wave_header.size - header_size;
    const u8* audio = vfile_cur(vf);

    wav_write_audio(sample_rate, 1, sizeof(u16), WAV_FMT_PCM, audio, audio_size, f);
    fclose(f);

    return true;
}

bool dump_stx(const char* out_file, const u8* data, u32 size) {
    FILE* f = fopen(out_file, "wb");
    if (!f) {
        return false;
    }

    vfile vf = vfile_open((u8*)data, size);

    const stx_first_block* header = VFILE_READ_PTR(stx_first_block, &vf);
    const u16 sample_rate = header->channels[0].sample_rate / 2;
    wav_write_headers(sample_rate, 2, sizeof(u16), WAV_FMT_PCM, 0, f);

    u32 audio_size = 0;
    for (u32 i = 0; i < 2; i++) {
        audio_size = 0;
        vf.pos = header->header.offset.start;

        for (u32 j = 0; j < header->header.block_count - 1; j++) {
            const u32 block_offset = vf.pos;
            const stx_block_header* block = VFILE_READ_PTR(stx_block_header, &vf);
            u16 channel_size = block->channel_size;
            channel_size = 1008;

            const u16 block_size = channel_size * block->channel_count;
            const u32 next_block = vf.pos + block_size;
            audio_size += block_size;

            if (block->magic != STX_MAGIC) {
                LOG_MSG(warning, "Invalid block magic %X @ 0x%X!\n", block->magic, vf.pos - sizeof(*block));
            }
            if (block->channel_size != 1008) {
                LOG_MSG(warning, "Unexpected channel size %d @ 0x%X\n", block->channel_size, vf.pos - sizeof(*block));
            }
            if (block->channel_count != 2) {
                LOG_MSG(warning, "Unexpected channel count %d @ 0x%X\n", block->channel_count, vf.pos - sizeof(*block));
            }

            // Skip to the appropriate channel & save samples
            vfile_seek(&vf, channel_size * i);
            const u16* samples = (const u16*)vfile_cur(vf);
            fwrite(samples, channel_size, 1, f);

            vf.pos = next_block;
        }
    }

    // Update WAV sizes
    fseek(f, 0, SEEK_SET);
    wav_write_headers(sample_rate, 2, sizeof(u16), WAV_FMT_PCM, audio_size, f);

    fclose(f);
    return true;
}
