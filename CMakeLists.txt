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
    ${Esper-RE_SOURCE_DIR}
)

set(BOBTAIL_OPENGL ON)
add_subdirectory(ext/bobtail)
add_subdirectory(ext/glfw)
add_subdirectory(ext/nfde)

# Build Polaris
add_subdirectory(polaris)

# Build CLI ALR program
add_executable(alr
    alr/src/main.c
    alr/src/alr.c
    alr/src/dump.c
    alr/src/split.c
    alr/src/replace.c
    alr/src/images.c

    formats/pd_common.c
)

add_executable(deck_reader
    deck_reader/src/main.c
)

add_executable(ssb
    ssb/src/main.c
    ssb/src/ssb.c
    formats/pd_common.c
)

# Link everything to the common library
target_link_libraries(alr PRIVATE bobtail)
target_link_libraries(deck_reader PRIVATE bobtail)
target_link_libraries(ssb PRIVATE bobtail)
# Polaris handles its own linking

# Copy ALR files to [build]/src/bin/ so they can be accessed while debugging. 
file(COPY "alr/bin/" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)
