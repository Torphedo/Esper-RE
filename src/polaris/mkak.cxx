#include "mkak.hxx"
#include <common/file.h>
#include <common/vfile.h>
#include <common/logging.h>
#include <formats/eventpack.h>

#include <cstdio>

namespace mkak {
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

} // namespace mkak
