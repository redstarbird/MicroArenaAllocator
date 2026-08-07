#if defined(TEST_SINGLE_HEADER)

#define MICRO_ARENA_IMPLEMENTATION
#include "MicroArena.h"

#elif defined(TEST_SOURCE_HEADER)

#include "MicroArena.h"

#else
#include "../include/MicroArena.h"
#include "../include/StringPool.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define ASSERT_TRUE(condition, message)                                         \
    do                                                                          \
    {                                                                           \
        if (!(condition))                                                       \
        {                                                                       \
            fprintf(stderr, "[FAIL] %s:%d: %s\n", __FILE__, __LINE__, message); \
            exit(1);                                                            \
        }                                                                       \
    } while (0)

#define ASSERT_EQUALS(expected, actual, message)                                                                                      \
    do                                                                                                                                \
    {                                                                                                                                 \
        if ((expected) != (actual))                                                                                                   \
        {                                                                                                                             \
            fprintf(stderr, "[FAIL] %s:%d: %s (Expected %d, got %d)\n", __FILE__, __LINE__, message, (int)(expected), (int)(actual)); \
            exit(1);                                                                                                                  \
        }                                                                                                                             \
    } while (0)

void TestBasicAllocation(void)
{
    struct MemoryArena *arena = CreateArena(MB(1), OOM_RETURN_NULL, NULL);
    ASSERT_TRUE(arena != NULL, "Failed to create arena");

    void *ptr1 = PushSize(arena, 100);
    ASSERT_TRUE(ptr1 != NULL, "Failed to allocate 100 bytes");
    void *ptr2 = PushSize(arena, 200);
    ASSERT_TRUE(ptr2 != NULL, "Failed to allocate 200 bytes");

    // Verify that the allocated blocks do not overlap and are in the correct order
    ASSERT_TRUE(ptr2 > ptr1, "Second allocation is not after the first allocation");
    ASSERT_TRUE((char *)ptr2 >= (char *)ptr1 + 100, "Second allocation overlaps with the first allocation");

    DestroyArena(arena);
    printf("[PASS] TestBasicAllocation\n");
}

void TestOOMReturnNull(void)
{
    struct MemoryArena *arena = CreateArena(KB(1), OOM_RETURN_NULL, NULL);
    ASSERT_TRUE(arena != NULL, "Failed to create arena");

    // Fill the arena to its capacity
    void *ptr1 = PushSize(arena, KB(1));
    ASSERT_TRUE(ptr1 != NULL, "Failed to allocate initial block");

    // This allocation should fail and return NULL due to OOM
    void *ptr2 = PushSize(arena, 1);
    ASSERT_TRUE(ptr2 == NULL, "Arena did not respect OOM_RETURN_NULL limit!");

    DestroyArena(arena);
    printf("[PASS] TestOOMReturnNull\n");
}

void TestMemoryAlignment(void)
{
    MemoryArena *arena = CreateArena(KB(4), OOM_RETURN_NULL, NULL);
    ASSERT_TRUE(arena != NULL, "Arena failed to create");

    char *charPtr = PushStruct(arena, char);

    uint64_t *uint64Ptr = PushStruct(arena, uint64_t);

    ASSERT_TRUE((((uintptr_t)uint64Ptr) % (size_t)ALIGNOF(uint64_t)) == 0, "uint64_t was not 8-byte aligned!");

    ASSERT_EQUALS((size_t)ALIGNOF(uint64_t), (char *)uint64Ptr - (char *)charPtr, "uint64_t was not placed immediately after char!");

    DestroyArena(arena);
    printf("[PASS] TestMemoryAlignment\n");
}

void TestOOMGrowChaining(void)
{
    // Create an arena with a small initial size and OOM_GROW_ARENA policy
    struct MemoryArena *arena = CreateArena(128, OOM_GROW_ARENA, NULL);
    ASSERT_TRUE(arena != NULL, "Failed to create arena");

    // Fill most of the arena to force an OOM condition
    void *ptr1 = PushSize(arena, 100);
    ASSERT_TRUE(ptr1 != NULL, "Failed to allocate initial block");

    // This allocation should trigger the OOM_GROW_ARENA policy
    void *ptr2 = PushSize(arena, 50);
    ASSERT_TRUE(ptr2 != NULL, "Failed to grow arena on OOM");

    // Verify memory integrity of the new arena by writing to the newly allocated block
    memset(ptr2, 0xAA, 50); // Fill the new block with a pattern

    // Clean up
    DestroyArena(arena);
    printf("[PASS] TestOOMGrowChaining\n");
}

