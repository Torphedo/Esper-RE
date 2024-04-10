#include <stdio.h>

#include "logging.h"

#include "alr.h"
#include "dump.h"
#include "split.h"
#include "replace.h"
#include "arguments.h"

int main(int argc, char* argv[]) {
    enable_win_ansi(); // Enable color on Windows
    // Parse command-line arguments.
    flags options = parse_arguments(argc, argv);

    if (options.input_path == NULL) {
        LOG_MSG(error, "No ALR to operate on, exiting\n");
        return 1;
    }
    if (options.silent) {
        disable_logging();
    }

    // Default to dump behaviour if nothing is specified. No arguments probably
    // means someone drag-and-dropped, which means they probably want textures.
    alr_interface interface = dump_interface;

    switch (options.mode) {
        case info_only:
            // Stops all output
            interface = stub_interface;
            break;
        case split:
            interface = split_interface;
            break;
        case dumptex:
            interface = dump_interface;
            break;
        case animation:
            LOG_MSG(error, "Unimplemented program mode, exiting.\n");
            return 1;
        case replacetex:
            if (options.output_path == NULL) {
                LOG_MSG(error, "No output file provided.\n");
                return 1;
            }
            return !(alr_edit(options.input_path, options.output_path, options, replace_interface));
            break;
    }

    // Parse ALR with selected interface.
    // Return value must be inverted because stdbool false == 0, and an exit
    // code of 0 means success.
    return !(alr_parse(options.input_path, options, interface));
}

