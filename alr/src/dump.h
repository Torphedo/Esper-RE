#include "alr.h"
#include "int_shorthands.h"

static void chunk_any(chunk_generic chunk, u8* chunk_buf, u32 idx) {
    return;
}

void chunk_0xD(chunk_generic chunk, u8* chunk_buf, u32 idx);
void chunk_texture(chunk_generic header, u8* chunk_buf, u32 idx);
void process_texture(u8* buf, u32 size, u32 idx);

void res_layout(chunk_generic chunk, u8* chunk_buf, u32 idx);
void stream_dump(chunk_generic chunk, u8* chunk_buf);

const alr_interface dump_interface = {
    .chunk_0x1 = chunk_any,
    .chunk_0x2 = chunk_any,
    .chunk_0x3 = chunk_any,
    .chunk_0x4 = chunk_any,
    .chunk_0x5 = chunk_any,
    .chunk_0x6 = chunk_any,
    .chunk_0x7 = chunk_any,
    .chunk_0x8 = chunk_any,
    .chunk_0x9 = chunk_any,
    .chunk_0xA = chunk_any,
    .chunk_0xB = chunk_any,
    .chunk_0xC = chunk_any,
    .chunk_0xD = chunk_0xD,
    .chunk_0xE = chunk_any,
    .chunk_0xF = chunk_any,
    .chunk_0x10 = chunk_texture,
    .chunk_0x11 = chunk_any,
    .chunk_0x12 = chunk_any,
    .chunk_0x13 = chunk_any,
    .chunk_0x14 = chunk_any,
    .chunk_0x15 = res_layout,
    .chunk_0x16 = chunk_any,

    .tex_handler = process_texture
};

