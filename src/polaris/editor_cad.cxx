#include "editor_cad.hxx"

#include <cglm/struct.h>
#include <nfd.h>

#include <common/vfile.h>
#include <common/int.h>
#include <formats/cad.h>
#include "imgui_utils.hxx"

static bool dump_raw_vertices(vec3f* verts, u32 vert_count, const char* out_path) {
    FILE* f = fopen(out_path, "wb");
    if (!f) {
        return false;
    }

    for (u32 i = 0; i < vert_count; i++) {
        vec3f* vert = &verts[i];
        fprintf(f, "v %f %f %f\n", vert->x, vert->y, vert->z);
    }

    fclose(f);
    return true;
}

void editor_cad::do_gui() noexcept {
    if (!data) {
        return; // Ignore if not initialized
    }

    ImGui::Begin("CAD Data");
    if (ImGui::BeginTabBar("Chunk Tabs")) {
        if (ImGui::BeginTabItem("Chunks")) {
            vfile vf = vfile_open(data, size);
            auto cad = (cad_file*)vfile_cur(vf);

            if (ImGui::CollapsingHeader("Vertices")) {
                ImGui::Text("%d vertices @ 0x%lX", cad->vertex_count, offsetof(cad_file, vertex_count));
                if (ImGui::Button("Dump to OBJ")) {
                    char* path = nullptr;
                    const nfdu8filteritem_t filters[] = { { "3D Object", "obj"} };
                    nfdresult_t result = NFD_SaveDialogU8(&path, filters, ARRAY_SIZE(filters), nullptr, nullptr);
                    if (result == NFD_OKAY && path != nullptr) {
                        dump_raw_vertices(cad->vertices, cad->vertex_count, path);
                    }
                    free(path);
                }

                for (u32 i = 0; i < cad->vertex_count; i++) {
                    char label[64] = {0};
                    snprintf(label, sizeof(label) - 1, "##vertex %d", i);
                    ImGui::InputFloat3(label, &cad->vertices[i].x);
                }
            }

            if (ImGui::CollapsingHeader("Quads")) {
                ImGui::Text("%d quads @ 0x%lX", cad->quad_count, offsetof(cad_file, quad_count));
                for (u32 i = 0; i < cad->quad_count; i++) {
                    char label[64] = {0};
                    snprintf(label, sizeof(label) - 1, "##quad_vert %d", i);
                    ImGui::InputScalarN(label, ImGuiDataType_S32, cad->quads[i].vertices, 3);
                }
            }

            if (ImGui::CollapsingHeader("Paths")) {
                ImGui::Text("%d paths @ 0x%lX", cad->path_count, offsetof(cad_file, path_count));
                for (u32 i = 0; i < cad->path_count; i++) {
                    cad_path* path = &cad->paths[i];
                    char label[64] = {0};

                    snprintf(label, sizeof(label) - 1, "Start point ##%d", i);
                    ImGui::InputFloat3(label, &path->start_point.x);

                    snprintf(label, sizeof(label) - 1, "End point ##%d", i);
                    ImGui::InputFloat3(label, &path->end_point.x);

                    snprintf(label, sizeof(label) - 1, "Connected areas ##%d", i);
                    ImGui::InputScalarN(label, ImGuiDataType_U32, path->connected_areas, 2);
                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();
                }
            }

            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Raw file data")) {
            hex_edit.DrawContents((void *) this->data, this->size);
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}