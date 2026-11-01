#if defined(TEST_SINGLE_HEADER)

#define MICRO_ARENA_IMPLEMENTATION
#include "MicroArena.h"

#elif defined(TEST_SOURCE_HEADER)

#include "MicroArena.h"

#else
#include "../include/MicroArena.h"
#include "../include/StringPool.h"
#endif

// Asserts whether a condition is true and outputs a provided error message if not
#define ASSERT_TRUE(condition, message)                                         \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, message); \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

// Asserts whether two values are equal and outputs a provided error message if not
#define ASSERT_EQUALS(expected, actual, message)                                                                                      \
    do                                                                                                                                \
    {                                                                                                                                 \
        if ((expected) != (actual))                                                                                                   \
        {                                                                                                                             \
            fprintf(stderr, "[FAIL] %s:%d: %s (Expected %d, got %d)\n", __FILE__, __LINE__, message, (int)(expected), (int)(actual)); \
            exit(1);                                                                                                                  \
        }                                                                                                                             \
    } while (0)
