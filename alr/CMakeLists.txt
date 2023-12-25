cmake_minimum_required(VERSION 3.16) # Latest version on apt
project(alr
	VERSION 0.6.0
	LANGUAGES C
)

set(CMAKE_C_STANDARD 99)

add_executable(alr
    "src/main.c"
    "src/alr.c"
    "src/dump.c"
    "src/split.c"
    "src/images.c"
    "src/arguments.c"

    "src/logging.c"
    "src/filesystem.c"
)

# Copy ALR files to [build]/src/bin/ so they can be accessed while debugging. 
file(COPY "bin/" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/bin)

