add_executable(alr
    "main.c"
    "parsers.c"
    "images.c"
    "arguments.c"
)
target_include_directories(alr PRIVATE
    "../ext/arena_allocator/src"
    "../ext/arena_allocator/vendor/logging"
)
