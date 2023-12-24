#include <stdio.h>

#include "logging.h"

#include "split.h"
#include "alr.h"
#include "dump.h"
#include "arguments.h"

// Cross-platform pause
static inline void pause() {
    LOG_MSG(info, "Press Enter to exit...");
    unsigned char dummy = getchar();
}

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
    if (options.split) {
        // Return value must be inverted because stdbool false == 0, and an
        // exit code of 0 means success.
        return !(split_alr(options.filename));
    }
    else {
        return !(alr_parse(options.filename, options, dump_interface));
    }
}

