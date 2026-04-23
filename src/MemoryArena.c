#include "MemoryArena.h"

#include <stdlib.h>
#include <stdint.h>

struct MemoryArena
{
    char *buffer;
    size_t size;
    size_t offset;
};

struct MemoryArena *CreateArena(size_t size)
{
    // Allocate memory for the MemoryArena struct
    struct MemoryArena *arena = (struct MemoryArena *)malloc(sizeof(struct MemoryArena));
    if (!arena)
    {
        return NULL;
    }

    // Allocate memory for the buffer
    arena->buffer = (char *)malloc(size);
    if (!arena->buffer)
    {
        free(arena);
        return NULL;
    }

    // Initialize the MemoryArena fields
    arena->size = size;
    arena->offset = 0;

    return arena;
}

void DestroyArena(struct MemoryArena *arena)
{
    if (arena)
    {
        if (arena->buffer)
        {
            free(arena->buffer);
        }
        free(arena);
    }
}

void *arenaAllocAlign(struct MemoryArena *arena, size_t size, size_t alignment)
{

    // Calculate the aligned address using bitwise operations
    uintptr_t currentAddress = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
    uintptr_t alignedAddress = (currentAddress + alignment - 1) & ~(alignment - 1);

    // Calculate the padding needed to achieve the aligned address
    size_t padding = alignedAddress - currentAddress;

    // Check that the arena has enough space for the requested size and padding
    if (arena->offset + padding + size > arena->size)
    {
        // Not enough space in the arena
        return NULL;
    }

    // Move the offset to the aligned address
    arena->offset += padding + size;

    // Return the aligned address
    return (void *)alignedAddress;
}
