#include <string.h>

#include "logging.h"

#include "int_shorthands.h"
#include "arguments.h"

flags parse_arguments(int argc, char** argv) {
    flags output = {
        .mode = split // Default mode is split
    };

    // Print help when no arguments are provided
    if (argc == 1) {
        output.mode = help;
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

        if (strcmp(argv[i], "--dump") == 0) {
            output.mode = dumptex;
        } else if (strcmp(argv[i], "--replace") == 0) {
            output.mode = replacetex;
        } else if (strcmp(argv[i], "--info") == 0) {
            output.mode = info_only;
        } else if (strcmp(argv[i], "--silent") == 0) {
            output.silent = true;
        }
        // This seems redundant because split is the default. But if it's set
        // after another option, it should override that one.
        else if (strcmp(argv[i], "--split") == 0) {
            output.mode = split;
        } else if (strcmp(argv[i], "--animation") == 0) {
            LOG_MSG(warning, "Animation dumping is unimplemented right now.\n");
            output.mode = animation;
        } else if (strcmp(argv[i], "--version") == 0) {
            output.mode = version;
        } else if (strcmp(argv[i], "--help") == 0) {
            output.mode = help;
        }
    }

    return output;
}
