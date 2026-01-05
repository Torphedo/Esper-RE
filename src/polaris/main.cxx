#include <glad/glad.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <common/logging.h>
#include <common/arguments.h>
#include <common/path.h>

#include <gui_bootstrap.hxx>
#include <layer_imgui.hxx>

#include <formats/st00.h>
#include "alr/alr_dump.hxx"
#include "gui/polaris.hxx"
#include "validation.hxx"
#include "version.h"

const char* dump_textures_flag = "dump-textures";
const char* dump_mats_flag = "dump-materials";
const char* validate_flag = "--validate";
const char* extract_audio_flag = "extract-audio";

void print_usage() {
    printf("Usage: polaris [ALR filename] [%s | %s]\n", dump_textures_flag, validate_flag);
}

int main(int argc, char** argv) {
    // Enable ANSI escape codes (for printing in color) on Windows
    enable_win_ansi();

    const char* path = "";
    const char* flag = "";
    const char* outpath = "";

    switch (argc) {
    default:
        outpath = args_get_from_back(argc, argv, 0);
        path = args_get_from_back(argc, argv, 1);
        break;
    case 2:
        path = args_get_from_back(argc, argv, 0);
        break;
    case 1:
        break;
    }

    if (*outpath == '-') {
        // If this last arg was a flag, there are no paths.
        path = outpath = "";
    }

    if (*path == '-') {
        // This is a flag, not a path. This probably means we only have 1 path
        // instead of 2, so swap them around.
        path = "";
        std::swap(path, outpath);

        // This should've been checked earlier
        assert(*path != '-');
    }

    // Setup GUI classes
    gui_app app;
    app.layers.emplace_back(std::make_unique<layer_imgui>());
    app.layers.emplace_back(std::make_unique<polaris>());
    polaris* pol = dynamic_cast<polaris*>(app.layers.back().get());

    if (strlen(path) > 0) {
        // We have an argument, it should be a filepath. Try to load as an ALR or .dat file.
        if (file_has_magic(path, 0x11)) {
            if (!pol->editor.alr.load(path)) {
                return EXIT_FAILURE;
            }
        }
        else if (file_has_magic(path, st00_magic)) {
            if (!pol->map.load(path)) {
                return EXIT_FAILURE;
            }
        }
    }

    if (args_getflag(argc, argv, dump_textures_flag, nullptr)) {
        LOG_MSG(info, "Dumping textures for %s\n", path);
        const bool res = alr::dump_all_textures(pol->editor.alr);
        return (res) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (args_getflag(argc, argv, dump_mats_flag, nullptr)) {
        LOG_MSG(info, "Dumping materials for %s\n", path);
        const bool res = alr::dump_all_materials(pol->editor.alr, outpath);
        return (res) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    else if (args_getflag(argc, argv, extract_audio_flag, nullptr)) {
        LOG_MSG(info, "Extracting audio...\n");
        if (argc < 4) {
            LOG_MSG(info, "The --%s option needs at least 4 arguments, like this:\n", extract_audio_flag);
            printf("\t%s --%s ./output_folder Assets/Data/Sound/Title_Logo.bin", argv[0], extract_audio_flag);
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
    else if (args_getflag(argc, argv, "validate", nullptr)) {
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
    } else if (args_getflag(argc, argv, "help", "h")) {
        print_usage();
    } else if (args_getflag(argc, argv, "version", "v")) {
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
        // No special arguments, run in normal graphical mode.
        pol->headless = false;
        if (!app.run("Polaris v" POLARIS_VERSION)) {
            // Actual error message printed for us
            LOG_MSG(error, "Failed to start up!\n");
            return EXIT_FAILURE;
        } else {
            return EXIT_SUCCESS;
        }
    }

    return EXIT_SUCCESS;
}
