#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <common/file.h>
#include <common/path.h>
#include <common/image.h>
#include <common/dds.h>
#include <common/arguments.h>

#include <polaris/version.h>
#include <string>

typedef enum {
    INVALID, DDS, DDX,
}filetype;

filetype get_type(const char* path) {
    if (path_has_extension(path, "ddx")) {
        return DDX;
    }
    if (path_has_extension(path, "dds")) {
        return DDS;
    }
    return INVALID;
}

bool ddx_to_dds(const char* inpath, const char* outpath) {
    u8* buf = file_load(inpath);
    if (!buf) {
        printf("Failed to load input!\n");
        return false;
    }

    const texture tex = {
        .data = buf,
        .width = 1024,
        .height = 2048,
        .compressed = true,
        .fmt = DXT1,
    };
    img_write(tex, outpath);

    free(buf);
    return true;
}

bool dds_to_ddx(const char* inpath, const char* outpath) {
    const s64 size = file_size(inpath);
    u8* buf = file_load(inpath);
    if (!buf) {
        printf("Failed to load input!\n");
        return false;
    }

    const u8* data = buf + sizeof(dds_header);
    const s64 outsize = MAX(0, size - sizeof(dds_header));

    FILE* f = fopen(outpath, "wb");
    if (f) {
        fwrite(data, outsize, 1, f);
        fclose(f);
    }

    free(buf);
    return true;
}

void print_usage(const char* argv0) {
    printf("usage: %s [input file] [output file]\n", argv0);
    printf("For example:\n");
    printf("\t%s Assets/Data/System/dbfont.ddx dbfont.dds\n", argv0);
    printf("\t%s dbfont.dds Assets/Data/System/dbfont.ddx\n", argv0);
}

void print_version() {
    printf("DDSX v1.0 by Torphedo\nOpen-source @ " POLARIS_URL "\n");
}

// CLI utility to convert a DDS file to DDX and back.
// DDX is used for the font atlas in Phantom Dust, it's just a 1024x2048 BC1
// texture with no DDS header.
int main(int argc, char** argv) {
    if (argc == 1 || args_getflag(argc, argv, "help", "h")) {
        print_usage(argv[0]);
        return EXIT_SUCCESS;
    }

    if (args_getflag(argc, argv, "version", "v")) {
        print_version();
        return EXIT_SUCCESS;
    }

    if (argc > 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char* inpath = argv[1];

    std::string outpath;
    if (argc < 3) {
        // If there's only 1 input, make the output be the same path with the
        // opposite file type.
        outpath = inpath;
        s32 idx = outpath.find(".dds");
        if (idx > 0) {
            outpath.replace(idx, 4, ".ddx");
        } else {
            idx = outpath.find(".ddx");
            if (idx > 0) {
                outpath.replace(idx, 4, ".dds");
            }
        }
    } else {
        // There are multiple outputs, proceed normally
        outpath = argv[2];
    }

    const filetype intype = get_type(inpath);
    const filetype outtype = get_type(outpath.c_str());

    if (intype == INVALID || outtype == INVALID) {
        printf("The input or output type is unknown (must be .dds or .ddx)\n");
        return EXIT_FAILURE;
    }

    if (intype == outtype) {
        printf("Both file paths are the same file type, so there's no conversion needed...\n");
        return EXIT_FAILURE;
    }

    bool res = false;
    if (intype == DDS) {
        assert(outtype == DDX);
        res = dds_to_ddx(inpath, outpath.c_str());
    } else {
        assert(intype == DDX && outtype == DDS);
        res = ddx_to_dds(inpath, outpath.c_str());
    }

    if (res) {
        printf("Converted '%s' to '%s'\n", inpath, outpath.c_str());
    } else {
        printf("Failed to convert '%s' to '%s'\n", inpath, outpath.c_str());
    }

    return (res) ? EXIT_SUCCESS : EXIT_FAILURE;
}