void TestTempArena(void)
{
    struct MemoryArena *arena = CreateArena(MB(2), OOM_RETURN_NULL, NULL);
    ASSERT_TRUE(arena != NULL, "Failed to create arena");

    // Create a temporary arena
    struct TempArena tempArena = BeginTempArena(arena);

    // Allocate some memory in the temporary arena
    void *tempData1 = PushSize(arena, 500);
    void *tempData2 = PushSize(arena, 300);
    ASSERT_TRUE(tempData1 != NULL && tempData2 != NULL, "Failed to allocate in temporary arena!");

    // Destroy the temporary arena, which should roll back the arena state to before the temporary allocations
    EndTempArena(tempArena);

    // Push new data, this new pointer should be the same as tempData1 since the temporary arena was ended and rolled back
    void *newData = PushSize(arena, 500);

    ASSERT_TRUE(newData == tempData1, "Temp arena did not reset the offset correctly!");

    // Clean up
    DestroyArena(arena);
    printf("[PASS] TestTempArena\n");
}

void TestStringPoolInterning(void)
{
    struct StringPool *pool = CreateStringPool(512, MB(1), OOM_RETURN_NULL, NULL);
    ASSERT_TRUE(pool != NULL, "Failed to create string pool");

    // Intern a string
    StringView view1 = InternString(pool, "Hello, World!", 13);
    ASSERT_TRUE(view1.data != NULL, "Failed to intern string");

    // Intern the same string again, should return the same StringView
    StringView view2 = InternString(pool, "Hello, World!", 13);
    ASSERT_TRUE(view2.data != NULL, "Failed to intern string a second time");

    // Verify that the same StringView is returned for the duplicate string
    ASSERT_TRUE(view1.data == view2.data && view1.length == view2.length, "Interned string did not return the same StringView for duplicate string!");

    // Clean up
    DestroyStringPool(pool);
    printf("[PASS] TestStringPoolInterning\n");
}

void TestStringPoolFormatInterning(void)
{
    struct StringPool *pool = CreateStringPool(512, MB(1), OOM_RETURN_NULL, NULL);
    ASSERT_TRUE(pool != NULL, "Failed to create string pool");

    // Intern a formatted string under 256 characters, should be interned directly without using the temporary arena
    StringView view1 = InternStringFormat(pool, "Value: %d", 42);
    ASSERT_TRUE(view1.data != NULL, "Failed to intern formatted string");
    ASSERT_TRUE(strncmp(view1.data, "Value: 42", view1.length) == 0, "Formatted string content is incorrect");

    // Intern the same formatted string again, should return the same StringView
    StringView view2 = InternStringFormat(pool, "Value: %d", 42);
    ASSERT_TRUE(view2.data != NULL, "Failed to intern formatted string a second time");
    ASSERT_TRUE(view1.data == view2.data && view1.length == view2.length, "Interned formatted string did not return the same StringView for duplicate formatted string!");

    // Intern a longer formatted string that exceeds the stack buffer size, should use the temporary arena for formatting
    char longFormat[300];
    memset(longFormat, 'A', sizeof(longFormat) - 1);
    longFormat[sizeof(longFormat) - 1] = '\0';
    StringView view3 = InternStringFormat(pool, "%s", longFormat);
    ASSERT_TRUE(view3.data != NULL, "Failed to intern long formatted string");
    ASSERT_TRUE(strncmp(view3.data, longFormat, view3.length) == 0, "Long formatted string content is incorrect");

    // Clean up
    DestroyStringPool(pool);
    printf("[PASS] TestStringPoolFormatInterning\n");
}

// Global static variables for testing the callback OOM policy
static bool callbackCalled = false;
static size_t callbackRequestedSize = 0;

void callbackFunction(MemoryArena *arena, size_t requestedSize)
{
    ASSERT_TRUE(arena != NULL, "OOM callback received a NULL arena pointer");

    callbackCalled = true;
    callbackRequestedSize = requestedSize;
}

void TestOOMCallback(void)
{
    // Reset callback state
    callbackCalled = false;

    // Create an arena with a small size and OOM_CALLBACK policy
    struct MemoryArena *arena = CreateArena(KB(1), OOM_CALLBACK, callbackFunction);
    ASSERT_TRUE(arena != NULL, "Failed to create arena");

    // Push exactly the arena size to fill it up
    PushSize(arena, KB(1));

    // Request 500 bytes to trigger the OOM condition and callback
    void *ptr = PushSize(arena, 500);

    ASSERT_TRUE(ptr == NULL, "OOM_CALLLBACK policy did not return NULL on OOM");
    ASSERT_TRUE(callbackCalled, "OOM callback was not called on OOM");
    ASSERT_EQUALS(500, callbackRequestedSize, "OOM callback was called with incorrect requested size");

    // Clean up
    DestroyArena(arena);
    printf("[PASS] TestOOMCallback\n");
}

int main(void)
{
    printf("================================\n");
    printf("Running Memory Arena Unit Tests\n");
    printf("================================\n");

    TestBasicAllocation();
    TestOOMReturnNull();
    TestMemoryAlignment();
    TestOOMGrowChaining();
    TestTempArena();
    TestStringPoolInterning();
    TestStringPoolFormatInterning();
    TestOOMCallback();

    printf("================================\n");
    printf("All tests passed successfully!\n");
    return 0;
}
