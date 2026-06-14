cmake_minimum_required(VERSION 3.13)
project(Esper-RE
    LANGUAGES C CXX
)
# We need compile_commands.json for the include paths in some editors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")

# Put all binaries we'll need to distribute in 1 folder
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Force shared libraries to load from a relative path instead of hardcoding
# their path on my computer (why is this the default?)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)
set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)


set(CMAKE_C_STANDARD 11)

# Polaris needs a ton of extra dependencies
include_directories(
    lib
    lib/glad/include
    lib/cglm/include
    lib/imgui
    lib/coregui/lib/glfw/include
    lib/bobtail
    lib/it2play
    ${Esper-RE_SOURCE_DIR}/src
)

set(BOBTAIL_OPENGL ON)
add_subdirectory(lib/bobtail)
add_subdirectory(lib/nfde)

add_library(miniaudio STATIC lib/miniaudio/miniaudio.c)
target_include_directories(miniaudio PUBLIC lib/miniaudio)
target_compile_definitions(miniaudio PUBLIC
    MA_NO_NODE_GRAPH MA_NO_RESOURCE_MANAGER
    MA_NO_GENERATION MA_NO_ENCODING MA_NO_FLAC
)

# Link this for Miniaudio with tracker module support
add_subdirectory(lib/ma_trackerFormats)
add_library(ma_trackerFormats STATIC
    lib/ma_trackerFormats/ibxm_reader.c
    lib/ma_trackerFormats/miniaudio_ibxm.c
    lib/ma_trackerFormats/miniaudio_it2play.c
)
target_link_libraries(ma_trackerFormats PUBLIC miniaudio ibxm IT2)
target_include_directories(miniaudio PUBLIC lib/ma_trackerFormats lib/ma_trackerFormats/lib/it2play)

add_library(pd_common STATIC
    src/formats/pd_common.c
)
target_link_libraries(pd_common PUBLIC bobtail)

add_executable(betaRip
    src/polaris/alr/alr_file.cxx
    src/polaris/alr/alr_dump.cxx
    src/polaris/util/scope_timer.cxx
    src/betaRip/main.cxx
)
target_link_libraries(betaRip PUBLIC pd_common bobtail)
target_include_directories(betaRip PUBLIC src/polaris)

# Build Polaris
include(polaris.cmake)

# We only need to explictly link with the math library (just "m") on Linux.
set(MATH_LIB "m")
if(MSVC)
    set(MATH_LIB "")
endif()

add_executable(deck_reader src/deck_reader/main.c)
add_executable(ssb src/ssb/main.c)
add_executable(ddsx src/ddsx/main.cxx)

# Link everything to the common library
target_link_libraries(deck_reader PRIVATE bobtail)
target_link_libraries(ssb PRIVATE pd_common ${MATH_LIB})
target_link_libraries(ddsx PRIVATE bobtail ${MATH_LIB})
# Polaris handles its own linking

# Copy test files to [build]/src/bin/ so they can be accessed while debugging
file(COPY "bin" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
