#ifndef HASH_H
#define HASH_H

#define HASH_FNV1A 0
#define HASH_SIPHASH 1
#define HASH_WYHASH 2

// Default to wyhash hashing for the string pool
#ifndef STRING_POOL_HASH_TYPE
#define STRING_POOL_HASH_TYPE HASH_WYHASH
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#if STRING_POOL_HASH_TYPE == HASH_FNV1A

uint32_t HashString_FNV1a32(const char *str, size_t length);
#define HashString(str, length) HashString_FNV1a32((str), (length))

#elif STRING_POOL_HASH_TYPE == HASH_SIPHASH

uint64_t Hash_SipHash24(const void *src, size_t src_sz);
#define HashString(str, length) Hash_SipHash24((str), (length))

#elif STRING_POOL_HASH_TYPE == HASH_WYHASH

uint64_t Hash_Wyhash(const void *key, size_t len, uint64_t seed);
#define HashString(str, length) Hash_Wyhash((str), (length), 0)

#endif

#endif // !HASH_H