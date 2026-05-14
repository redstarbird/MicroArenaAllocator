cmake_minimum_required(VERSION 3.10)
project(ArenaAllocator VERSION 0.1.0 LANGUAGES C)

# --- Build Options ---
option(FORCE_MALLOC_FALLBACK "Force the arena to use standard malloc instead of virtual memory" OFF)
option(BUILD_32_BIT "Compile the library as a 32-bit architecture" OFF)

if(FORCE_MALLOC_FALLBACK)
    add_compile_definitions(ARENA_USE_VIRTUAL_MEMORY=0)
endif()

if(BUILD_32_BIT)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -m32")
    endif()
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Enable warnings for GCC and Clang
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -Werror -g -fsanitize=address,undefined -fno-omit-frame-pointer")
endif()


# -----------------------------
# Build the MemoryArena library
# -----------------------------
# Define the source files for the MemoryArena library
add_library(MemoryArena STATIC src/MemoryArena.c)

# Add the additional source files for the MemoryArena library
target_sources(MemoryArena
    PRIVATE
        src/StringPool.c
        src/Hash.c
)

# Include the header files for the MemoryArena library
target_include_directories(MemoryArena PUBLIC include)

# -----------------------------
# Testing setup
# -----------------------------

include(CTest)
enable_testing()

add_executable(TestMemoryArena tests/testArena.c)

# Link the MemoryArena library to the test executable
target_link_libraries(TestMemoryArena PRIVATE MemoryArena)

add_test(NAME CoreArenaTests COMMAND TestMemoryArena)