#pragma once
#include <vector>
#include <string>
#include <common/int.h>

namespace mkak {
    std::vector<std::string> list_files(const char* path);
    bool dump_to_folder(const char* path, const char* output_dir);
    bool create_pack(const std::vector<std::string>& files, const char* output_path);
}
