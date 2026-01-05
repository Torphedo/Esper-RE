#include <glad/glad.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <common/logging.h>
#include <common/path.h>

#include <gui_bootstrap.hxx>
#include <layer_imgui.hxx>

#include <formats/st00.h>
#include "alr/alr_dump.hxx"
#include "gui/polaris.hxx"
#include "validation.hxx"
#include "version.h"

const char* dump_textures_flag = "--dump-textures";
const char* dump_mats_flag = "--dump-materials";
const char* validate_flag = "--validate";
const char* extract_audio_flag = "--extract-audio";

void print_usage() {
    printf("Usage: polaris [ALR filename] [%s | %s]\n", dump_textures_flag, validate_flag);
}

int main(int argc, char** argv) {
    // Enable ANSI escape codes (for printing in color) on Windows
    enable_win_ansi();

    gui_app app;
    app.layers.emplace_back(std::make_unique<layer_imgui>());
    app.layers.emplace_back(std::make_unique<polaris>());
    polaris* pol = dynamic_cast<polaris*>(app.layers.back().get());

    const char* path = "";
    const char* flag = "";
    const char* outpath = "";

    switch (argc) {
    case 4:
        outpath = argv[3];
        [[fallthrough]];
    case 3:
        flag = argv[2];
        [[fallthrough]];
    default:
    case 2:
        if (argv[1][0] == '-') {
            flag = argv[1];
        } else {
            path = argv[1];
        }
    case 1:
        break;
    }

    if (strlen(path) > 0) {
        // We have an argument, it should be a filepath.
        if (file_has_magic(path, 0x11)) {
            if (!pol->editor.alr.load(path)) {
                // An error message will be printed for us down the chain, just exit
                return EXIT_FAILURE;
            }
        }
        else if (file_has_magic(path, st00_magic)) {
            if (!pol->map.load(path)) {
                // An error message will be printed for us down the chain, just exit
                return EXIT_FAILURE;
            }
        }
    }

    if (strlen(flag) == 0) {
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

    if (strcmp(flag, dump_textures_flag) == 0) {
        LOG_MSG(info, "Dumping textures for %s\n", path);
        const bool res = alr::dump_all_textures(pol->editor.alr);
        return (res) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (strcmp(flag, dump_mats_flag) == 0) {
        LOG_MSG(info, "Dumping materials for %s\n", path);
        const bool res = alr::dump_all_materials(pol->editor.alr, outpath);
        return (res) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else if (strcmp(flag, extract_audio_flag) == 0) {
        LOG_MSG(info, "Extracting audio...\n");
        if (argc < 4) {
            LOG_MSG(info, "The %s option needs at least 4 arguments, like this:\n", extract_audio_flag);
            printf("\t%s %s ./output_folder Assets/Data/Sound/Title_Logo.bin", argv[0], extract_audio_flag);
            return EXIT_FAILURE;
        }

        const u32 out_dir_idx = 2;
        const u32 files_idx = 3;
        const char* out_dir = argv[out_dir_idx];
        char* const * files = &argv[files_idx];
        for (u32 i = 0; i < argc - files_idx; i++) {
            audio_tool audioTool;
            audioTool.load(files[i]);

            std::string prefix = files[i];
            if (path_has_slashes(prefix.c_str())) {
                path_get_filename(files[i], prefix.data());
            }
            prefix.replace(prefix.find(".bin"), 4, "");
            audioTool.dump_clips_to_wav(out_dir, prefix.c_str());

            LOG_MSG(info, "Extracted '%s' to '%s'\n", files[i], out_dir);
        }
    }
    else if (strcmp(flag, validate_flag) == 0) {
        LOG_MSG(info, "Validating '%s'...\n", path);
        std::string message;
        bool result = alr_validate(message, pol->editor.alr, true);
        result &= mapdata_validate(pol->map, message);
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
        printf("Polaris (Esper-RE tools) v" POLARIS_VERSION "\n");
        printf("Open-source @ " POLARIS_URL "\n");
        printf("Written by Torphedo\n");
        const char* special_thanks[] = {
            "fleevoid (Almost everything early on, brainstorming, etc.)",
            "blasianblazy (Early ALR layout info, cubemap info, map object IDs/transforms, etc.)",
            "NerdyMiner (Early ALR research, texture & model research)",
            "Toaf (indirect animation & SSB info via releasing a development build)",
            "Vu (SSB research, decoding for ALR texture and bone names)",
            "Nuion (texture dumper testing)",
            "Czarpos (Autodesk Maya .anim Blender plugin)",
        };

        printf("Special Thanks:\n");
        for (const char* txt : special_thanks) {
            printf("\t%s\n", txt);
        }
    } else {
        LOG_MSG(error, "I didn't find any known arguments, I'm not sure what you want me to do with the file.\n");
        print_usage();
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
