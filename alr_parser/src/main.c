#include <string.h>
#include <stdio.h>

#include <logging.h>

#include "parsers.h"

// Cross-platform pause
static inline void pause()
{
    log_error(INFO, "Press Enter to exit...");
    unsigned char dummy = getchar();
}

int main(int argc, char* argv[])
{
    // Parse command-line arguments.
    switch (argc)
    {
        case 1:
            log_error(INFO, "Please provide an input filename.\n");
            pause();
            return 1;
        case 2:
            log_error(WARNING, "No action specified, defaulting to --split\n");
            split_alr(argv[1]);
            break;
        default:
            if (strcmp(argv[2], "--split") == 0)
            {
                split_alr(argv[1]);
            }
            else if (strcmp(argv[2], "--dump") == 0)
            {
                block_parse_all(argv[1]);
            }
            else if (strcmp(argv[2], "--info") == 0)
            {
                set_info_mode();
                block_parse_all(argv[1]);
            }
    }

	return 0;
}
