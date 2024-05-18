#include <string.h>

#include "logging.h"
#include "int.h"
#include "arguments.h"

flags parse_arguments(int argc, char** argv, char* args[], u32 args_count) {
    flags output = {
        .mode = 0 // Default mode is the first option
    };

    // Use default when no arguments are provided
    if (argc == 1) {
        return output;
    }

    // Loop over every argument, skipping the first which is just our program
    // name.
    for (u32 i = 1; i < argc; i++) {
        // Check if first 2 characters are "--"
        if (argv[i][0] != '-' || argv[i][1] != '-') {
            // If it's not a flag, it must be a filename
            if (output.input_path != NULL) {
                if (output.output_path != NULL) {
                    output.input_path = output.output_path;
                }
                output.output_path = argv[i];
            } else {
                output.input_path = argv[i];
            }

            continue;
        }

        // Disable all output on silent mode
        if (strcmp(argv[i], "--silent") == 0) {
            disable_logging();
            break;
        }

        // Check for flags the caller asked us to look for
        for (u32 j = 0; j < args_count; j++) {
            if (strcmp(argv[i], args[j]) == 0) {
                output.mode = j;
                break;
            }
        }
    }

    return output;
}
