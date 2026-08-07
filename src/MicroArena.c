#include "MicroArena.h"

#include <stdlib.h>
#include <stdint.h>

#include "platform.h"

#if ARENA_USE_VIRTUAL_MEMORY
struct MemoryArena
{
    char *buffer;
    // Maximum virtual memory capacity reserved for the arena
    size_t reservedSize;
    // Currently commited size of the arena
    size_t committedSize;

    // Current offset in the arena for the next allocation
    size_t offset;

    // Peak offset reached in the arena, used for tracking maximum usage
    size_t peakOffset;

    // Pointer to the previous arena block, used for linked list of blocks for OOM_GROW_ARENA policy
    struct MemoryArena *prev;

    // Out of memory handling policy for the arena
    enum oomPolicy oomPolicy;
    // Optional callback function for OOM handling when the policy is OOM_CALLBACK
    void (*oomCallback)(struct MemoryArena *arena, size_t requestedSize);
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
    enum oomPolicy oomPolicy;
    // Optional callback function for OOM handling when the policy is OOM_CALLBACK
    void (*oomCallback)(struct MemoryArena *arena, size_t requestedSize);
};

#endif

// Outputs the current stats of the memory arena, including total size, used space, peak usage, and free space
void OutputArenaStats(struct MemoryArena *arena)
{
    printf("Memory Arena Stats:\n");

#if ARENA_USE_VIRTUAL_MEMORY
    // Virtual memory implementation
    printf("Allocated Size: %zu bytes\n", arena->committedSize);
    if (arena->oomPolicy == OOM_GROW_ARENA)
    {
        printf("Reserved Size: %zu bytes\n", arena->reservedSize);
    }
    printf("Used: %zu bytes\n", arena->offset);
    printf("Peak Usage: %zu bytes\n", arena->peakOffset);
    printf("Free: %zu bytes\n", arena->committedSize - arena->offset);
#else
    // Non-virtual memory implementation
    size_t totalSize = 0;
    size_t usedSize = 0;
    struct ArenaBlock *block = arena->currentBlock;

    while (block)
    {
        totalSize += block->size;
        usedSize += block->offset;
        block = block->prev;
    }

    printf("Total Size: %zu bytes\n", totalSize);
    printf("Used: %zu bytes\n", usedSize);
    printf("Peak Usage: %zu bytes\n", arena->peakOffset);
    printf("Free: %zu bytes\n", totalSize - usedSize);
#endif

    // Print the OOM policy of the arena
    printf("OOM Policy: ");
    switch (arena->oomPolicy)
    {
    case OOM_RETURN_NULL:
        printf("Return NULL\n");
        break;
    case OOM_ABORT:
        printf("Abort\n");
        break;
    case OOM_CALLBACK:
        printf("Callback\n");
        break;
    case OOM_GROW_ARENA:
        printf("Grow Arena\n");
        break;
    default:
        printf("Invalid\n");
        break;
    }
}

#if ARENA_USE_VIRTUAL_MEMORY
struct MemoryArena *CreateArena(size_t size, enum oomPolicy policy, void (*oomCallback)(struct MemoryArena *, size_t))
{
#if defined(_M_X64) || defined(__x86_64__)
    // On 64-bit platforms, an extremely large maximum virtual memory capacity can be reserved
    size_t actualReservedSize = (policy == OOM_GROW_ARENA) ? TB(1) : size;
#else
    // For 32-bit platforms (such as WebAssembly), reserve a smaller maximum virtual memory capacity for the arena
    size_t actualReservedSize = (policy == OOM_GROW_ARENA) ? GB(1) : size;
#endif

    size_t osPageSize = ArenaGetOSPageSize();

    // Reserve virtual memory for the arena, this uses no physical memory yet
    void *reservedMemory = ARENA_SYS_RESERVE(osPageSize + actualReservedSize);
    if (!reservedMemory)
    {
        return NULL;
    }

    // Commit the initial size of memory for the arena, this will allocate physical memory for the committed size
    ARENA_SYS_COMMIT(reservedMemory, osPageSize);

    // Allocate memory for the MemoryArena struct
    struct MemoryArena *arena = (struct MemoryArena *)reservedMemory;
    if (!arena)
    {
        return NULL;
    }

    // Set the buffer pointer to the memory immediately following the first page reserved for the MemoryArena struct
    arena->buffer = (char *)reservedMemory + osPageSize;

