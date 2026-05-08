#include "Hash.h"

// --------------------------------
// FNV-1a Hash Implementation
// --------------------------------

#if STRING_POOL_HASH_TYPE == HASH_FNV1A
#include <stdio.h>
// Computes the FNV-1a hash of the given string with a specified length and returns a 32-bit hash value
uint32_t HashString_FNV1a32(const char *str, size_t length)
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

#endif

// --------------------------------
// SipHash Implementation
// --------------------------------

#if STRING_POOL_HASH_TYPE == HASH_SIPHASH
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

// Windows-specific includes for cryptographic random number generation to create a random key for SipHash
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#include <bcrypt.h>
// Link against bcrypt.lib for Windows
#pragma comment(lib, "bcrypt.lib")
#endif

// Global key for SipHash, randomly generated at startup
uint8_t SiphashKey[16];

// Generates a random 16-byte key for SipHash using platform-specific secure random number generation functions
void GenerateSipHashKey()
{
#if defined(_WIN32) || defined(_WIN64)
    NTSTATUS status = BCryptGenRandom(NULL, SiphashKey, 16, BCRYPT_USE_SYSTEM_PREFERRED_RNG);

    if (status != 0)
    {
        fprintf(stderr, "Fatal: Failed to generate random key for SipHash!\n");
        abort();
    }

#else
    // On POSIX systems, read from /dev/urandom to get a random key for SipHash
    FILE *urandom = fopen("/dev/urandom", "rb");
    if (urandom == NULL)
    {
        fprintf(stderr, "Fatal: Failed to open /dev/urandom for SipHash key generation!\n");
        abort();
    }

    size_t bytesRead = fread(SiphashKey, 1, 16, urandom);
    fclose(urandom);

    if (bytesRead != 16)
    {
        fprintf(stderr, "Fatal: Failed to read enough random bytes for SipHash key generation\n");
        abort();
    }
#endif
}

// --- Helper: Rotate Left ---
#define SIP_ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

// --- SipRound Macro ---
#define SIPROUND               \
    do                         \
    {                          \
        v0 += v1;              \
        v1 = SIP_ROTL(v1, 13); \
        v1 ^= v0;              \
        v0 = SIP_ROTL(v0, 32); \
        v2 += v3;              \
        v3 = SIP_ROTL(v3, 16); \
        v3 ^= v2;              \
        v0 += v3;              \
        v3 = SIP_ROTL(v3, 21); \
        v3 ^= v0;              \
        v2 += v1;              \
        v1 = SIP_ROTL(v1, 17); \
        v1 ^= v2;              \
        v2 = SIP_ROTL(v2, 32); \
    } while (0)

// SipHash-2-4
// The key must be exactly 16 bytes (128 bits).
uint64_t Hash_SipHash24(const void *src, size_t src_sz)
{
    const uint8_t *ni = (const uint8_t *)src;
    const uint8_t *end = ni + (src_sz & ~7);

    // Read the two 64-bit halves of the key
    uint64_t k0, k1;
    memcpy(&k0, SiphashKey, 8);
    memcpy(&k1, SiphashKey + 8, 8);

    // Initialize state
    uint64_t v0 = k0 ^ 0x736f6d6570736575ULL;
    uint64_t v1 = k1 ^ 0x646f72616e646f6dULL;
    uint64_t v2 = k0 ^ 0x6c7967656e657261ULL;
    uint64_t v3 = k1 ^ 0x7465646279746573ULL;

    uint64_t m;
    int leftover = src_sz & 7;
    uint64_t b = ((uint64_t)src_sz) << 56;

    // Process 64-bit blocks
    for (; ni != end; ni += 8)
    {
        memcpy(&m, ni, 8);
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }

    // Process leftovers
    switch (leftover)
    {
    case 7:
        b |= ((uint64_t)ni[6]) << 48; // fallthrough
    case 6:
        b |= ((uint64_t)ni[5]) << 40; // fallthrough
    case 5:
        b |= ((uint64_t)ni[4]) << 32; // fallthrough
    case 4:
        b |= ((uint64_t)ni[3]) << 24; // fallthrough
    case 3:
        b |= ((uint64_t)ni[2]) << 16; // fallthrough
    case 2:
        b |= ((uint64_t)ni[1]) << 8; // fallthrough
    case 1:
        b |= ((uint64_t)ni[0]);
        break;
    case 0:
        break;
    }

    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;

    // Finalization
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;

    return v0 ^ v1 ^ v2 ^ v3;
}

