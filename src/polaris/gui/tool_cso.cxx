#include "tool_cso.hxx"
#include <nfd.h>

#include <common/vfile.h>

#include <formats/dxbc_cso.h>
#include <util/imgui_utils.hxx>
#include <util/utils.hxx>

static const nfdu8filteritem_t quest_filter[] = { { "DirectX Compiled Shader Object", "cso"} };

#define INPUT_UNK_FIELD(obj, idx, type) ImGui::Input##type("Unknown " #idx, &obj.unk##idx);

struct swizzle_t {
    char txt[5]; // 4 chars + null
    // Implicit conversion operator
    operator char*() {
        return txt;
    }
};


swizzle_t mask_to_swizzle(u8 val) {
    swizzle_t out = {
        .txt = "XYZW",
    };

    for (s32 i = 3; i >= 0; i--) {
        const u8 mask = 1 << i;
        if ((val & mask) == 0) {
            out.txt[i] = 0;
        }
    }

    return out;
}

void type_text(u8 type) {
    if (type == 3) {
        ImGui::Text("float");
        return;
    }

    ImGui::Text("%d", type);
}

void view_isgn(const ISGNPart* part) {
    for (u32 i = 0; i < part->ElementCount; i++) {
        const ISGNElement& e = part->elements[i];
        const char* name = (const char*)(uintptr_t(part) + e.NameOffset);
        ImGui::Text("Element %d: '%s'", i, name);

        ImGui::Text("\tComponents: %s", (char*)mask_to_swizzle(e.Mask));
        ImGui::Text("\tComponents used: %s", (char*)mask_to_swizzle(e.ReadWriteMask));
        ImGui::Text("\tRegister #%d", e.Register);

        ImGui::Text("\tComponent Type: ");
        ImGui::SameLine();
        type_text(e.ComponentType);
    }
}

void view_cso(const void* data, u32 size) {
    vfile vf = vfile_open((void*)data, size);
    const CSOHeader* header = VFILE_READ_PTR(CSOHeader, &vf);

    for (u32 i = 0; i < header->PartCount; i++) {
        vf.pos = header->PartOffsets[i];
        const CSOPart* part = VFILE_READ_PTR(CSOPart, &vf);
        const ISGNPart* isgn = VFILE_READ_PTR(ISGNPart, &vf);
        if (strncmp(part->Name, "ISGN", 4) == 0) {
            if (ImGui::CollapsingHeader("Input Elements")) {
                view_isgn(isgn);
            }
        }
    }
}

void cso_tool::do_gui() noexcept {
    if (!enabled) {
        return;
    }

    ImGui::Begin("Shader Viewer", &enabled);
    if (data) {
        if (ImGui::Button("Unload file")) {
            this->unload();
        }
    } else {
        ImGui::Text("No shader file loaded.");

        if (ImGui::Button("Load CSO shader")) {
            char* path = nullptr;
            nfdresult_t result_in = NFD_OpenDialogU8(&path, quest_filter, ARRAY_SIZE(quest_filter), nullptr);
            if (result_in == NFD_OKAY && path) {
                this->load(path);
            }
            free(path);
        }
    }

    if (!data) {
        ImGui::End();
        return;
    }

    if (ImGui::BeginTabBar("Shader Viewer Tabs")) {
        if (ImGui::BeginTabItem("Custom Viewer")) {
            view_cso(data, size);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Hex Editor")) {
            hex_edit.DrawContents(data, size);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    ImGui::End();
}
