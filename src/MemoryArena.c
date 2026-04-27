#include "MemoryArena.h"

#include <stdlib.h>
#include <stdint.h>

struct MemoryArena
{
    char *buffer;
    size_t size;
    size_t offset;
    size_t peakOffset;
};

struct TempArena
{
    struct MemoryArena *arena;
    size_t offset;
};

// Outputs the current stats of the memory arena, including total size, used space, peak usage, and free space
void OutputArenaStats(struct MemoryArena *arena)
{
    printf("Memory Arena Stats:\n");
    printf("Total Size: %zu bytes\n", arena->size);
    printf("Used: %zu bytes\n", arena->offset);
    printf("Peak Usage: %zu bytes\n", arena->peakOffset);
    printf("Free: %zu bytes\n", arena->size - arena->offset);
}

struct MemoryArena *CreateArena(size_t size)
{
    // Allocate memory for the MemoryArena struct
    struct MemoryArena *arena = (struct MemoryArena *)malloc(sizeof(struct MemoryArena) + size);
    if (!arena)
    {
        return NULL;
    }

    // Set the buffer pointer to the memory immediately following the MemoryArena struct
    arena->buffer = (char *)(arena + 1);

    // Initialize the MemoryArena fields
    arena->size = size;
    arena->offset = 0;
    arena->peakOffset = 0;

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

    // Update the peak offset if necessary
    if (arena->offset > arena->peakOffset)
    {
        arena->peakOffset = arena->offset;
    }

    // Return the aligned address
    return (void *)alignedAddress;
}

struct TempArena BeginTempArena(struct MemoryArena *arena)
{
    // Create a temporary arena for the current arena state
    struct TempArena temp;
    temp.arena = arena;
    temp.offset = arena->offset;
    return temp;
}

void EndTempArena(struct TempArena temp)
{
    // Return the arena to the offset saved in the temporary arena, this essentially erases any allocations within the temporary arena
    temp.arena->offset = temp.offset;
}