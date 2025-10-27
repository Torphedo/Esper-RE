#include "mapdata.hxx"
#include <cstdlib>

#include <common/file.h>
#include <common/vfile.h>
#include <common/logging.h>

#include "formats/st00.h"
#include "imgui_utils.hxx"

bool mapdata::load_verify() const noexcept {
    const u32 magic = *(u32*)data;
    if (magic != st00_magic) {
        LOG_MSG(error, "\"%s\" doesn't seem to be a .dat map file (invalid magic 0x%x)\n", filepath, magic);
        return false;
    }
    return true;
}

mapdata::mapdata(const char* filepath) : filepath(filepath) {
    initialized = load(filepath);
}

// Move assignment operator (when assigning with a temp value)
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
            for (s32 i = 0; i < ARRAY_SIZE(header.unk9); i++) {
                const s32 offset = header.unk9[i];
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
            const s32 selected_offset = header.unk9[selected_chunk];
            hex_edit.DrawContents(data + selected_offset, 0x100);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}
