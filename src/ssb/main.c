#include <stdio.h>
#include <stdbool.h>

#include <common/int.h>
#include <common/logging.h>
#include <common/arguments.h>

#include "ssb.h"

const char* args[] = {
    "--help",
    "--version",
    "--info",
};

typedef enum {
    help,
    version,
    info_only,
}mode;

static const char* version_string = "1.0.0";
static const char* url = "https://github.com/Torphedo/Esper-RE";

int main(int argc, char* argv[]) {
    enable_win_ansi(); // Enable color on Windows
    // Parse command-line arguments.
    const flags options = parse_arguments(argc, argv, args, 3);

    if (options.silent) {
        logging_enabled = false;
    }

    switch (options.mode) {
        case version:
            printf("ssb (Esper-RE tools) v%s\n", version_string);
            printf("Open-source @ %s\n", url);
            printf("Written by Torphedo [w/ help from Vu314]\n");
            return 0;
        case help:
            LOG_MSG(info, "Usage: ssb [filename] [--info]\n");
            printf("Input/Output:\n");
            printf("\tfilename: path of an SSB file to act on\n");
            return 0;
    }

    // Return value must be inverted because stdbool false == 0, and an exit
    // code of 0 means success.

    if (options.input_path == NULL) {
        LOG_MSG(error, "No SSB to operate on, exiting\n");
        return 1;
    }

    FILE* f = fopen(options.input_path, "rb");
    if (f == NULL) {
        return 1;
    }
    ssb_header header = {0};
    fread(&header, sizeof(header), 1, f);
    fseek(f, header.func_table_addr, SEEK_SET);
    const u32 num_entries = (header.text_addr - header.func_table_addr) / sizeof(ssb_functable_entry);

    for (u32 i = 0; i < num_entries; i++) {
        ssb_functable_entry entry = {0};
        fread(&entry, sizeof(entry), 1, f);
        decoded_text text = decode_text(entry);

        LOG_MSG(info, "%s @ 0x%X\n", text.data, sizeof(ssb_header) + (entry.func_offset * 4));
    }
}

