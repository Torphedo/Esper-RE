#include <glad/glad.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <gui_bootstrap.hxx>
#include <layer_imgui.hxx>
#include <common/logging.h>
#include <common/vfile.h>

#include <formats/alr.h>
#include "polaris.hxx"
#include "alr/alr_resources.hxx"
#include "validation.hxx"
#include "version.h"

const char* dump_textures_flag = "--dump-textures";
const char* validate_flag = "--validate";

void print_usage() {
    printf("Usage: polaris [ALR filename] [%s | %s]\n", dump_textures_flag, validate_flag);
}

int dump_all_textures(const polaris& pol) {
    al::resource::chunk texture_chunk = pol.alr.first_chunk_by_id(0x15);
    al::resource::chunk atlas_chunk = pol.alr.first_chunk_by_id(0x10);
    if (texture_chunk.size == 0 && atlas_chunk.size == 0) {
        LOG_MSG(warning, "I couldn't find any textures to dump.\n");
        return EXIT_FAILURE;
    }

    system("mkdir textures"); // We need this folder for later

    // Try to find texture and texture atlas metadata, we need both to make a
    // good guess about dimensions.
    u32 textures_dumped = 0;

    // Read texture chunk data
    vfile vf = vfile_open(pol.alr.data + texture_chunk.offset, texture_chunk.size);
    // Skip over the ID and size fields we already have
    vfile_seek(&vf, sizeof(chunk_generic));
    const u32 num_entries = VFILE_READ(u32, &vf);
    const texture_entry* tex_entries = (texture_entry*)vfile_cur(vf);

    // Read atlas chunk data
    const atlas_entry* atlas_entries = nullptr;
    const atlas_name* atlas_names = nullptr;
    atlas_header header_atlas = {0};
    if (atlas_chunk.size > 0) {
        vf = vfile_open(pol.alr.data + atlas_chunk.offset, atlas_chunk.size);

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
        texture cur_tex = convert_tex(pol.alr.resource_buffer(), tex_entries[i]);

        // Decode the texture filename
        char decoded_name[0x20] = {0};
        decode_single32(decoded_name, tex_entries[i].text1);
        decode_single32(&decoded_name[ENCODED_CHAR_COUNT], tex_entries[i].text2);
        strncat(decoded_name, ".dds", sizeof(decoded_name) - 1);
        char* name = decoded_name;

        if (atlas_entries != nullptr && header_atlas.atlas_count > i) {
            const atlas_entry entry = atlas_entries[i];
            // We get better dimension info from the atlas headers, so use it!
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
        LOG_MSG(warning, "I couldn't find any textures to dump.\n");
    }
    return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
    // Enable ANSI escape codes (for printing in color) on Windows
    enable_win_ansi();

    gui_app app;
    app.layers.emplace_back(std::make_unique<layer_imgui>());
    app.layers.emplace_back(std::make_unique<polaris>());
    polaris* pol = dynamic_cast<polaris*>(app.layers.back().get());
    
    if (argc >= 2) {
        // We have an argument, it should be a filepath.
        if (!pol->alr.load(argv[1])) {
            // An error message will be printed for us down the chain, just exit
            return EXIT_FAILURE;
        }
    }

    if (argc < 3) {
        pol->headless = false;
        // No special arguments, run in normal graphical mode.
        if (!app.run("Polaris v" POLARIS_VERSION)) {
            // Actual error message printed for us
            LOG_MSG(error, "Failed to start up!\n");
            return EXIT_FAILURE;
        } else {
            return EXIT_SUCCESS;
        }
    }

    // Parse arguments
    const char* path = argv[1];
    const char* flag = argv[2];

    if (strcmp(flag, dump_textures_flag) == 0) {
        LOG_MSG(info, "Dumping textures for %s\n", path);
        return dump_all_textures(*pol);
    } else if (strcmp(flag, validate_flag) == 0) {
        LOG_MSG(info, "Validating '%s'...\n", path);
        std::string message;
        const bool result = alr_validate(message, *pol);
        if (result) {
            LOG_MSG(info, "Validation passed!\n");
        } else {
            LOG_MSG(error, "Validation failed!\n");
        }
        printf("%s", message.c_str());

        return !result;
    } else if (strcmp(flag, "--help") == 0) {
        print_usage();
    } else if (strcmp(flag, "--version") == 0) {
        printf("Polaris (Esper-RE tools) v" POLARIS_VERSION);
        printf("Open-source @ " POLARIS_URL "\n");
        printf("Written by Torphedo [w/ help from fleevoid, blasianblazy, Vu & Nuion]\n");
    } else {
        LOG_MSG(error, "I didn't find any known arguments, I'm not sure what you want me to do with the file.\n");
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
