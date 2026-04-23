#ifndef MEMORYARENA_H
#define MEMORYARENA_H

#include <stddef.h>

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

#endif // MEMORYARENA_H