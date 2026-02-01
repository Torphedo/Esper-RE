#include "mkak.hxx"

#include <common/file.h>
#include <common/vfile.h>
#include <common/path.h>
#include <formats/eventpack.h>

namespace mkak {

std::vector<std::string> list_files(const char* path) {
    std::vector<std::string> output;
    u32 size = file_size(path);
    u8* data = file_load(path);
    if (!data) {
        LOG_MSG(error, "Failed to load input file '%s'\n", path);
        return output;
    }

    vfile vf = vfile_open(data, size);
    auto* header = (ak_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(ak_header) + (header->file_count * 4));

    for (u32 i = 0; i < header->file_count; i++) {
        const char* filename = (const char*) vfile_cur(vf);
        vfile_seek(&vf, strlen(filename) + 1);
        output.emplace_back(filename);
    }

    return output;
}

bool dump_to_folder(const char* path, const char* output_dir) {
    const bool output_exists = file_exists(output_dir);

    if (!output_exists) {
        LOG_MSG(error, "Input file '%s' doesn't exist\n", output_dir);
        return false;
    }

    u32 size = file_size(path);
    u8* data = file_load(path);
    if (!data) {
        LOG_MSG(error, "Failed to load input file '%s'\n", path);
        return false;
    }

    vfile vf = vfile_open(data, size);
    auto* header = (ak_header*)vfile_cur(vf);
    vfile_seek(&vf, sizeof(ak_header) + (header->file_count * 4));

    for (u32 i = 0; i < header->file_count; i++) {
        const char* filename = (const char*) vfile_cur(vf);
        vfile_seek(&vf, strlen(filename) + 1);
        const u8* file_data = data + header->file_offsets[i];
        u32 end_of_file = header->file_offsets[i + 1];
        if (i + 1 == header->file_count) {
            end_of_file = size;
        }

        const u32 resource_size = end_of_file - header->file_offsets[i];
        std::string temp = std::string(output_dir) + "/" + filename;
        FILE* f = fopen(temp.c_str(), "wb");
        if (f) {
            fwrite(file_data, resource_size, 1, f);
            fclose(f);
        }
    }

    free(data);
    return true;
}

bool create_pack(const std::vector<std::string>& files, const char* output_path) {
    ak_header header = {
        .file_count = (u32)files.size(),
    };
    header.header_size = sizeof(header) + (header.file_count * sizeof(*header.file_offsets));

    FILE* f = fopen(output_path, "wb");
    if (!f) {
        LOG_MSG(error, "Unable to open output file '%s'\n", output_path);
        return false;
    }

    // Write filenames so we can find out the full header size
    fseek(f, header.header_size, SEEK_SET);
    for (const std::string& path : files) {
        std::string basename = path;

        // Note that we have to use strlen() since we changed the string without
        // going through the correct API. I had been doing this with the normal
        // string API, but this is much more readable.
        path_get_filename(path.c_str(), basename.data());
        fwrite(basename.data(), strlen(basename.data()) + 1, 1, f);
    }
    header.header_size = ftell(f);

    // In vanilla, MK files start their data @ 0x1000, while AK files start
    // immediately after the header. To emulate this, we round up to the next
    // multiple of 0x1000, or do nothing.
    const bool is_mk = path_has_extension(output_path, "mk");
    const u32 first_offset = (is_mk) ? 0x1000 : header.header_size;

    fwrite(&header, sizeof(header), 1, f);
    fseek(f, first_offset, SEEK_SET);

    std::vector<u32> offsets;
    for (const std::string& path : files) {
        if (!path_is_file(path.c_str())) {
            LOG_MSG(warning, "'%s' isn't a file, I'm skipping it.\n", path.c_str());
            continue;
        }

        offsets.push_back(ftell(f));

        // Copy file contents to the pack file, 4K at a time
        u8 buf[4096];
        FILE* infile = fopen(path.c_str(), "rb");
        if (!infile) {
            LOG_MSG(warning, "Unable to open input file '%s'\n", path.c_str());
            continue;
        }
        while (!feof(infile)) {
            const size_t bytes_read = fread(buf, 1, sizeof(buf), infile);
            fwrite(buf, 1, bytes_read, f);
        }
        fclose(infile);
    }

    // Jump back to start to write header & offsets
    fseek(f, 0, SEEK_SET);
    fwrite(&header, sizeof(header), 1, f);
    fwrite(offsets.data(), sizeof(u32), offsets.size(), f);
    fclose(f);

    return true;
}

} // namespace mkak
