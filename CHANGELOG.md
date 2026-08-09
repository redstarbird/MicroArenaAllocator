# Changelog

Any features, notable changes and bug fixes will be found in the file.
Based off of [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

## [0.1.0-beta] - 2026-08-07

### Added

- **Initial beta release**
- **Core memory arena management**: Memory arenas can be effectively created and destroyed using `CreateArena` and `DestroyArena` respectively
- **Data sizing macros**: Easily create arenas of specific sizes without having to write large numbers (`KB`, `MB`, `GB`, `TB`)
- **Uninitialised macros**: High performance uninitialised allocation macros for faster, uninitialized memory blocks (`PushStructNoInit`, `PushArrayNoInit`, `PushSizeNoInit`)
- **Zero-initialised macros**: Safe, Zero-initialised allocation macros for safer memory allocation (`PushStruct`, `PushArray`, `PushSize`)
- **Raw data copying**: Raw data copying macro for strings and raw byte blocks (`PushData`)
- **Virtual Memory Backend**: Direct OS integration via `mmap` (POSIX) and `VirtualAlloc` (Windows) for zero-cost virtual address reservation
- **Malloc Fallback Backend**: Seamlessly falls back to chaining standard heap blocks on systems without Virtual Memory, this is done using the `FORCE_MALLOC_FALLBACK` CMake option
- **Dynamic OS Page Sizing**: Queries the OS at runtime to ensure perfect physical RAM alignment across x86 (4KB), Apple Silicon (16KB), and WebAssembly (64KB)
- **Out of Memory (OOM) Policies**: Flexible error handling via `OOM_GROW_ARENA`, `OOM_ABORT`, `OOM_RETURN_NULL`, and custom `OOM_CALLBACK` functions
- **32-Bit VM Fallback**: Custom struct-swapping logic to chain 1GB virtual memory blocks when addressable space is exhausted on 32-bit systems, activated using the `BUILD_32_BIT` CMake option
- **TempArena Architecture**: Zero-cost, stack-based trackers for scoped memory rewinding without calling `free()` (`BeginTempArena`, `EndTempArena`)
- **String Pool System**: High-performance string interning and deduplication using custom hash tables
- **Variadic String Formatting**: `InternStringFormat` allows `printf`-style string generation directly into the arena without memory leaks, utilizing `TempArena` for dynamic scaling
- **Universal Compiler Alignment Macro**: Advanced macro fallbacks for `_Alignof` ensuring compatibility from modern C11 compilers all the way back to legacy C89
- **Configurable String Hashing**: The string pool uses Wyhash by default but the user can change the hashing style for different use cases to SipHash and FNV-1a using the `STRING_POOL_HASH_TYPE` macro
- **Multiple Distribution Formats**: Flexible project integration via CMake FetchContent, Git Submodules, STB-style single header, or an amalgamated header/source pair
- **Arena Statistics**: Arena statistics are tracked and can be outputted using the `OutputArenaStats` function
