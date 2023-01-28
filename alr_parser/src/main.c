#include <string.h>
#include <stdio.h>

#include <logging.h>

#include "parsers.h"
#include "arguments.h"

// Cross-platform pause
static inline void pause()
{
    log_error(INFO, "Press Enter to exit...");
    unsigned char dummy = getchar();
}

int main(int argc, char* argv[])
{
    // Parse command-line arguments.
    flags options = parse_arguments(argc, argv);

    if (options.filename == NULL)
    {
        log_error(CRITICAL, "main(): No filenames detected.\n");
        return 1;
    }

    if (options.silent) { disable_logging(); }
    if (options.info_mode) { set_info_mode(); }
    if (options.split)
    {
        return split_alr(options.filename);
    }
    else
    {
        return block_parse_all(options.filename, options);
    }
}
