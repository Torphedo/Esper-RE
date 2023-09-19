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
    u16 options_count = sizeof(arguments) / sizeof(char*);
    for (u32 i = 1; i < argc; i++) {
        // Check if first 2 characters are "--"
        if (argv[i][0] == 0x2D && argv[i][1] == 0x2D) {
            for (u16 j = 0; j < options_count; j++) {
                if (strlen(argv[i]) == strlen(arguments[j])) {
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
        }
        else {
            output.filename = argv[i];
            if (argc == 2) {
                LOG_MSG(warning, "No action specified, defaulting to --split\n");
                output.split = true;
            }
            continue;
        }
    }

    return output;
}

