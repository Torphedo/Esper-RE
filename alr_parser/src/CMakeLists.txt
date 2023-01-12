add_executable(alr_parser
	"main.c"
	"parsers.c"
	"images.c"
	"../external/src/arena.c"
	"../external/vendor/logging/logging.c"
)
target_include_directories(alr_parser PRIVATE
		"../external/src"
		"../external/vendor/logging"
)
