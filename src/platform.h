#ifndef PLATFORM_H
#define PLATFORM_H

#if defined(_WIN32) || defined(_WIN64)

// Exclude rarely-used bloat from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Windows-specific memory alloc/free functions for the arena using VirtualAlloc on Windows for page-aligned zero-initialized memory
#define ARENA_SYS_ALLOC(size) VirtualAlloc(NULL, (size), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
#define ARENA_SYS_FREE(ptr, size) VirtualFree((ptr), 0, MEM_RELEASE)

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)

#include <sys/mman.h>

// Define MAP_ANONYMOUS if it's not defined (some systems use MAP_ANON instead)
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

// POSIX-specific memory alloc/free functions for the arena using mmap on POSIX systems for page-aligned zero-initialized memory
#define ARENA_SYS_ALLOC(size) mmap(NULL, (size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
#define ARENA_SYS_FREE(ptr, size) munmap((ptr), (size))

#else
// Fallback to standard malloc/free if no platform-specific virtual memory functions are available

// Non-page aligned memory allocation using calloc for zero-initialized memory, this is not ideal but allows for the arena to function on platforms without virtual memory support
#define ARENA_SYS_ALLOC(size) calloc(size)
#define ARENA_SYS_FREE(ptr, size) free(ptr)

#endif

#endif // !PLATFORM_H