#ifndef MEMORYARENA_H
#define MEMORYARENA_H

#include <stddef.h>

typedef struct MemoryArena MemoryArena;

struct MemoryArena *CreateArena(size_t size);
void *arenaAllocAlign(struct MemoryArena *arena, size_t size, size_t alignment);
void DestroyArena(struct MemoryArena *arena);

#endif // MEMORYARENA_H