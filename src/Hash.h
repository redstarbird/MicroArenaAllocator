#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>

uint32_t HashStringFNV1a_32(const char *str, size_t length);

#endif // !HASH_H