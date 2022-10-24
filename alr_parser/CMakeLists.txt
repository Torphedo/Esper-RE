cmake_minimum_required(VERSION 3.16)
project(alr_parser
	VERSION 0.2.0
	LANGUAGES C
)

set(CMAKE_C_STANDARD 99)

add_subdirectory(src)

# Copy binary ALR files to [build]/src/bin/ so that they can be accessed during debugging easily.
# It would be better to just change the working directory to /alr_parser/.
file(COPY "bin/" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/src/bin)
