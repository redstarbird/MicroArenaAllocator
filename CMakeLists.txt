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


# -----------------------------
# Build the MicroArena library
# -----------------------------
# Define the source files for the MicroArena library
add_library(MicroArena STATIC src/MicroArena.c)

# Add the additional source files for the MicroArena library
target_sources(MicroArena
    PRIVATE
        src/StringPool.c
        src/Hash.c
)

# Include the header files for the MicroArena library
target_include_directories(MicroArena PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Enable warnings for GCC and Clang
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(MicroArena PRIVATE -Wall -Wextra -pedantic -g)

    if(ARENA_ENABLE_ASAN)
        target_compile_options(MicroArena PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(MicroArena PRIVATE -fsanitize=address,undefined)
    endif()
endif()

# -----------------------------
# Testing setup
# -----------------------------
if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    enable_testing()

    # Option to test the generated release files from the "scripts/generate_header_release_files.py" script
    option(TEST_RELEASE_ARTIFACTS "Build and run tests for generated release files" OFF)

    add_executable(TestMicroArena tests/testArena.c)

    # Link the MicroArena library to the test executable
    target_link_libraries(TestMicroArena PRIVATE MicroArena)

    add_test(NAME CoreArenaTests COMMAND TestMicroArena)

    if (TEST_RELEASE_ARTIFACTS)

        # Create test for amalgamated header/source pair
        add_executable(TestAmalgamated tests/testArena.c ${CMAKE_CURRENT_SOURCE_DIR}/release_build/header_and_source/MicroArena.c)
        target_include_directories(TestAmalgamated PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/release_build/header_and_source)
        target_compile_definitions(TestAmalgamated PRIVATE TEST_SOURCE_HEADER)
        add_test(NAME AmalgamatedTests COMMAND TestAmalgamated)

        # Create test for single header
        add_executable(TestSingleHeader tests/testArena.c)
        target_include_directories(TestSingleHeader PRIVATE ${CMAKE_SOURCE_DIR}/release_build/single_header)
        target_compile_definitions(TestSingleHeader PRIVATE TEST_SINGLE_HEADER)
        add_test(NAME SingleHeaderTests COMMAND TestSingleHeader)

        # Apply appropriate sanitisers and warnings and stuff
        if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
            # Compile warnings
            target_compile_options(TestAmalgamated PRIVATE -Wall -Wextra -pedantic -g)
            target_compile_options(TestSingleHeader PRIVATE -Wall -Wextra -pedantic -g)

            # Sanitisers (if options enabled)
            if(ARENA_ENABLE_ASAN)
                target_compile_options(TestAmalgamated PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
                target_link_options(TestAmalgamated PRIVATE -fsanitize=address,undefined)
                target_compile_options(TestSingleHeader PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
                target_link_options(TestSingleHeader PRIVATE -fsanitize=address,undefined)
            endif()
        endif()

    endif()

endif()