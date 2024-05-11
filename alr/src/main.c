#include <stdio.h>

#include <common/logging.h>
#include <common/arguments.h>

#include "alr.h"
#include "dump.h"
#include "split.h"
#include "replace.h"

static const char* version_string = "0.6.6";
static const char* url = "https://github.com/Torphedo/Esper-RE";

#define ARRAY_LEN(arr) (sizeof((arr)) / (sizeof((arr)[0])))

char* args[] = {
    "--help",
    "--version",
    "--dump",
    "--replace",
    "--info",
    // "--silent",
    "--split",
    "--animation",
};

typedef enum {
    help,
    version,
    dumptex,
    replacetex,
    info_only,
    split,
    animation,
}mode;

int main(int argc, char* argv[]) {
    enable_win_ansi(); // Enable color on Windows
    // Parse command-line arguments.
    flags options = parse_arguments(argc, argv, args, ARRAY_LEN(args));

    if (options.silent) {
        disable_logging();
    }

    // Default to dump behaviour if nothing is specified. No arguments probably
    // means someone drag-and-dropped, which means they probably want textures.
    alr_interface interface = dump_interface;

    switch (options.mode) {
        case version:
            printf("alr (Esper-RE tools) v%s\n", version_string);
            printf("Open-source @ %s\n", url);
            printf("Written by Torphedo [w/ help from fleevoid, blasianblazy, & Nuion]\n");
            return 0;
        case help:
            LOG_MSG(info, "Usage: alr [filename] --[info / split / dump / replace] [output]\n");
            printf("Input/Output:\n");
            printf("\tfilename: path of an ALR file to act on\n");
            printf("\toutput: (optional) path of the output ALR file. Only used in replace mode.\n\n");

            printf("Options:\n");
            printf("\tinfo:    Prints extra information, and stops any files (like textures) from being written.\n");
            printf("\tsplit:   Splits each chunk of ALR data into a separate file in the \"resources\"\n");
            printf("\t         folder. Used for debugging & texture research. Each texture is\n");
            printf("\t         \"resource_x.bin\", and each other chunk is \"x.bin\" (where x is the index).\n");
            printf("\tdump:    Dumps texture data to DDS using metadata in the file, and by brute force.\n");
            printf("\t         \"Brute force\" textures are in the \"textures\" folder, and all resolution\n");
            printf("\t         and all size/format info is guessed. It only supports square sizes,\n");
            printf("\t         so rectangular textures will look broken. Textures dumped using metadata\n");
            printf("\t         in the file are put next to the alr executable (will be moved to\n");
            printf("\t         \"textures\" in a future version).\n");
            printf("\treplace: Copies data from the input ALR to the output ALR. If any dumped textures\n");
            printf("\t         are found in the \"textures\" folder, they replace the original data.\n");
            printf("\t         You can edit textures by using the dump option, editing the dump, then\n");
            printf("\t         using this option to output a modded ALR.\n");
            printf("\nNote: the order of options and filenames doesn't matter. All that matters is that\n");
            printf("      the input path comes before the output.\n");

            printf("\nExamples: alr --dump FDLogo.alr\n");
            printf("          alr --replace FDLogo.alr FDLogo_modded.alr\n");
            printf("          alr FDLogo.alr --split\n");
            return 0;
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
            if (options.input_path == NULL) {
                LOG_MSG(error, "No input file provided.\n");
                return 1;
            }
            return !(alr_edit(options.input_path, options.output_path, options, replace_interface));
            break;
    }

    // Parse ALR with selected interface.
    // Return value must be inverted because stdbool false == 0, and an exit
    // code of 0 means success.

    if (options.input_path == NULL) {
        LOG_MSG(error, "No ALR to operate on, exiting\n");
        return 1;
    }
    return !(alr_parse(options.input_path, options, interface));
}

