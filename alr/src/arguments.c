#include <string.h>

#include "logging.h"

#include "int_shorthands.h"
#include "arguments.h"

static const char* arguments[] = {
        "--split",
        "--info",
        "--dump",
        "--silent",
        "--layout",
        "--animation"
};

flags parse_arguments(int argc, char** argv) {
    flags output = {0};

    // This size is calculated at compile time
    u16 options_count = sizeof(arguments) / sizeof(char*);

    // Loop over every argument
    for (u32 i = 1; i < argc; i++) {
        // Check if first 2 characters are "--"
        if (argv[i][0] != '-' || argv[i][1] != '-') {
            // If it's not a flag, it must be a filename
            output.filename = argv[i];
            if (argc == 2) {
                LOG_MSG(warning, "No action specified, defaulting to --split\n");
                output.split = true;
            }
            continue;
        }

        // Check each argument against each valid option
        for (u16 j = 0; j < options_count; j++) {
            // Early exit if the lengths are different
            if (strlen(argv[i]) != strlen(arguments[j])) {
                continue;
            }

            bool matches = (strcmp(argv[i], arguments[j]) == 0);
            if (matches) {
                switch(j) {
                    case 0:
                        output.split = true;
                        break;
                    case 1:
                        output.info_mode = true;
                        break;
                    case 2:
                        output.dump_images = true;
                        break;
                    case 3:
                        output.silent = true;
                        break;
                    case 4:
                        output.layout = true;
                        break;
                    case 5:
                        output.animation = true;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    return output;
}

