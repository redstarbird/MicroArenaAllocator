#ifndef MEMORYARENA_H
#define MEMORYARENA_H
/**
 * @file MemoryArena.h
 * @brief A high-performance, cross-platform memory arena allocator.
 * * @warning By design, this memory arena is NOT thread-safe. Do not share a single
 * MemoryArena instance across multiple threads.
 * For multi-threading, create a separate arena per thread.
 */

// Include stddef.h for size_t and offsetof
#include <stddef.h>
// Include string.h for memset and memcpy
#include <string.h>
// For printf
#include <stdio.h>

// Detect if virtual memory functions are available on the current platform, if not already defined by the build system
#ifndef ARENA_USE_VIRTUAL_MEMORY

#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define ARENA_USE_VIRTUAL_MEMORY 1
#else
#define ARENA_USE_VIRTUAL_MEMORY 0
#endif

#endif

/**
 * @brief An opaque handle to a memory arena.
 * The internal layout is hidden for safety and ABI compatibility.
 */
typedef struct MemoryArena MemoryArena;

#if ARENA_USE_VIRTUAL_MEMORY
/**
 * @brief Temporary arena state used for resetting the arena back to a previous state.
 * Stores the offset in the arena to reset to, as well as a pointer to the arena itself.
 */
struct TempArena
{
    /**
     * @brief Read-only: Pointer to the arena that this temporary state belongs to. This is needed to reset the arena back to the correct state when EndTempArena is called.
     */
    struct MemoryArena *arena;
    /**
     * @brief Read-only: The offset in the arena to reset to when EndTempArena is called. This allows the arena to be reset back to this offset, effectively freeing any allocations made after this point.
     */
    size_t offset;
};
#else
// Forward declaration of ArenaBlock for the non-virtual memory implementation
struct ArenaBlock;

/**
 * @brief Temporary arena state used for resetting the arena back to a previous state.
 * Stores the offset in the arena to reset to, as well as a pointer to the arena itself.
 */
struct TempArena
{
    /**
     * @brief Read-only: Pointer to the arena that this temporary state belongs to. This is needed to reset the arena back to the correct state when EndTempArena is called.
     */
    struct MemoryArena *arena;

    /**
     * @brief Read-only: Pointer to the starting block of the arena at the time of creating this temporary state. This is needed to reset the arena back to the correct state when EndTempArena is called, as the arena may have grown with new blocks since this temporary state was created.
     */
    struct ArenaBlock *startBlock;

    /**
     * @brief Read-only: The offset in the arena to reset to when EndTempArena is called. This allows the arena to be reset back to this offset, effectively freeing any allocations made after this point.
     */
    size_t offset;
};
#endif

/**
 * @brief Defines the behaviour of the arena when an allocation cannot be fulfilled due to insufficient memory in the arena
 */
enum oomPolicy
{
    /** * @brief Dynamically allocates a new memory block and chains it to the arena.
     * @note Prevents crashes, but incurs a slight performance penalty due to OS allocation overhead.
     * When virtual memory is available, this policy will reserve a large virtual memory block (1TB on 64-bit platforms, 1GB on 32-bit platforms)
     * and commit physical memory as needed up to the reserved limit. This allows for efficient growth of the arena with minimal overhead until the reserved limit is reached.
     * If the limit is reached on a 64-bit platform, the arena cannot grow anymore and NULL will be returned. On 32-bit platforms, the arena can grow indefinitely.
     */
    OOM_GROW_ARENA,

    /** * @brief Instantly kills the program via exit(1). Ideal for critical game engine allocations
     * where failing to allocate means the game cannot continue.
     */
    OOM_ABORT,

    /** * @brief Safely returns NULL. The user is responsible for handling the failure.
     */
    OOM_RETURN_NULL,

    /** * @brief Triggers a custom user-defined function. Useful for flushing caches,
     * logging the error to a server, or triggering an emergency garbage collection.
     * NULL will be returned after the callback, so the user is expected to handle the failure after the callback as well.
     */
    OOM_CALLBACK
};

/** @brief Creates a new memory arena with the specified size and out-of-memory policy.
 * @param size The initial size of the arena.
 * @param policy The out-of-memory handling policy.
 * @param oomCallback A custom callback function to be called when an out-of-memory situation occurs (only used if policy is OOM_CALLBACK) should be a valid function pointer or NULL.
 * @return A pointer to the newly created memory arena, or NULL if creation fails.
 */
struct MemoryArena *CreateArena(size_t size, enum oomPolicy policy, void (*oomCallback)(struct MemoryArena *, size_t));

/** @brief Allocates a block of memory from the arena with the specified size and alignment.
 * @param arena The memory arena to allocate from.
 * @param size The size of the memory block to allocate.
 * @param alignment The alignment requirement for the allocated memory block.
 * @return A pointer to the allocated memory block, or NULL if allocation fails due to insufficient memory in the arena and the OOM policy is OOM_RETURN_NULL or OOM_CALLBACK.
 * In the case of an OS error during allocation (such as failure to reserve or commit memory when using virtual memory), NULL will be returned regardless of the OOM policy, as this indicates a critical failure that cannot be handled by the arena's OOM policies.
 */
void *arenaAllocAlign(struct MemoryArena *arena, size_t size, size_t alignment);

/** @brief Destroys the specified memory arena and frees all associated memory.
 * @param arena The memory arena to destroy.
 */
void DestroyArena(struct MemoryArena *arena);

/** @brief Outputs statistics about the specified memory arena.
 * @param arena The memory arena for which to output statistics.
 */
