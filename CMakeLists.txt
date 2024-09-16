# 3.11 required for FetchContent to download depedencies
cmake_minimum_required(VERSION 3.11)
project(Esper-RE
	VERSION 0.6.7
	LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_FLAGS "-mtiny")

# Download common C library on CMake generation
include(FetchContent)
FetchContent_Declare(
    bobtail
    GIT_REPOSITORY https://github.com/Torphedo/bobtail.git
    GIT_TAG        4deb3c70fee355549b826052b40432c350310245
)
FetchContent_MakeAvailable(bobtail)
include_directories(${bobtail_SOURCE_DIR} ${Esper-RE_SOURCE_DIR})

add_executable(alr
    "alr/src/main.c"
    "alr/src/alr.c"
    "alr/src/dump.c"
    "alr/src/split.c"
    "alr/src/replace.c"
    "alr/src/images.c"

    "formats/pd_common.c"
)

add_executable(deck_reader
    "deck_reader/src/main.c"
)

add_executable(ssb
    "ssb/src/main.c"
    "ssb/src/ssb.c"
    "formats/pd_common.c"
)

# Link everything to the common library
target_link_libraries(alr PRIVATE common)
target_link_libraries(deck_reader PRIVATE common)
target_link_libraries(ssb PRIVATE common)

# Copy ALR files to [build]/src/bin/ so they can be accessed while debugging. 
file(COPY "alr/bin/" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)

