#include "arena.h"
#include "logging.h"

#include <malloc.h>
#include <Windows.h>

arena_t* create_arena(uint64_t size, arena_mode mode, uint64_t reserve_size)
{
    if (size > reserve_size && reserve_size != 0)
    {
        log_error(CRITICAL, "create_arena(): Requested allocation size is larger than the requested virtual reservation size!\n");
        return NULL;
    }
    arena_t* arena = calloc(1, sizeof(arena_t));
    arena->mode = mode;

    switch (mode)
    {
        case ALLOCATE_ALL:
            if (size == 0)
            {
                log_error(CRITICAL, "create_arena(): size can only be 0 in RESERVE mode!\n");
                free(arena);
                return NULL;
            }
            arena->reserve_size = 0;
            arena->base_addr = calloc(1, size);
            if (arena->base_addr == NULL) {
                log_error(CRITICAL, "create_arena(): Failed to allocate requested %lli bytes!\n", size);
                free(arena);
                return NULL;
            }
            arena->size = size;
            break;
        case RESERVE:
            if (reserve_size == 0) {reserve_size = 0x800000000;} // 32GB
            arena->reserve_size = reserve_size;
            // Reserve requested amount of virtual address space
            arena->base_addr = VirtualAlloc(NULL, reserve_size, MEM_RESERVE, PAGE_READWRITE);
            DWORD error = GetLastError();
            if (error != 0)
            {
                log_error(CRITICAL, "create_arena(): Error reserving memory for arena (code %li)\n", error);
            }
            if (arena->base_addr == NULL) {
                free(arena);
                return NULL;
            }
            else
            {
                // Commit requested size and update arena->size
                if (expand_arena(arena, size) == 0)
                {
                    return NULL;
                }
                break;
            }
        default:
            return NULL;
    }
    return arena;
}

void destroy_arena(arena_t* arena)
{
    switch (arena->mode) {
        case ALLOCATE_ALL:
            free(arena->base_addr);
            break;
        case RESERVE:
            SetLastError(0);
            VirtualFree(arena->base_addr, 0, MEM_RELEASE);
            DWORD error = GetLastError();
            if (error != 0)
            {
                log_error(CRITICAL, "destroy_arena(): Error freeing arena! Code: %lli (0x%x)\n", error, error);
            }
            break;
    }
    free(arena);
}

unsigned int expand_arena(arena_t* arena, uint64_t expand_size)
{
    if (arena->mode != RESERVE)
    {
        log_error(WARNING, "expand_arena() invalid parameter: arena mode must be RESERVE!\n");
        return 0;
    }
    else if ((arena->base_addr + arena->reserve_size) >= (arena->base_addr + arena->size + expand_size))
    {
        void* alloc_address = VirtualAlloc(arena->base_addr + arena->size, expand_size, MEM_COMMIT, PAGE_READWRITE);
        arena->size += expand_size;
        return expand_size;
    }
    else
    {
        log_error(CRITICAL, "expand_arena(): Not enough reserved virtual memory to expand the arena by the requested %lli bytes!\n", expand_size);
        return 0;
    }
}

void clear_arena(arena_t* arena)
{
    arena->pos = 0;
}

void* arena_alloc(arena_t* arena, uint64_t size)
{
    // Expand the arena to fit the requested size if needed
    if (arena->mode == RESERVE && arena->pos + size > arena->size)
    {
        expand_arena(arena, (arena->pos + size) - arena->size);
    }
    // Address at start of allocation
    void* data_address = arena->base_addr + arena->pos;
    arena_push(arena, size); // Push arena pointer
    return data_address; // Return allocation's address
}

void arena_push(arena_t* arena, uint64_t size)
{
    if (arena->pos + size > arena->size)
    {
        uint64_t difference = arena->pos + size - arena->size;
        log_error(WARNING, "arena_push(): Pushing arena position %lli bytes beyond committed storage!\n", difference);
    }
    arena->pos += size;
}

void arena_pop(arena_t* arena, uint64_t size)
{
    arena->pos -= size;
}

uint64_t arena_pos(arena_t* arena)
{
    return (uint64_t) arena->base_addr + arena->pos;
}
