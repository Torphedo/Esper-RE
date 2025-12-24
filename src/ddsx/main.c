#include <stdlib.h>
#include <stdio.h>
#include <common/file.h>

#define DDS_HEADER_SIZE 0x80

// CLI utility to convert a DDS file to DDX.
// DDX is used for the font atlas in Phantom Dust, it's just a 1024x2048 BC1
// texture with no DDS header.
int main(int argc, char** argv) {
    if (argc != 3) {
        // TODO: Add a real error message
        printf("wrong!\n");
        return EXIT_FAILURE;
    }
    const char* inpath = argv[1];
    const char* outpath = argv[2];

    const s64 size = file_size(inpath);
    u8* buf = file_load(inpath);
    if (!buf) {
        printf("Failed to load input!\n");
        return EXIT_FAILURE;
    }

    const u8* data = buf + DDS_HEADER_SIZE;
    const s64 outsize = MAX(0, size - DDS_HEADER_SIZE);

    int result = EXIT_FAILURE;
    FILE* f = fopen(outpath, "wb");
    if (f) {
        fwrite(data, outsize, 1, f);
        fclose(f);
        printf("Wrote trimmed DDX to '%s'\n", outpath);
        result = EXIT_SUCCESS;
    }

    free(buf);
    return result;
}
