#pragma once
#include <stdint.h>

///
/// \enum ALLOCATE_ALL = 0,\n RESERVE = 1
typedef enum {
    ALLOCATE_ALL = 0,
    RESERVE = 1
}arena_mode;

typedef struct arena_t {
    unsigned char* base_addr;
    uint64_t pos;
    uint64_t size;
    uint64_t reserve_size;
    arena_mode mode;
}arena_t;

///
/// \param size The amount of physical storage in bytes to commit. Can only be 0 in RESERVE mode.
/// \param mode The method used to allocate the arena. In ALLOCATE_ALL mode, it will immediately allocate memory for the whole arena with calloc().\n\n In RESERVE mode, it will reserve virtual address space and commit memory to it later. The size parameter can be used to commit some storage immediately. This mode allows you to create expanding contiguous buffers without any copying.
/// \param reserve_size The amount of virtual address space to reserve, if in RESERVE mode. If 0, it will reserve 32GB. Reserved space does not affect reported memory usage of the program.
/// \return A pointer to the newly created arena structure.
arena_t* create_arena(uint64_t size, arena_mode mode, uint64_t reserve_size);

///
/// This will invalidate all pointers to the arena, and the structure will be filled with zeroes.
/// \param arena The arena to be destroyed.
void destroy_arena(arena_t* arena);

///
/// \param arena Pointer to the arena to allocate memory in
/// \param expand_size The number of bytes to expand the arena by
/// \return Number of bytes successfully expanded. 0 indicates failure.
unsigned int expand_arena(arena_t* arena, uint64_t expand_size);

// Reset the position to 0, freeing all the data to be overwritten
void clear_arena(arena_t* arena);

// Returns a pointer to the new allocation and increments the arena position
///
/// \param arena Pointer to the arena to allocate memory in
/// \param size The amount of memory to allocate
/// \return Pointer to the newly allocated buffer
void* arena_alloc(arena_t* arena, uint64_t size);

// Increment arena pointer by the specified size
void arena_push(arena_t* arena, uint64_t size);

#define arena_push_array(arena, type, count) (type*) arena_push((arena), sizeof(type) * (count));

// Decrement arena pointer by the specified size
void arena_pop(arena_t* arena, uint64_t size);

// Returns pointer to current position
uint64_t arena_pos(arena_t* arena);
