#include <string.h>

#include <logging.h>

#include "arguments.h"

static const char* arguments[] = {
        "--split",
        "--info",
        "--dump",
        "--silent",
        "--tga",
        "--dds",
        "--layout",
        "--animation"
};

flags parse_arguments(int argc, char** argv) {
    flags output = {0};
    uint16_t options_count = sizeof(arguments) / sizeof(char*);
    for (uint32_t i = 1; i < argc; i++) {
        // Check if first 2 characters are "--"
        if (argv[i][0] == 0x2D && argv[i][1] == 0x2D) {
            for (uint16_t j = 0; j < options_count; j++) {
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
                                output.tga = true;
                                break;
                            case 5:
                                output.dds = true;
                                break;
                            case 6:
                                output.layout = true;
                                break;
                            case 7:
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
                log_error(WARNING, "parse_arguments(): No action specified, defaulting to --split\n");
                output.split = true;
            }
            continue;
        }
    }

    return output;
}
