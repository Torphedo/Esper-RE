cmake_minimum_required(VERSION 3.13)
project(Esper-RE
	VERSION 0.6.7
	LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
# set(CMAKE_C_FLAGS "-mtiny")

add_subdirectory("ext/bobtail")
include_directories("ext/bobtail" ${Esper-RE_SOURCE_DIR})

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
target_link_libraries(alr PRIVATE bobtail)
target_link_libraries(deck_reader PRIVATE bobtail)
target_link_libraries(ssb PRIVATE bobtail)

# Copy ALR files to [build]/src/bin/ so they can be accessed while debugging. 
file(COPY "alr/bin/" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)

