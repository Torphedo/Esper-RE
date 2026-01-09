cmake_minimum_required(VERSION 3.13)
project(Esper-RE
    LANGUAGES C CXX
)
# We need compile_commands.json for the include paths in some editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

set(CMAKE_C_STANDARD 11)

# Polaris needs a ton of extra dependencies
include_directories(
    lib
    lib/glad/include
    lib/cglm/include
    lib/imgui
    lib/coregui/lib/glfw/include
    lib/bobtail
    ${Esper-RE_SOURCE_DIR}/src
)

set(BOBTAIL_OPENGL ON)
add_subdirectory(lib/bobtail)
add_subdirectory(lib/nfde)

add_library(miniaudio STATIC lib/miniaudio/miniaudio.c)
target_include_directories(miniaudio PUBLIC lib/miniaudio)
target_compile_definitions(miniaudio PUBLIC
    MA_NO_DEVICE_IO MA_ENABLE_ONLY_SPECIFIC_BACKENDS
    MA_NO_THREADING MA_NO_NODE_GRAPH MA_NO_RESOURCE_MANAGER
    MA_NO_GENERATION MA_NO_ENCODING MA_NO_FLAC
)

add_library(ibxm STATIC lib/micromod/ibxm-ac/ibxm.c)
target_include_directories(ibxm PUBLIC lib/micromod/ibxm-ac)

add_library(pd_common STATIC
    src/formats/pd_common.c
)
target_compile_definitions(pd_common PUBLIC PD_COMMON_HAVE_MINIAUDIO_IBXM)
target_link_libraries(pd_common PUBLIC bobtail ibxm miniaudio)

# Build Polaris
include(polaris.cmake)

add_executable(deck_reader
    src/deck_reader/main.c
)

add_executable(ssb
    src/ssb/main.c
)

add_executable(ddsx
    src/ddsx/main.c
)

# Link everything to the common library
target_link_libraries(deck_reader PRIVATE bobtail)
target_link_libraries(ssb PRIVATE pd_common m)
target_link_libraries(ddsx PRIVATE bobtail)
# Polaris handles its own linking

# Copy test files to [build]/src/bin/ so they can be accessed while debugging
file(COPY "bin" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
