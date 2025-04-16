#include "mapdata.hxx"
#include <cstdlib>

#include <common/file.h>
#include <common/vfile.h>
#include <common/logging.h>

#include "formats/st00.h"
#include "imgui_utils.hxx"

mapdata::mapdata(const char* filepath) {
    if (!file_exists(filepath)) {
        return;
    }

    size = file_size(filepath);
    data = file_load(filepath);

    if (data == nullptr) {
        LOG_MSG(error, "Failed to load layout file \"%s\"\n", filepath);
    }

    initialized = true;
}

mapdata& mapdata::operator=(mapdata&& other) {
    if (this != &other) {
        free(data);
        memcpy(this, &other, sizeof(other)); // Copy state from temporary
        // Wipe temporary so it doesn't free our pointer on destroy
        memset(&other, 0, sizeof(other));
    }

    return *this;
}

mapdata::~mapdata() {
    free(data);
    initialized = false;
}

void mapdata::do_gui() noexcept {
    if (!this->initialized) {
        return; // Ignore if not initialized
    }

    ImGui::Begin(".dat Data");
    if (ImGui::BeginTabBar("Chunk Tabs")) {
        if (ImGui::BeginTabItem("Raw file data")) {
            hex_edit.DrawContents((void *) this->data, this->size);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Chunks")) {

            vfile vf = vfile_open(data, size);
            st00_t header = VFILE_READ(st00_t, &vf);

            ImGui::BeginChildFitContent("Chunk Selection", 0.3f);
            for (s32 i = 0; i < ARRAY_SIZE(header.unk7); i++) {
                const s32 offset = header.unk7[i];
                if (offset <= 0 && offset >= this->size) {
                    continue;
                }

                char label[0x20] = {0};
                snprintf(label, sizeof(label) - 1, "Chunk %d", i);
                if (ImGui::Selectable(label)) {
                    selected_chunk = i;
                }
            }

            ImGui::EndChild();
            ImGui::SameLine();

            ImGui::BeginChildFitContent("Chunk Hex Editor", 0.3f);
            const s32 selected_offset = header.unk7[selected_chunk];
            hex_edit.DrawContents(data + selected_offset, 0x100);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
