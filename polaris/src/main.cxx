#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <common/logging.h>
#include <common/vfile.h>

#include "formats/alr.h"
#include "gui_loop.hxx"
#include "polaris.hxx"
#include "alr_texture.hxx"

static const char* version_string = "1.0.0";
static const char* url = "https://github.com/Torphedo/Esper-RE";
const char* dump_textures_flag = "--dump-textures";

void print_usage() {
    printf("Usage: polaris [ALR filename] [%s]\n", dump_textures_flag);
}

int dump_all_textures(polaris pol, const char* path) {
    polaris::chunk texture_chunk = polaris::chunk(0, 0, 0);
    polaris::chunk atlas_chunk = polaris::chunk(0, 0, 0);
    system("mkdir textures"); // We need this folder for later

    // Try to find texture and texture atlas metadata, we need both to make a
    // good guess about dimensions.
    for (polaris::chunk chunk : pol.chunks) {
        if (chunk.id == 0x15) {
            texture_chunk = chunk;
        }
        if (chunk.id == 0x10) {
            atlas_chunk = chunk;
        }
    }

    u32 textures_dumped = 0;

    // Read texture chunk data
    vfile vf = vfile_open(pol.alr_data + texture_chunk.offset, texture_chunk.size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));
    const u32 num_entries = VFILE_READ(u32, &vf);
    texture_entry* tex_entries = (texture_entry*)vfile_cur(vf);

    // Read atlas chunk data
    atlas_entry* atlas_entries = nullptr;
    atlas_name* atlas_names = nullptr;
    atlas_header header_atlas = {0};
    if (atlas_chunk.size > 0) {
        vf = vfile_open(pol.alr_data + atlas_chunk.offset, atlas_chunk.size);

        // Skip over the ID and size fields we already have
        vfile_seek(&vf, sizeof(chunk_generic));
        header_atlas = VFILE_READ(atlas_header, &vf);

        // Skip over names
        atlas_names = (atlas_name*)vfile_cur(vf);
        vfile_seek(&vf, sizeof(atlas_name) * header_atlas.atlas_count);

        atlas_entries = (atlas_entry*)vfile_cur(vf);
    }

    for (u32 i = 0; i < num_entries; i++) {
        // Convert the ALR texture data to our standard texture struct
        texture cur_tex = convert_tex(pol.alr_data + pol.resbuf_offset, tex_entries[i]);

        // Decode the texture filename
        char decoded_name[0x20] = {0};
        decode_single32(decoded_name, tex_entries[i].text1);
        decode_single32(&decoded_name[ENCODED_CHAR_COUNT], tex_entries[i].text2);
        strncat(decoded_name, ".dds", sizeof(decoded_name) - 1);
        char* name = decoded_name;

        if (atlas_entries != nullptr && header_atlas.atlas_count > i) {
            atlas_entry entry = atlas_entries[i];
            // We get better dimension info from the atlas headers, use it!
            // Dimensions from the atlas headers are almost always more
            // accurate, so we always use them unless they're obviously wrong.

            const u32 too_small = 0;
            const u32 too_big = 8192;
            if (entry.width > too_small && entry.width < too_big) {
                cur_tex.width = entry.width;
            }
            if (entry.height > too_small && entry.height < too_big) {
                cur_tex.height = entry.height;
            }

            // Also use the name from the atlas for the filename, because it'll
            // have correct capitalization
            name = (char*)atlas_names[i].name;
        }

        char path[0x30] = {0};
        snprintf(path, sizeof(path) - 1, "textures/%s", name);

        // Save the texture
        img_write(cur_tex, path);
        LOG_MSG(info, "Dumped %s\n", name);
        textures_dumped++;
    }

    if (textures_dumped == 0) {
        // This isn't a *failure*, but might be confusing if we don't say
        // anything and someone expects a texture file to appear.
        LOG_MSG(warning, "Just so you know, I couldn't find any textures in \"%s\".\n", path);
    }
    return EXIT_SUCCESS;
}

// In main() we only "kick off" the program, and gui_main() handles the
// setup/main loop/teardown.
// It feels a little silly to have one main() that basically just calls
// another, but I want to keep all the GUI bootstrapping out of main().
int main(int argc, char** argv) {
    // Enable ANSI escape codes (for printing in color) on Windows
    enable_win_ansi();

    if (argc == 1) {
        // No arguments, run in normal graphical mode.
        // polaris::do_gui() has the real UI code, and is basically the real entry
        // point. Sorry for the kind of unintuitive structure.
        if (!gui_main()) {
            // Actual error message is reported at the failure point
            LOG_MSG(error, "Failed to start up!\n");
            return EXIT_FAILURE;
        } else {
            return EXIT_SUCCESS;
        }
    }
    
    if (argc < 3) {
        // We need at least a filepath and a flag to tell us what to do with
        // the file in headless mode.
    }

    // Parse arguments
    const char* path = argv[1];
    const char* flag = argv[2];

    polaris pol;
    if (!pol.load_alr(path)) {
        // An error message will be printed for us down the chain, so just exit
        return EXIT_FAILURE;
    }

    if (strcmp(flag, dump_textures_flag) == 0) {
        return dump_all_textures(pol, path);
    } else if (strcmp(flag, "--help") == 0) {
        print_usage();
    } else if (strcmp(flag, "--version") == 0) {
        printf("Polaris (Esper-RE tools) v%s", version_string);
        printf("Open-source @ %s\n", url);
        printf("Written by Torphedo [w/ help from fleevoid, blasianblazy, & Nuion]\n");
    } else {
        LOG_MSG(error, "I didn't find any known arguments, I'm not sure what you want me to do with the file.\n");
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
