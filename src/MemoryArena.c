#include "MemoryArena.h"

#include <stdlib.h>
#include <stdint.h>

#include "platform.h"

#if ARENA_USE_VIRTUAL_MEMORY
struct MemoryArena
{
    char *buffer;
    size_t size;
    size_t offset;
    size_t peakOffset;

    // Out of memory handling policy for the arena
    enum oomPolicy oomPolicy;
    // Optional callback function for OOM handling when the policy is OOM_CALLBACK
    void (*oomCallback)(struct MemoryArena *arena, size_t requestedSize);
};

struct TempArena
{
    struct MemoryArena *arena;
    size_t offset;
};
#else
typedef struct ArenaBlock
{
    char *buffer;
    size_t size;
    size_t offset;
    struct ArenaBlock *prev;
} ArenaBlock;

struct MemoryArena
{
    struct ArenaBlock *currentBlock;
    size_t peakOffset;

    // Out of memory handling policy for the arena
    oomPolicy oomPolicy;
    // Optional callback function for OOM handling when the policy is OOM_CALLBACK
    void (*oomCallback)(struct MemoryArena *arena, size_t requestedSize);
};

struct TempArena
{
    struct MemoryArena *arena;
    struct ArenaBlock *startBlock;
    size_t offset;
};

#endif

// Outputs the current stats of the memory arena, including total size, used space, peak usage, and free space
void OutputArenaStats(struct MemoryArena *arena)
{
    printf("Memory Arena Stats:\n");
    printf("Total Size: %zu bytes\n", arena->size);
    printf("Used: %zu bytes\n", arena->offset);
    printf("Peak Usage: %zu bytes\n", arena->peakOffset);
    printf("Free: %zu bytes\n", arena->size - arena->offset);
}

#if ARENA_USE_VIRTUAL_MEMORY
struct MemoryArena *CreateArena(size_t size, enum oomPolicy policy, void (*oomCallback)(struct MemoryArena *, size_t))
{
    // Allocate memory for the MemoryArena struct
    struct MemoryArena *arena = (struct MemoryArena *)ARENA_SYS_ALLOC(sizeof(struct MemoryArena) + size);
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
    arena->oomPolicy = policy;
    arena->oomCallback = oomCallback;

    return arena;
}
#else
// For non-virtual memory arenas, we need to allocate a block of memory for the arena and manage it using linked blocks if necessary
struct MemoryArena *CreateArena(size_t size, enum oomPolicy policy, void (*oomCallback)(struct MemoryArena *, size_t))
{
    // Allocate memory for the MemoryArena struct
    struct MemoryArena *arena = (struct MemoryArena *)ARENA_SYS_ALLOC(sizeof(struct MemoryArena));
    if (!arena)
    {
        return NULL;
    }

    // Initialize the first block of the arena
    arena->currentBlock = (struct ArenaBlock *)malloc(sizeof(struct ArenaBlock));
    if (!arena->currentBlock)
    {
        free(arena);
        return NULL;
    }

    arena->currentBlock->buffer = (char *)malloc(size);
    if (!arena->currentBlock->buffer)
    {
        free(arena->currentBlock);
        free(arena);
        return NULL;
    }

    arena->currentBlock->size = size;
    arena->currentBlock->offset = 0;
    arena->currentBlock->prev = NULL;

    // Initialize the MemoryArena fields
    arena->peakOffset = 0;

    return arena;
}
#endif

void DestroyArena(struct MemoryArena *arena)
{
    if (arena)
    {
        ARENA_SYS_FREE(arena, sizeof(struct MemoryArena) + arena->size);
    }
}

void *arenaAllocAlign(struct MemoryArena *arena, size_t size, size_t alignment)
{

    // Calculate the aligned address using bitwise operations
    uintptr_t currentAddress = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
    uintptr_t alignedAddress = (currentAddress + alignment - 1) & ~(alignment - 1);

    // Calculate the padding needed to achieve the aligned address
    size_t padding = alignedAddress - currentAddress;

    // Check if there is enough space in the arena for the requested size and padding
    if (arena->offset + padding + size > arena->size)
    {
        // Handle out of memory based on the arena's OOM policy
        switch (arena->oomPolicy)
        {
        case OOM_RETURN_NULL:
            return NULL;
        case OOM_ABORT:
            fprintf(stderr, "Out of memory in arena allocation. Requested size: %zu bytes\n", size);
            abort();
        case OOM_CALLBACK:
            if (arena->oomCallback)
            {
                arena->oomCallback(arena, size);
            }

            // Return NULL after the callback, as the callback is expected to handle the OOM situation
            return NULL;

        case OOM_GROW_ARENA:
#ifndef ARENA_USE_VIRTUAL_MEMORY
            // For non-virtual memory arenas, we can attempt to grow the arena by allocating a new block and linking it to the current arena
            struct ArenaBlock *newBlock = (struct ArenaBlock *)malloc(sizeof(struct ArenaBlock));
            if (!newBlock)
            {
                return NULL; // Failed to allocate new block
            }

            // If the requested size is larger than the current block size, a new block can be allocated that is large enough to contain the requested size, otherwise we can just allocate a block of the same size as the current block
            size_t newSize = size > arena->currentBlock->size ? size : arena->currentBLock->size;

            newBlock->buffer = (char *)malloc(newSize);
            if (!newBlock->buffer)
            {
                free(newBlock);
                return NULL; // Failed to allocate buffer for new block
            }

            newBlock->size = newSize;
            newBlock->offset = 0;
            newBlock->prev = arena->currentBlock;

            arena->currentBlock = newBlock;
#endif
            break;
        }
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

#if ARENA_USE_VIRTUAL_MEMORY
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
#else
struct TempArena BeginTempArena(struct MemoryArena *arena)
{
    // Create a temporary arena for the current arena state
    struct TempArena temp;
    temp.arena = arena;
    temp.startBlock = arena->currentBlock;
    temp.offset = arena->currentBlock ? arena->currentBlock->offset : 0;
    return temp;
}

void EndTempArena(struct TempArena temp)
{
    MemoryArena *arena = temp.arena;
    struct ArenaBlock *block = arena->currentBlock;

    while (block != temp.startBlock)
    {
        struct ArenaBlock *prev = block->prev;
        free(block->buffer);
        free(block);
        block = prev;
    }

    if (block)
    {
        block->offset = temp.offset;
        if (arena)
        {
            arena->currentBlock = block;
        }
    }
}

#endif