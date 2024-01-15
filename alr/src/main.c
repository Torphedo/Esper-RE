#include <stdio.h>

#include "logging.h"

#include "alr.h"
#include "dump.h"
#include "split.h"
#include "replace.h"
#include "arguments.h"

int main(int argc, char* argv[]) {
    // Parse command-line arguments.
    flags options = parse_arguments(argc, argv);

    if (options.filename == NULL) {
        LOG_MSG(error, "No filenames detected.\n");
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
            return !(alr_edit(options.filename, "out.alr", options, replace_interface));
            break;
    }

    // Parse ALR with selected interface.
    // Return value must be inverted because stdbool false == 0, and an exit
    // code of 0 means success.
    return !(alr_parse(options.filename, options, interface));
}

