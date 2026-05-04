#include "StringPool.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include "Hash.h"

struct StringPool *CreateStringPool(size_t stringCount, size_t arenaSize, enum oomPolicy policy, void (*oomCallback)(struct MemoryArena *, size_t))
{
    // If initial size is not provided, default to 2 KB
    if (stringCount == 0)
    {
        stringCount = KB(8);
    }

    // Ensure the initial size is a multiple of 2 for efficient memory alignment
    assert((stringCount & (stringCount - 1)) == 0 && "String count must be a power of 2");

    // Ensure the arena size is large enough to hold the specified number of string views
    assert((stringCount * sizeof(struct StringView)) < arenaSize && "Arena size must be large enough to hold the specified number of string views");

    // Allocate memory for the StringPool struct
    struct StringPool *pool = (struct StringPool *)malloc(sizeof(struct StringPool));
    if (!pool)
    {
        return NULL;
    }

    // Create the internal memory arena for the string pool
    pool->arena = CreateArena(arenaSize, policy, oomCallback);
    if (!pool->arena)
    {
        free(pool);
        return NULL;
    }

    // Initialize the StringPool fields
    pool->count = 0;
    pool->capacity = stringCount;

    // Allocate memory for the StringView hash
    // pool->strings = (struct StringView *)arenaAllocAlign(pool->arena, sizeof(struct StringView) * stringCount, _alignof(struct StringView));
    printf("Allocating memory for %zu string views\n", stringCount);
    pool->strings = PushArray(pool->arena, struct StringView, stringCount);

    printf("Alocated memory for %zu string views at address: %p\n", stringCount, (void *)pool->strings);

    return pool;
}

struct StringView *InternString(struct StringPool *pool, const char *str, size_t length)
{
    if (!pool || !str || length == 0)
    {
        StringView *empty = {0};
        return empty;
    }

    // Calulate the hash of the input string
    uint32_t hash = HashStringFNV1a_32(str, length);

    // Calculate the index in the hash table using fast modulo (capacity must be a power of 2)
    size_t index = hash & (pool->capacity - 1);

    // Linear probing to occupy an empty slot or find a matching string
    while (1)
    {
        struct StringView *slot = &pool->strings[index];

        // If the slot is empty, a new string can be inserted here
        if (slot->data == NULL)
        {
            char *newText = (char *)PushData(pool->arena, str, length + 1);
            if (!newText)
            {
                return &(struct StringView){0};
            }

            newText[length] = '\0'; // Null-terminate the string

            slot->data = newText;
            slot->length = length;
            pool->count++;

            return slot;
        }

        // If the slot is occupied, check if the existing string matches the input string
        if (slot->length == length && memcmp(slot->data, str, length) == 0)
        {
            // The string already exists in the pool, return the existing StringView
            return slot;
        }

        // If the slot is occupied but does not match, continue probing to the next index (wrap around using bitwise AND)
        index = (index + 1) & (pool->capacity - 1);
    }
}

// Destroys the provided string pool and frees all associated memory
void DestroyStringPool(struct StringPool *pool)
{
    if (pool && pool->arena)
    {
        DestroyArena(pool->arena);
        pool->arena = NULL;
        free(pool);
    }
}