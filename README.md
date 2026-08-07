# ⚡ Micro arena allocator (v0.1.0-beta)

[![C/C++ CI Pipeline](https://github.com/redstarbird/MicroArenaAllocator/actions/workflows/ci.yml/badge.svg)](https://github.com/redstarbird/MicroArenaAllocator/actions/workflows/ci.yml)
![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)
![GitHub release (latest by date)](https://img.shields.io/github/v/release/redstarbird/MicroArenaAllocator)
![GitHub last commit](https://img.shields.io/github/last-commit/redstarbird/MicroArenaAllocator)

An extremely fast, extremely cross-platform, zero-dependency Memory Arena and String Pool library for C/C++

Designed for game engines, compilers, and high-performance systems, this library provides $O(1)$ allocations, completely eliminating the overhead and memory fragmentation of standard `malloc` and `free`.

## 🔥 Key Features

- **Zero-Cost Allocations:** Allocations take a single CPU cycle thanks to simple pointer addition.
- **Universal Cross-Platform:** Perfectly aligns memory across 64-bit (Windows, macOS, Linux), 32-bit (Raspberry Pi, legacy, etc), and Apple Silicon.
- **Virtual Memory Backend:** Uses `mmap` (POSIX) and `VirtualAlloc` (Windows) for instant, massive address space reservation without physical RAM commit.
- **Malloc Fallback:** Seamlessly degrades to standard heap chaining on systems without Virtual Memory, this allows for extreme backwards compatibility
- **Scoped Memory (TempArena):** Instantly rewind and garbage-collect temporary allocations without any memory leaks.
- **String Pool Subsystem:** High-performance string interning, deduplication, and variadic `printf` formatting.

## 🚀 Installation

MicroArena can be installed in multiple ways to fit your exact setup and workflow. The primary distributions are using CMake's FetchContent, Git Submodules, a single header file, and a header/source pair. The most recommended ways are to use FetchContent or Git submodules. If you are not using CMake or Git, the distributed release files can be used instead.

### Option 1: CMake FetchContent (Recommended)

This is the cleanest and easiest way to integrate the library if you are using CMake already. Simply add this to your CMakeLists.txt:

```CMake

include(FetchContent)

# Declare dependency with specific release
FetchContent_Declare(
    MicroArena
    GIT_REPOSITORY https://github.com/redstarbird/MicroArenaAllocator.git
    GIT_TAG v0.1.0-beta # Use the release you want to install, latest is recommended
)

# Download dependency and add to the build
FetchContent_MakeAvailable(MicroArena)
```

Then link it to your target (replace `YourExecutable` with the name of your target/executable):

```CMake
target_link_libraries(YourExecutable PRIVATE MicroArena)
```

### Option 2: Git Submodule

This way is ideal if you are not using CMake or if you want to use a specific commit of the library.

#### 1. Add the Git submodule

```bash
git submodule add https://github.com/redstarbird/MicroArenaAllocator.git external/MicroArena
```

#### 2. Link with CMake

```CMake
# Add the submodule
add_subdirectory(external/MicroArena)

# Link it to your executable
add_executable(YourProgram src/main.c)
target_link_libraries(YourProgram PRIVATE MicroArena)
```

#### 3. Build

```bash
# Generate the build files
cmake -B build

# Compile the project
cmake --build build --config Release
```

### Single header

If you prefer to use a STB-style library/header, you can use the single header distribution from the GitHub release of your choice.

#### 1. Add the file to your project

Download the `MicroArena.h` file from the GitHub release of your choice, and simply put it into your project.

#### 2. Define implementation macro in one file

In **exactly** one C/C++ file, define the implementation macro before including the header:

```c
#define MICRO_ARENA_IMPLEMENTATION
#include "MicroArena.h"
```

In all other files, just use `#include "MicroArena.h"` as normal.

#### ⚠️ Single-header warning

⚠️ If using a single header release, the string pool functionality will be built into the `MicroArena.h` file rather than a seperate header. ⚠️

### Header/source pair

The header source pair distribution contains a single header `MicroArena.h` containing definitions, and a `MicroArena.c` file containing the implementation. This method allows you to not have to define `MICRO_ARENA_IMPLEMENTATION` which can be preferable in large projects.

#### 1. Add the files to your project

Download the MicroArena.zip file from the GitHub release of your choice, extract the files, and place them in your project.

#### 2. Compile the files directly alongside your other source files

#### ⚠️ Header/source warning

⚠️ If using a header/source pair release, the string pool functionality will be built into the `MicroArena.h` and `MicroArena.c` files rather than a seperate header. ⚠️

## 💻 Basic Usage Examples

### 1. Basic Allocation

```c
#include <MemoryArena.h>

int main() {
    // Reserve a 16MB virtual memory arena (aborts on Out-of-Memory)
    struct MemoryArena* arena = CreateArena(MB(16), OOM_ABORT, NULL);

    // O(1) Allocations
    float* positions = PushArray(arena, float, 1000);
    Entity* player = PushStruct(arena, Entity);

    // Instantly free all 1001 allocations at once
    DestroyArena(arena);
    return 0;
}
```

### 2. Temporary Memory (Zero-Cost Garbage Collection)

```c
// Use a temporary arena for each frame
void ProcessFrame(struct MemoryArena* globalArena) {
    // Start a temporary scope
    struct TempArena temp = BeginTempArena(globalArena);

    // Do heavy math, allocate temporary matrices, etc. ...
    Matrix* mathBuffer = PushArray(globalArena, Matrix, 500);

    // Instantly rewind the arena. The mathBuffer memory is safely freed
    // and ready to be overwritten in the next frame
    EndTempArena(temp);
}
```

### 3. String Interning

```c
#include <StringPool.h>

void LoadAssets(struct StringPool* pool) {
    // Safely formats and deduplicates the string instantly
    struct StringView tex1 = InternStringFormat(pool, "textures/%s_%d.png", "player", 1);
    struct StringView tex2 = InternStringFormat(pool, "textures/%s_%d.png", "player", 1);

    // Both tex1.data and tex2.data will point to the exact same memory address thanks to interning
}
```

## 🧵 Thread Safety ⚠️

By design, `MicroMemoryArena` is not thread safe.
This library is designed to be as efficient as possible so having locking mechanisms such as OS mutexes or atomic hardware locks would kill the $O(1)$ allocation speed.

### Best practice for multi-threading

To safely use multi-threading with this library, do not share a single arena across multiple threads/workers. Instead, use [Thread-Local Storage](https://en.wikipedia.org/wiki/Thread-local_storage) to give a separate, private, lock-free arena to each thread.

```c
// Thread locking example using _Thread_local
_Thread_local struct MemoryArena* threadArena = NULL;

void WorkerThread() {
    if (!threadArena) {
        threadArena = CreateArena(MB(16), OOM_ABORT, NULL);
    }
    // Safe, effecient parallel allocations
}
```

### Future Implementation

Thread safety may be added in a future release of this library.

## 🛠️ Configuration Options

You can customize the library's behavior when configuring your CMake project using the following options:

| CMake option          | Default | Description                                                                         |
| --------------------- | ------- | ----------------------------------------------------------------------------------- |
| FORCE_MALLOC_FALLBACK | OFF     | Forces the use of fall-back `malloc` block-chaining instead of Virtual Memory.      |
| BUILD_32_BIT          | OFF     | Compiles the library in 32-bit mode                                                 |
| ARENA_ENABLE_ASAN     | OFF     | (Internal) Enables AddressSanitizer, shouldn't be used when linking to this library |

## License

This project is licensed under the MIT License - see the [LICENSE](https://github.com/redstarbird/MicroArenaAllocator/blob/master/LICENSE) file for details.
