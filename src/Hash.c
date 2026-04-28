#include "Hash.h"

// Computes the FNV-1a hash of the given string with a specified length and returns a 32-bit hash value
uint32_t HashStringFNV1a_32(const char *str, size_t length)
{
    const uint32_t FNV_OFFSET_BASIS = 2166136261U;
    const uint32_t FNV_PRIME = 16777619U;

    uint32_t hash = FNV_OFFSET_BASIS;

    for (size_t i = 0; i < length; i++)
    {
        // XOR the byte with the hash
        hash ^= (uint8_t)str[i];
        // Multiply by the FNV prime
        hash *= FNV_PRIME;
    }

    return hash;
}