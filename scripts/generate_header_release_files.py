import re
import os
import zipfile

# Header files
HEADERS = [
    "include/MemoryArena.h",
    "include/StringPool.h",
    "src/platform.h",
    "src/Hash.h"
]

SOURCES = [
    "src/Hash.c",
    "src/MemoryArena.c",
    "src/StringPool.c"
]

BUILD_FOLDER = "release_build"

HEADER_NAME = "MemoryArena.h"

SOURCE_NAME = "MemoryArena.c"

# Removes local includes
def remove_local_includes(text):
    return re.sub("#include\s+\"[^\"]+\"", "", text)

def open_and_remove_includes(filepath):
    with open(filepath, 'r') as f:
        return remove_local_includes(f.read())

def generate():
    # Make dir if not exists
    os.makedirs(BUILD_FOLDER, exist_ok=True)

    # Build header content string with each header seperated by two newlines
    header_content = ""
    for header in HEADERS:
        header_content += (open_and_remove_includes(header) + "\n\n") 

    # Build source content string in the same way as the headers
    source_content = ""
    for source in SOURCES:
        source_content += (open_and_remove_includes(source) + "\n\n") 

    # Create header and source pair
    with open(f"{BUILD_FOLDER}/{HEADER_NAME}", "w") as f:
        f.write(header_content)

    with open(f"{BUILD_FOLDER}/{SOURCE_NAME}", "w") as f:
        f.write(f"#include \"{HEADER_NAME}\"")
        f.write(source_content)

    # Zip source file and header file pair
    with zipfile.ZipFile("release_build/MicroArenaSource.zip", mode="w") as zip:
        zip.write(f"{BUILD_FOLDER}/{HEADER_NAME}", HEADER_NAME)
        zip.write(f"{BUILD_FOLDER}/{SOURCE_NAME}", SOURCE_NAME)

    # Create STB-style single header
    with open(f"{BUILD_FOLDER}/{HEADER_NAME}", "w") as f:
        # Inject comment message at the top
        f.write("/* Memory Arena */\n")
        f.write("/*In one C/C++ file, do: #define MEMORY_ARENA_IMPLEMENTATION*/\n")

        # Header content
        f.write("\n\n\n#ifndef MEMORY_ARENA_H\n")
        f.write("#define MEMORY_ARENA_H\n")
        f.write(header_content)
        f.write("#endif // MEMORY_ARENA_H")

        f.write("\n\n/* ========================================================= */\n")
        f.write("/* IMPLEMENTATION */\n")
        f.write("/* ========================================================= */\n\n")
        
        f.write("#ifdef MEMORY_ARENA_IMPLEMENTATION\n\n")
        f.write(source_content)
        f.write("#endif\n")

if __name__ == "__main__":
    generate()
    print("Successfully generated source and header release files")