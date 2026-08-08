#ifndef STRINGPOOL_H
#define STRINGPOOL_H
/** *
 * @file StringPool.h
 * @brief A high-performance string interning pool for efficient storage and retrieval of unique strings.
 * The StringPool uses a memory arena for efficient memory management and a hash table for fast lookups.
 * It is designed to be used in performance-critical applications where many duplicate strings may exist, such as in game engines or compilers.
 */

#include "MicroArena.h"

/** *
 * @brief Represents a view into a string stored in the StringPool. Contains a pointer to the string data and its length.
 * The string data is owned by the StringPool and should not be modified or freed by the user.
 */
typedef struct StringView
{
    /** @brief Read-only: Pointer to the string data */
    const char *data;
    /** @brief Read-only: Length of the string */
    size_t length;
} StringView;

/** *
 * @brief A StringPool is a data structure that stores unique strings and allows for efficient retrieval of string views. It uses a memory arena for efficient memory management and a hash table for fast lookups.
 * The StringPool is designed to be used in performance-critical applications where many duplicate strings may exist, such as in game engines or compilers.
 */
typedef struct StringPool
{
    /** @brief Read-only: Internal memory arena used for storing interned strings */
    MemoryArena *arena;

    /** @brief Read-only: Array of StringView structures representing the interned strings */
    StringView *strings;

    /** @brief Read-only: Number of strings currently interned in the pool */
    size_t count;

    /** @brief Read-only: Capacity of the strings array */
    size_t capacity;
} StringPool;

/** *
 * @brief Used to configure the creation of a string pool. This is passed to the `CreateStringPool` function.
 */
typedef struct StringPoolConfig
{
    /** @brief The initial number of strings the pool can hold */
    size_t stringCount;

    /** @brief The size of the internal memory arena */
    size_t arenaSize;

    /** @brief The out-of-memory policy to use (what the pool should do when full) */
    enum oomPolicy policy;

    /** @brief A callback function to be called when an out-of-memory condition occurs, only if the `oomPolicy` is set to `OOM_CALLBACK` */
    void (*oomCallback)(struct MemoryArena *, size_t)
} StringPoolConfig;

/** *
 * @brief Creates a new string pool with the specified parameters.
 * @param stringCount The initial number of strings the pool can hold.
 * @param arenaSize The size of the internal memory arena.
 * @param policy The out-of-memory policy to use.
 * @param oomCallback A callback function to be called when an out-of-memory condition occurs.
 * @return A pointer to the newly created string pool, or NULL if creation fails.
 */
struct StringPool *CreateStringPool(size_t stringCount, size_t arenaSize, enum oomPolicy policy, void (*oomCallback)(struct MemoryArena *, size_t));

/** *
 * @brief Interns a string in the given string pool.
 * @param pool The string pool in which to intern the string.
 * @param str The string to intern.
 * @param length The length of the string.
 * @return A view into the interned string.
 */
StringView InternString(struct StringPool *pool, const char *str, size_t length);

/** *
 * @brief Interns a formatted string in the given string pool.
 * @param pool The string pool in which to intern the formatted string.
 * @param format The format string (printf-style) for the string to intern.
 * @param ... The variable arguments corresponding to the format specifiers in the format string.
 * @return A view into the interned formatted string.
 */
struct StringView InternStringFormat(struct StringPool *pool, const char *format, ...);

/** *
 * @brief Destroys the given string pool and frees all associated memory.
 * @param pool The string pool to destroy.
 */
void DestroyStringPool(struct StringPool *pool);

#endif // !STRINGPOOL_H
