#ifndef MEMORYARENA_H
#define MEMORYARENA_H

// Include stddef.h for size_t and _Alignof
#include <stddef.h>
// Include string.h for memset and memcpy
#include <string.h>

typedef struct MemoryArena MemoryArena;

struct MemoryArena *CreateArena(size_t size);
void *arenaAllocAlign(struct MemoryArena *arena, size_t size, size_t alignment);
void DestroyArena(struct MemoryArena *arena);

// ------
// Macros
// ------

// Sizing helper macros
#define KB(x) ((size_t)(x) * 1024)
#define MB(x) (KB(x) * 1024)
#define GB(x) (MB(x) * 1024)

// Type safe allocation macros

// Non-intializing allocation macros (unsafe but faster in some cases)

// Allocates a single struct with no initialization (unsafe but faster in some cases)
#define PushStructNoInit(arena, type) \
    ((type *)arenaAllocAlign((arena), sizeof(type), _Alignof(type)))

// Allocate an array of structs with no initialization (unsafe but faster in some cases)
#define PushArrayNoInit(arena, type, count) \
    ((type *)arenaAllocAlign((arena), sizeof(type) * (count), _Alignof(type)))

// Push a raw block of unaligned memory with no initialization (unsafe but faster in some cases)
#define PushSizeNoInit(arena, size) \
    arenaAllocAlign((arena), (size), 1)

//
// Initializing allocation macros (safe but slower in some cases)
//

// Allocates a single struct with zero initialization
#define PushStruct(arena, type) \
    ((type *)memset(PushStructNoInit((arena), type), 0, sizeof(type)))

// Allocate an array of structs with zero initialization
#define PushArray(arena, type, count) \
    ((type *)memset(PushArrayNoInit((arena), type, (count)), 0, sizeof(type) * (count)))

// Push a raw block of unaligned memory
#define PushSize(arena, size) \
    ((type *)memset(PushSizeNoInit(arena, size), 0, (size)))

// Pushes raw data into the arena (useful for strings and other non-struct data)
#define PushData(arena, data, size) \
    memcpy(PushArrayNoInit((arena), char, (size)), (data), (size))

#endif // MEMORYARENA_H