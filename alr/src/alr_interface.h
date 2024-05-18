#pragma once
#include <common/int.h>
#include <common/arguments.h>

#include "alr.h"

// Interfaces for handling the ALR data.
// Chunk size & ID, then the chunk data, then the offset array index this chunk
// is located in. (idx is used for split function)
typedef void (*chunk_handler)(void* ctx, chunk_generic chunk, u8* chunk_buf, u32 idx);
typedef void (*buffer_handler)(void* ctx, u8* buf, u32 size, u32 idx);

typedef enum {
    ALR_MAX_CHUNK_ID = 0x16
}chunkmax;

// ALR processing interface. The chunk loop will call the appropriate function
// for each chunk and for each texture buffer. Data from the resource layout
// header (0x15 chunk) is needed for texture metadata, so that chunk handler
// should store some state for later.
typedef struct {
    chunk_handler chunk_handlers[ALR_MAX_CHUNK_ID + 1];
    buffer_handler tex_handler;
    char* filename;
}alr_interface;


// Sends chunk data to callbacks via the provided interface, which can modify
// the input data. The ALR is written to the output file with any modifications
// made by callbacks. This allows callbacks to easily edit individual buffers
// of an ALR without handling the output themselves.
bool alr_edit(char* alr_filename, char* out_filename, flags options, alr_interface handlers);

// Sends chunk data to callbacks via the provided interface.
bool alr_parse(char* alr_filename, flags options, alr_interface handlers);

// Stub interface that does nothing, for info-only mode that produces no output
static const alr_interface stub_interface = {0};

