// txt2h, 2003-2006 by ScottManDeath
// Modified by Torphedo 2024

#include <stdio.h>
#include <malloc.h>
#include <string.h>

void write_line(FILE* f, const char* line) {
    fprintf(f, "\"%s\\n\"\n", line);
}

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: txt2h infile outfile\n");
        return 1;
    }
    const char* in_path = argv[1];
    const char* out_path = argv[2];

    FILE* in_file = fopen(in_path, "rb");
    FILE* out_file = fopen(out_path, "wb");

    if (in_file == NULL) {
        printf("Error opening input %s\n", in_path);
        return 1;
    }
    if (out_file == NULL) {
        printf("Error opening output %s\n", out_path);
        return 1;
    }

    // Copy lines to output with string formatting
    char buff[8192] = {0};
    while (fgets(buff, sizeof(buff), in_file)) {
        // Handle Windows carriage return first, since it comes before \n.
        char* newline = strchr(buff, '\r');
        if (newline != NULL) {
            *newline = 0x00; // Terminate the string at the line ending
        }
        else {
            // We only delete \n if there's no carriage return
            newline = strchr(buff, '\n');
            if (newline != NULL) {
                *newline = 0x00;
            }
        }
        write_line(out_file, buff);
    }

    // Cleanup
    fclose(out_file);
    fclose(in_file);
}
