add_executable(alr_parser
	"main.c"
	"parsers.c"
	"images.c"
    "arguments.c"
)
target_include_directories(alr_parser PRIVATE
		"../external/src"
		"../external/vendor/logging"
)
