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

    if (options.split) {
        interface = split_interface;
    }

    if (options.replace) {
        return !(alr_edit(options.filename, "out.alr", options, replace_interface));
    }

    // Parse ALR with selected interface.
    // Return value must be inverted because stdbool false == 0, and an exit
    // code of 0 means success.
    return !(alr_parse(options.filename, options, interface));
}