void OutputArenaStats(struct MemoryArena *arena);

/** @brief Begins a temporary allocation scope within the specified memory arena.
 * @param arena The memory arena to use for temporary allocations.
 * @return A TempArena structure handle to the temporary allocation scope.
 */
struct TempArena BeginTempArena(struct MemoryArena *arena);

/** @brief Ends a temporary allocation scope, resetting the arena back to the state it was in when BeginTempArena was called with the corresponding TempArena handle.
 * @param temp The TempArena handle representing the temporary allocation scope to end.
 */
void EndTempArena(struct TempArena temp);

#ifndef ALIGNOF
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
// 1. Standard C11 (Modern and perfectly safe)
#define ALIGNOF(x) _Alignof(x)
#elif defined(__GNUC__) || defined(__clang__)
// 2. GCC/Clang
#define ALIGNOF(x) __alignof__(x)
#elif defined(_MSC_VER)
// 3. MSVC
#define ALIGNOF(x) __alignof(x)
#else
// 4. Catch-all C89/C99 Fallback (Unsafe and may not work correctly on all platforms, but provides a fallback for older compilers)
#define ALIGNOF(type) offsetof(struct { char c; type t; }, t)
#endif
#endif

// ------
// Macros
// ------

// Sizing helper macros
/** @brief Converts kilobytes to bytes.
 * @param x The number of kilobytes.
 * @return The equivalent number of bytes.
 */
#define KB(x) ((size_t)(x) * 1024)

/** @brief Converts megabytes to bytes.
 * @param x The number of megabytes.
 * @return The equivalent number of bytes.
 */
#define MB(x) (KB(x) * 1024)

/** @brief Converts gigabytes to bytes.
 * @param x The number of gigabytes.
 * @return The equivalent number of bytes.
 */
#define GB(x) (MB(x) * 1024)

/** @brief Converts terabytes to bytes.
 * @param x The number of terabytes.
 * @return The equivalent number of bytes.
 * @note This is used internally for virtual memory but is not recommended for general use.
 */
#define TB(x) (GB(x) * 1024)

// Type safe allocation macros

// Non-intialising allocation macros (unsafe but faster in some cases)

/** @brief Allocates a single struct with no zero-initialisation. Unsafe but fast
 * @param arena The memory arena to allocate from.
 * @param type The type of the struct to allocate.
 * @return A pointer to the allocated struct, or NULL if allocation fails.
 */
#define PushStructNoInit(arena, type) \
    ((type *)arenaAllocAlign((arena), sizeof(type), ALIGNOF(type)))

/** @brief Allocates an array of structs with no zero-initialisation. Unsafe but fast
 * @param arena The memory arena to allocate from.
 * @param type The type of the struct to allocate.
 * @param count The number of elements in the array to allocate.
 * @return A pointer to the allocated array, or NULL if allocation fails.
 */
#define PushArrayNoInit(arena, type, count) \
    ((type *)arenaAllocAlign((arena), sizeof(type) * (count), ALIGNOF(type)))

/** @brief Pushes a raw block of unaligned memory with no zero-initialisation. Unsafe but fast
 * @param arena The memory arena to allocate from.
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or NULL if allocation fails.
 */
#define PushSizeNoInit(arena, size) \
    arenaAllocAlign((arena), (size), 1)

//
// Zero-initialising allocation macros (safe but slower in some cases)
//

// Internal helper function to safely zero-initialise memory only if allocation succeeded
static inline void *arenaAllocAlignZero(struct MemoryArena *arena, size_t size, size_t alignment)
{
    void *ptr = arenaAllocAlign(arena, size, alignment);
    if (ptr != NULL)
    {
        memset(ptr, 0, size);
    }
    return ptr;
}

// Internal helper function to safely copy data only if allocation succeeded
static inline void *arenaPushDataHelper(struct MemoryArena *arena, const void *data, size_t size)
{
    void *ptr = arenaAllocAlign(arena, size, 1);
    if (ptr != NULL)
    {
        memcpy(ptr, data, size);
    }
    return ptr;
}

/** @brief Allocates a single struct with zero-initialisation. Safe but slower.
 * @param arena The memory arena to allocate from.
 * @param type The type of the struct to allocate.
 * @return A pointer to the allocated struct, or NULL if allocation fails.
 */
#define PushStruct(arena, type) \
    ((type *)arenaAllocAlignZero((arena), sizeof(type), ALIGNOF(type)))

/** @brief Allocates an array of structs with zero-initialisation. Safe but slower.
 * @param arena The memory arena to allocate from.
 * @param type The type of the struct to allocate.
 * @param count The number of elements in the array to allocate.
 * @return A pointer to the allocated array, or NULL if allocation fails.
 */
#define PushArray(arena, type, count) \
    ((type *)arenaAllocAlignZero((arena), sizeof(type) * (count), ALIGNOF(type)))

/** @brief Pushes a raw block of unaligned memory, zero initialised. Safe but slower.
 * @param arena The memory arena to allocate from.
 * @param size The size of the memory block to allocate.
 * @return A pointer to the allocated memory block, or NULL if allocation fails.
 */
#define PushSize(arena, size) \
    arenaAllocAlignZero((arena), (size), 1)

/** @brief Pushes raw data into the arena (useful for strings and other non-struct data)
 * @param arena The memory arena to allocate from.
 * @param data The data to push.
 * @param size The size of the data to push.
 * @return A pointer to the allocated memory block, or NULL if allocation fails.
 */
#define PushData(arena, data, size) \
    arenaPushDataHelper((arena), (data), (size))

#endif // MEMORYARENA_H
