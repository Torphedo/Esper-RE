cmake_minimum_required(VERSION 3.16)
project(deck_reader
	VERSION 0.2.0
	LANGUAGES C
)

set(CMAKE_C_STANDARD 99)

add_executable(deck_reader
	"src/main.c"
)