#endif

// --------------------------------
// Wyhash Implementation
// --------------------------------

#if STRING_POOL_HASH_TYPE == HASH_WYHASH

// --- Compiler-Specific 128-bit Multiplication ---
#if defined(_MSC_VER) && defined(_M_X64)
#include <intrin.h>
#pragma intrinsic(_umul128)
#endif

// The core mixing function of wyhash
static inline uint64_t wymix(uint64_t A, uint64_t B)
{
#if defined(__SIZEOF_INT128__)
    // GCC / Clang native 128-bit math
    __uint128_t r = A;
    r *= B;
    return (uint64_t)r ^ (uint64_t)(r >> 64);
#elif defined(_MSC_VER) && defined(_M_X64)
    // MSVC 64-bit intrinsic
    uint64_t a, b;
    a = _umul128(A, B, &b);
    return a ^ b;
#else
    // 32-bit fallback math (Slow but portable)
    uint64_t ha = A >> 32, la = (uint32_t)A;
    uint64_t hb = B >> 32, lb = (uint32_t)B;
    uint64_t rh = ha * hb, rm0 = ha * lb, rm1 = hb * la, rl = la * lb;
    uint64_t t = rl + (rm0 << 32), c = t < rl;
    uint64_t lo = t + (rm1 << 32);
    c += lo < t;
    uint64_t hi = rh + (rm0 >> 32) + (rm1 >> 32) + c;
    return lo ^ hi;
#endif
}

// Helper to read memory safely without unaligned access crashes
static inline uint64_t wyr8(const uint8_t *p)
{
    uint64_t v;
    memcpy(&v, p, 8);
    return v;
}
static inline uint64_t wyr4(const uint8_t *p)
{
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}
static inline uint64_t wyr3(const uint8_t *p, size_t k)
{
    return (((uint64_t)p[0]) << 16) | (((uint64_t)p[k >> 1]) << 8) | p[k - 1];
}

// wyhash
uint64_t Hash_Wyhash(const void *key, size_t len, uint64_t seed)
{
    const uint8_t *p = (const uint8_t *)key;
    uint64_t a, b;

    // Secret primes used by the algorithm
    uint64_t secret[4] = {
        0xa0761d6478bd642fULL, 0xe7037ed1a0b428dbULL,
        0x8ebc6af09c88c6e3ULL, 0x589965cc75374cc3ULL};

    seed ^= secret[0];

    if (len <= 16)
    {
        if (len >= 4)
        {
            a = (wyr4(p) << 32) | wyr4(p + ((len >> 3) << 2));
            b = (wyr4(p + len - 4) << 32) | wyr4(p + len - 4 - ((len >> 3) << 2));
        }
        else if (len > 0)
        {
            a = wyr3(p, len);
            b = 0;
        }
        else
        {
            a = b = 0;
        }
    }
    else
    {
        size_t i = len;
        if (i > 48)
        {
            uint64_t see1 = seed, see2 = seed;
            do
            {
                seed = wymix(wyr8(p) ^ secret[1], wyr8(p + 8) ^ seed);
                see1 = wymix(wyr8(p + 16) ^ secret[2], wyr8(p + 24) ^ see1);
                see2 = wymix(wyr8(p + 32) ^ secret[3], wyr8(p + 40) ^ see2);
                p += 48;
                i -= 48;
            } while (i > 48);
            seed ^= see1 ^ see2;
        }
        while (i > 16)
        {
            seed = wymix(wyr8(p) ^ secret[1], wyr8(p + 8) ^ seed);
            i -= 16;
            p += 16;
        }
        a = wyr8(p + i - 16);
        b = wyr8(p + i - 8);
    }
    return wymix(secret[1] ^ len, wymix(a ^ secret[1], b ^ seed));
}

#endif