#ifndef MEMORYARENA_H
#define MEMORYARENA_H

// Include stddef.h for size_t and offsetof
#include <stddef.h>
// Include string.h for memset and memcpy
#include <string.h>
// For printf
#include <stdio.h>

// Detect if virtual memory functions are available on the current platform
#if defined(_WIN32) || defined(_WIN64) || defined(__linux__) || defined(__unix__) || defined(__APPLE__)
#define ARENA_USE_VIRTUAL_MEMORY 1
#else
#define ARENA_USE_VIRTUAL_MEMORY 0
#endif

typedef struct MemoryArena MemoryArena;

#if ARENA_USE_VIRTUAL_MEMORY
struct TempArena
{
    struct MemoryArena *arena;
    size_t offset;
};
#else
// Forward declaration of ArenaBlock for the non-virtual memory implementation
struct ArenaBlock;

struct TempArena
{
    struct MemoryArena *arena;
    struct ArenaBlock *startBlock;
    size_t offset;
};
#endif

// Out of memory handling policies for the arena
enum oomPolicy
{
    OOM_GROW_ARENA,
    OOM_ABORT,
    OOM_RETURN_NULL,
    OOM_CALLBACK
};

struct MemoryArena *CreateArena(size_t size, enum oomPolicy policy, void (*oomCallback)(struct MemoryArena *, size_t));
void *arenaAllocAlign(struct MemoryArena *arena, size_t size, size_t alignment);
void DestroyArena(struct MemoryArena *arena);
void OutputArenaStats(struct MemoryArena *arena);
struct TempArena BeginTempArena(struct MemoryArena *arena);
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
#define KB(x) ((size_t)(x) * 1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)
#define TB(x) (GB(x) * 1024)

// Type safe allocation macros

// Non-intialising allocation macros (unsafe but faster in some cases)

// Allocates a single struct with no initialization (unsafe but faster in some cases)
#define PushStructNoInit(arena, type) \
    ((type *)arenaAllocAlign((arena), sizeof(type), _Alignof(type)))

// Allocate an array of structs with no initialization (unsafe but faster in some cases)
#define PushArrayNoInit(arena, type, count) \
    ((type *)arenaAllocAlign((arena), sizeof(type) * (count), _Alignof(type)))

// Push a raw block of unaligned memory with no initialisation (unsafe but faster in some cases)
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

// Allocates a single struct with zero initialization
#define PushStruct(arena, type) \
    ((type *)arenaAllocAlignZero((arena), sizeof(type), _Alignof(type)))

// Allocate an array of structs with zero initialization
#define PushArray(arena, type, count) \
    ((type *)arenaAllocAlignZero((arena), sizeof(type) * (count), _Alignof(type)))

// Push a raw block of unaligned memory, zero initialized
#define PushSize(arena, size) \
    arenaAllocAlignZero((arena), (size), 1)

// Pushes raw data into the arena (useful for strings and other non-struct data)
#define PushData(arena, data, size) \
    arenaPushDataHelper((arena), (data), (size))

#endif // MEMORYARENA_H