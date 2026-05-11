// platform.h - Platform-specific definitions and abstractions for memory management
#ifndef PLATFORM_H
#define PLATFORM_H

#if defined(_WIN32) || defined(_WIN64)

// Exclude rarely-used bloat from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// Virtual memory functions for reserve and commit architecture
// PAGE_NOACCESS is used to reserve memory without committing actual physical memory, this allows for extremely large sizes to be reserved
#define ARENA_SYS_RESERVE(size) VirtualAlloc(NULL, (size), MEM_RESERVE, PAGE_NOACCESS)
// MEM_COMMIT is used to actually allocate the physical memory
#define ARENA_SYS_COMMIT(ptr, size) VirtualAlloc((ptr), (size), MEM_COMMIT, PAGE_READWRITE)
// Virtual memory release function
#define ARENA_SYS_RELEASE(ptr, size) VirtualFree((ptr), 0, MEM_RELEASE)

// Standard memory allocations functions (for chained blocks fallback in non-virtual memory arenas)
#define ARENA_SYS_ALLOC(size) VirtualAlloc(NULL, (size), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
#define ARENA_SYS_FREE(ptr, size) VirtualFree((ptr), 0, MEM_RELEASE)

#elif defined(__linux__) || defined(__unix__) || defined(__APPLE__)

#include <sys/mman.h>

// Define MAP_ANONYMOUS if it's not defined (some macOS/BSD systems use MAP_ANON instead)
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

// Virtual memory functions for reserve and commit architecture using mmap on POSIX systems
// PROT_NONE is used to reserve memory without committing actual physical memory, this allows for extremely large sizes to be reserved
#define ARENA_SYS_RESERVE(size) mmap(NULL, (size), PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
// PROT_READ | PROT_WRITE is used to actually allocate the physical memory with read/write permissions
#define ARENA_SYS_COMMIT(ptr, size) mprotect((ptr), (size), PROT_READ | PROT_WRITE)
// Virtual memory release function
#define ARENA_SYS_RELEASE(ptr, size) munmap((ptr), (size))

// POSIX-specific memory alloc/free functions for the arena using mmap on POSIX systems for page-aligned zero-initialised memory
#define ARENA_SYS_ALLOC(size) mmap(NULL, (size), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
#define ARENA_SYS_FREE(ptr, size) munmap((ptr), (size))

#else
// Fallback to standard calloc/free if no platform-specific virtual memory functions are available
// calloc is used to make sure memory is zero-initialised for consistent behaviour across platforms

// Fake virtual memory functions for platforms without virtual memory support
// These functions simply allocate and free memory using calloc and free
#define ARENA_SYS_RESERVE(size) calloc(1, (size))
#define ARENA_SYS_COMMIT(ptr, size) (ptr) // No actual commit step needed as memory is already allocated and zero-initialised
#define ARENA_SYS_RELEASE(ptr, size) free(ptr)

// Non-page aligned memory allocation using calloc for zero-initialised memory, this is not ideal but allows for the arena to function on platforms without virtual memory support
#define ARENA_SYS_ALLOC(size) calloc(size)
#define ARENA_SYS_FREE(ptr, size) free(ptr)

#endif

#endif // !PLATFORM_H