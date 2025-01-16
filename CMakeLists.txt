cmake_minimum_required(VERSION 3.13)
project(Esper-RE
    VERSION 0.6.7
    LANGUAGES C
)
# We need compile_commands.json for the include paths in some editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_C_STANDARD 11)

# Polaris needs a ton of extra dependencies
include_directories(
    ext
    ext/glad/include
    ext/glfw/include
    ext/cglm/include
    ext/imgui
    ext/ImGuiFileDialog
    ext/bobtail
    ${Esper-RE_SOURCE_DIR}/src
)

set(BOBTAIL_OPENGL ON)
add_subdirectory(ext/bobtail)
add_subdirectory(ext/glfw)
add_subdirectory(ext/nfde)

# Build Polaris
include(polaris.cmake)

# Build CLI ALR program
add_executable(alr
    src/alr/main.c
    src/alr/alr.c
    src/alr/dump.c
    src/alr/split.c
    src/alr/replace.c
    src/alr/images.c

    src/formats/pd_common.c
)

add_executable(deck_reader
    src/deck_reader/main.c
)

add_executable(ssb
    src/ssb/main.c
    src/ssb/ssb.c
    src/formats/pd_common.c
)

# Link everything to the common library
target_link_libraries(alr PRIVATE bobtail)
target_link_libraries(deck_reader PRIVATE bobtail)
target_link_libraries(ssb PRIVATE bobtail)
# Polaris handles its own linking

# Copy test files to [build]/src/bin/ so they can be accessed while debugging
file(COPY "bin" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)
