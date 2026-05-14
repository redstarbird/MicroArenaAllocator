#ifndef STRINGPOOL_H
#define STRINGPOOL_H

#include "MemoryArena.h"
typedef struct StringView
{
    const char *data;
    size_t length;
} StringView;

typedef struct StringPool
{
    // Internal memory arena used for storing interned strings
    MemoryArena *arena;

    // Array of StringView structures representing the interned strings
    StringView *strings;

    // Number of strings currently interned in the pool
    size_t count;

    // Capacity of the strings array
    size_t capacity;
} StringPool;

struct StringPool *CreateStringPool(size_t stringCount, size_t arenaSize, enum oomPolicy policy, void (*oomCallback)(struct MemoryArena *, size_t));
StringView InternString(struct StringPool *pool, const char *str, size_t length);
void DestroyStringPool(struct StringPool *pool);
struct StringView InternStringFormat(struct StringPool *pool, const char *format, ...);

#endif // !STRINGPOOL_H
