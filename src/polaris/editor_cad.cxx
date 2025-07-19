#include "editor_cad.hxx"

#include <cglm/struct.h>

#include <common/vfile.h>
#include <common/int.h>
#include <formats/cad.h>

void editor_cad::do_gui() noexcept {
    if (!data) {
        return; // Ignore if not initialized
    }

    ImGui::Begin("CAD Data");
    if (ImGui::BeginTabBar("Chunk Tabs")) {
        if (ImGui::BeginTabItem("Raw file data")) {
            hex_edit.DrawContents((void *) this->data, this->size);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Chunks")) {
            vfile vf = vfile_open(data, size);
            const u32 num_vertices = VFILE_READ(u32, &vf);
            vec3s *vertices = (vec3s *) vfile_cur(vf);
            vfile_seek(&vf, sizeof(*vertices) * num_vertices);

            for (u32 i = 0; i < num_vertices; i++) {
                char label[64] = {0};
                snprintf(label, sizeof(label) - 1, "##vertex %d", i);
                ImGui::InputFloat3(label, vertices[i].raw);
            }

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}