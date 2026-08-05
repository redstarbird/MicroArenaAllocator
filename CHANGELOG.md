# Changelog

Any features, notable changes and bug fixes will be found in the file.
Based off of [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)

## [0.1.0-beta] - Unreleased

### Added

- **Initial beta release**
- **Core memory arena management**: Memory arena's can be effectively created and destroyed using `CreateArena` and `DestroyArena` respectively
- **Data sizing macros**: Easily create arena's of specific sizes without having to write large numbers (`KB`, `MB`, `GB`, `TB`)
- **Uninitialised macros**: High performance uninitialised allocation macros for faster, uninitialized memory blocks (`PushStructNoInit`, `PushArrayNoInit`, `PushSizeNoInit`)
- **Zero-initialised macros**: Safe, Zero-initialised allocation macros for safer memory allocation (`PushStruct`, `PushArray`, `PushSize`)
- **Raw data copying**: Raw data copying macro for strings and raw byte blocks (`PushData`)
- **Virtual Memory Backend**: Direct OS integration via `mmap` (POSIX) and `VirtualAlloc` (Windows) for zero-cost virtual address reservation
- **Malloc Fallback Backend**: Seamlessly falls back to chaining standard heap blocks on systems without Virtual Memory
- **Dynamic OS Page Sizing**: Queries the OS at runtime to ensure perfect physical RAM alignment across x86 (4KB), Apple Silicon (16KB), and WebAssembly (64KB)
- **Out of Memory (OOM) Policies**: Flexible error handling via `OOM_GROW_ARENA`, `OOM_ABORT`, `OOM_RETURN_NULL`, and custom `OOM_CALLBACK` functions
- **32-Bit VM Fallback**: Custom struct-swapping logic to chain 1GB virtual memory blocks when addressable space is exhausted on 32-bit systems
- **TempArena Architecture**: Zero-cost, stack-based trackers for scoped memory rewinding without calling `free()` (`BeginTempArena`, `EndTempArena`)
- **String Pool System**: High-performance string interning and deduplication using custom hash tables.
- **Variadic String Formatting**: `InternStringFormat` allows `printf`-style string generation directly into the arena without memory leaks, utilizing `TempArena` for dynamic scaling
- **Universal Compiler Alignment Macro**: Advanced macro fallbacks for `_Alignof` ensuring compatibility from modern C11 compilers all the way back to legacy C89