    // Initialize the MemoryArena fields
    arena->committedSize = 0;
    arena->offset = 0;
    arena->peakOffset = 0;
    arena->oomPolicy = policy;
    arena->oomCallback = oomCallback;
    arena->reservedSize = actualReservedSize;
    arena->prev = NULL;

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

    // Set the OOM policy and callback for the arena
    arena->oomPolicy = policy;
    arena->oomCallback = oomCallback;

    // Initialize the first block of the arena
    arena->currentBlock = (struct ArenaBlock *)ARENA_SYS_ALLOC(sizeof(struct ArenaBlock));
    if (!arena->currentBlock)
    {
        ARENA_SYS_FREE(arena, sizeof(struct MemoryArena));
        return NULL;
    }

    arena->currentBlock->buffer = (char *)ARENA_SYS_ALLOC(size);
    if (!arena->currentBlock->buffer)
    {
        ARENA_SYS_FREE(arena->currentBlock, sizeof(struct ArenaBlock));
        ARENA_SYS_FREE(arena, sizeof(struct MemoryArena));
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
    if (!arena)
    {
        return;
    }
#if ARENA_USE_VIRTUAL_MEMORY
    // Walk through the linked list of virtual memory blocks and release each one
    struct MemoryArena *current = arena;
    while (current != NULL)
    {
        struct MemoryArena *prev = current->prev;

        size_t osPageSize = ArenaGetOSPageSize();
        ARENA_SYS_RELEASE(current, osPageSize + current->reservedSize);
        current = prev;
    }
#else
    // Walk through the linked list of memory allocated blocks and free each one
    struct ArenaBlock *current = arena->currentBlock;
    while (current != NULL)
    {
        struct ArenaBlock *prev = current->prev;
        // Free the buffer for the current block and then free the block itself
        ARENA_SYS_FREE(current->buffer, current->size);
        ARENA_SYS_FREE(current, sizeof(struct ArenaBlock));

        current = prev;
    }
    ARENA_SYS_FREE(arena, sizeof(struct MemoryArena));
#endif
}

void *arenaAllocAlign(struct MemoryArena *arena, size_t size, size_t alignment)
{
#if ARENA_USE_VIRTUAL_MEMORY
    // Calculate the aligned address using bitwise operations
    uintptr_t currentAddress = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
#else
    uintptr_t currentAddress = (uintptr_t)arena->currentBlock->buffer + (uintptr_t)arena->currentBlock->offset;
#endif
    uintptr_t alignedAddress = (currentAddress + alignment - 1) & ~(alignment - 1);

    // Calculate the padding needed to achieve the aligned address
    size_t padding = alignedAddress - currentAddress;
#if ARENA_USE_VIRTUAL_MEMORY
    size_t newOffset = arena->offset + padding + size;
#else
    size_t newOffset = arena->currentBlock->offset + padding + size;
#endif

// If the new offset exceeds the arena's reserved size, handle based on the arena's OOM policy
#if ARENA_USE_VIRTUAL_MEMORY
    if (newOffset > arena->reservedSize)
#else
    if (newOffset > arena->currentBlock->size)
#endif
    {
        // Handle out of memory based on the arena's OOM policy
        switch (arena->oomPolicy)
        {
        case OOM_RETURN_NULL:
            return NULL;
        case OOM_ABORT:
#if ARENA_USE_VIRTUAL_MEMORY
            fprintf(stderr, "Out of memory in arena allocation. Requested size: %zu bytes, available: %zu, reserved size: %zu, OOM policy: %d\n", size, arena->committedSize - arena->offset, arena->reservedSize, arena->oomPolicy);
#else
            fprintf(stderr, "Out of memory in arena allocation. Requested size: %zu bytes, current block size: %zu, OOM policy: %d\n", size, arena->currentBlock->size, arena->oomPolicy);
#endif
            abort();
        case OOM_CALLBACK:
            if (arena->oomCallback)
            {
                arena->oomCallback(arena, size);
            }

            // Return NULL after the callback, as the callback is expected to handle the OOM situation
            return NULL;

        case OOM_GROW_ARENA:
        {
#if ARENA_USE_VIRTUAL_MEMORY

#if defined(_M_X64) || defined(__x86_64__)
            // On 64-bit platforms the 1TB reservation acts as the grow logic
            // If the arena is already at this maximum 1TB reservation size, it cannot grow anymore so return NULL

            // Cannot grow anymore, return NULL
            // Ensure the caller handles this case properly to avoid unexpected behavior.
            return NULL;
#else
            // For 32-bit platforms, we can grow the arena by reserving a new block of virtual memory and linking it to the current arena
            MemoryArena *nextBlock = CreateArena(arena->reservedSize, OOM_GROW_ARENA, arena->oomCallback);
            if (!nextBlock)
            {
                return NULL; // Failed to create new block
            }

            struct MemoryArena *oldArena = (struct MemoryArena *)ARENA_SYS_ALLOC(sizeof(struct MemoryArena));
            // Copy the current arena state to the old arena
            *oldArena = *arena;

            // Update the current arena to state of the new block, this effectively makes the new block the current arena
            *arena = *nextBlock;

            // Update the current arena to the new block
            arena->prev = oldArena;

            // Free the temporary next block struct
            ARENA_SYS_FREE(nextBlock, sizeof(struct MemoryArena) + nextBlock->reservedSize);

            currentAddress = (uintptr_t)arena->buffer + (uintptr_t)arena->offset;
#endif

#else
            // For non-virtual memory arenas, Allocate a new block and link it to the current arena
            struct ArenaBlock *newBlock = (struct ArenaBlock *)ARENA_SYS_ALLOC(sizeof(struct ArenaBlock));
            if (!newBlock)
            {
                // Failed to allocate new block
                return NULL;
            }

            // If the requested size is larger than the current block size, a new block can be allocated that is large enough to contain the requested size, otherwise we can just allocate a block of the same size as the current block
            size_t newSize = size > arena->currentBlock->size ? size : arena->currentBlock->size;

            if (newSize == 0)
            {
                ARENA_SYS_FREE(newBlock, sizeof(struct ArenaBlock));
                return NULL; // Invalid size, return NULL
            }
            newBlock->buffer = (char *)ARENA_SYS_ALLOC(newSize);
            if (!newBlock->buffer)
            {
                ARENA_SYS_FREE(newBlock, sizeof(struct ArenaBlock));
                return NULL; // Failed to allocate buffer for new block
            }

            newBlock->size = newSize;
            newBlock->offset = 0;
            newBlock->prev = arena->currentBlock;

            arena->currentBlock = newBlock;

            currentAddress = (uintptr_t)arena->currentBlock->buffer + (uintptr_t)arena->currentBlock->offset;
#endif // !ARENA_USE_VIRTUAL_MEMORY
       // After handling the OOM situation, recalculate the aligned address and padding for the new block

            alignedAddress = (currentAddress + alignment - 1) & ~(alignment - 1);
            padding = alignedAddress - currentAddress;
#if ARENA_USE_VIRTUAL_MEMORY
            newOffset = arena->offset + padding + size;
#else
            newOffset = arena->currentBlock->offset + padding + size;
#endif

        } // Extra scope for OOM_GROW_ARENA case (C11 and older C standards do not allow declarations in switch cases without extra scope)
        }
    }

#if ARENA_USE_VIRTUAL_MEMORY
    // Commit more memory if needed for the new offset
    if (newOffset > arena->committedSize)
    {
        while (newOffset > arena->committedSize)
        {
            size_t commitSize = arena->committedSize + MB(8);
            if (commitSize > arena->reservedSize)
            {
                commitSize = arena->reservedSize;
            }

            // Commit additional memory
            ARENA_SYS_COMMIT(arena->buffer + arena->committedSize, commitSize - arena->committedSize);

            arena->committedSize = commitSize;
        }
    }
#endif

// Move the offset to the aligned address
#if ARENA_USE_VIRTUAL_MEMORY
    arena->offset += padding + size;
#else
    arena->currentBlock->offset += padding + size;
#endif

#if ARENA_USE_VIRTUAL_MEMORY
    // Update the peak offset if necessary
    if (arena->offset > arena->peakOffset)
    {
        arena->peakOffset = arena->offset;
    }
#else
    // Update the peak offset in non-virtual memory mode
    size_t totalUsed = 0;
    struct ArenaBlock *block = arena->currentBlock;
    while (block)
    {
        totalUsed += block->offset;
        block = block->prev;
    }
    if (totalUsed > arena->peakOffset)
    {
        arena->peakOffset = totalUsed;
    }
#endif

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
        ARENA_SYS_FREE(block->buffer, block->size);
        ARENA_SYS_FREE(block, sizeof(struct ArenaBlock));
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
