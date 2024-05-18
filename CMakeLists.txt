cmake_minimum_required(VERSION 3.16) # Latest version on apt
project(Esper-RE
	VERSION 0.6.6
	LANGUAGES C
)

set(CMAKE_C_STANDARD 99)

add_executable(alr
    "alr/src/main.c"
    "alr/src/alr.c"
    "alr/src/dump.c"
    "alr/src/split.c"
    "alr/src/replace.c"
    "alr/src/images.c"

    "common/arguments.c"
    "common/logging.c"
    "common/filesystem.c"
)

add_executable(deck_reader
    "common/logging.c"
    "deck_reader/src/main.c"
)

include_directories(${Esper-RE_SOURCE_DIR})

# Copy ALR files to [build]/src/bin/ so they can be accessed while debugging. 
file(COPY "alr/bin/" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)

