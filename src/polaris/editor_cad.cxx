#include "editor_cad.hxx"

#include <cglm/struct.h>
#include <nfd.h>

#include <common/vfile.h>
#include <common/int.h>
#include <formats/cad.h>
#include "imgui_utils.hxx"

static bool dump_raw_vertices(const cad_file& cad, const char* out_path) {
    FILE* f = fopen(out_path, "wb");
    if (!f) {
        return false;
    }

    // Dump vertices
    for (u32 i = 0; i < cad.vertex_count; i++) {
        const vec3f& vert = cad.vertices[i];
        fprintf(f, "v %f %f %f\n", vert.x, vert.y, vert.z);
    }

    // Dump indices
    for (const cad_quad& quad : cad.quads) {
        const char* group = "unimplemented_nonzero";
        if (quad.flags & 1) {
            group = "flag_1";
        } else if (quad.flags == 0) {
            group = "none";
        }
        fprintf(f, "g %s\n", group);

        // We add 1 to everything since OBJ indices start @ 1.
        fprintf(f, "f %d %d %d\n", quad.vertices[0] + 1, quad.vertices[1] + 1, quad.vertices[2] + 1);
        fprintf(f, "f %d %d %d\n", quad.vertices[0] + 1, quad.vertices[2] + 1, quad.vertices[3] + 1);
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
                        dump_raw_vertices(*cad, path);
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
                    ImGui::InputScalarN(label, ImGuiDataType_S32, cad->quads[i].vertices, 4);
                    snprintf(label, sizeof(label) - 1, "Flags##%d", i);
                    ImGui::InputU8(label, &cad->quads[i].flags);
                    ImGui::Separator();
                }
            }

            if (ImGui::CollapsingHeader("Quad Unknown 3")) {
                ImGui::Text("%d quads @ 0x%lX", cad->quad_count, offsetof(cad_file, quad_count));
                for (u32 i = 0; i < cad->quad_count; i++) {
                    char label[64] = {0};
                    snprintf(label, sizeof(label) - 1, "##quad_unk3 %d", i);
                    ImGui::InputScalarN(label, ImGuiDataType_S16, cad->quads[i].unknown3, 4);
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