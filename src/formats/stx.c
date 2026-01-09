#include "stx.h"
#include <stdbool.h>

stx_block_header stx_block_create(u16 total_num_blocks, u16 idx) {
    const bool is_first_block = (idx == 0);
    const u16 offset = (is_first_block) ? STX_FIRST_OFFSET : sizeof(stx_block_header);
    stx_block_header out = {
        .magic = STX_MAGIC,
        .offset = {
            .next_block = offset,
        },
        .channel_count = 2,
        .build_day = STX_PC_BUILD_DAY,
        .build_month = STX_PC_BUILD_MONTH,
        .build_year = STX_PC_BUILD_YEAR,
        .block_count = total_num_blocks,
        .block_idx = idx,
        // In vanilla files, the first block has this set to 0.
        .channel_size = STX_TOTAL_BLOCK_SAMPLES,
        .block_number = (u16)(idx + 1),
        .loop_start_block = 1,
        .loop_end_block = (u16) MAX(0, total_num_blocks - 1),
    };

    return out;
}
