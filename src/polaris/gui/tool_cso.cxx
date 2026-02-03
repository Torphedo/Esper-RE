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

void type_text(u16 type) {
    if (type == D3D_SVT_FLOAT) {
        ImGui::Text("float");
        return;
    }

    ImGui::Text("%d", type);
}

void class_text(u16 c) {
    if (c == D3D_SVC_VECTOR) {
        ImGui::Text("vector");
        return;
    }

    ImGui::Text("%d", c);
}

void view_isgn(const ISGNPart* part) {
    ImGui::ScopedIndent indent(ImGui::CharWidth(4));
    for (u32 i = 0; i < part->ElementCount; i++) {
        const ISGNElement& e = part->elements[i];
        const char* name = (const char*)(uintptr_t(part) + e.NameOffset);
        ImGui::Text("Element %d: '%s'", i, name);

        ImGui::Text("\tComponents: %s", (char*)mask_to_swizzle(e.Mask));
        ImGui::Text("\tComponents used: %s", (char*)mask_to_swizzle(e.ReadWriteMask));
        ImGui::Text("\tRegister #%d", e.Register);

        ImGui::Text("\tComponent Type: ");
        ImGui::SameLine(); type_text(e.ComponentType);
        ImGui::Text("\n");
    }
}

void view_rdef(const RDEFPart* part) {
    const auto start = (uintptr_t)part;
    const char* creator = (const char*)(start + part->CreatorNameOffset);
    const auto* buffers = (RDEFConstBuffer*)(start + part->ConstBufferOffset);
    const auto* resDescs = (RDEFResDesc*)(start + part->ResBindingOffset);

    ImGui::ScopedIndent indent(ImGui::CharWidth(4));
    ImGui::Text("v%d.%d, created by '%s'", part->VersionMajor, part->VersionMinor, creator);
    ImGui::Spacing();
    ImGui::Text("\nConstant Buffers:\n");
    for (u32 i = 0; i < part->ConstBufferCount; i++) {
        const RDEFConstBuffer& buf = buffers[i];
        const char* name = (const char*)(start + buf.NameOffset);
        const auto* vars = (RDEFVariable*)(start + buf.VariablesOffset);

        ImGui::Separator();
        ImGui::Text("Buffer '%s' is %d bytes, %d variables", name, buf.BufferSize, buf.VariableCount);
        ImGui::Text("Flags %d, type %d", buf.Flags, buf.BufferType);
        ImGui::Text("Variables:");

        for (u32 j = 0; j < buf.VariableCount; j++) {
            const RDEFVariable& v = vars[j];
            const char* var_name = (const char*)(start + v.NameOffset);
            const auto* type = (RDEFVariableType*)(start + v.TypeOffset);

            ImGui::ScopedIndent indent2(ImGui::CharWidth(4));
            ImGui::Text("'%s' has size %d, type ", var_name, v.Size);

            ImGui::SameLine(); type_text(type->Type);

            ImGui::SameLine(); ImGui::Text(" class ");
            ImGui::SameLine(); class_text(type->Class);
        }
    }

    ImGui::Text("\nResource Bindings:\n");
    for (u32 i = 0; i < part->ResBindingCount; i++) {
        const RDEFResDesc& res = resDescs[i];
        const char* name = (const char*)(start + res.NameOffset);

        ImGui::Separator();
        ImGui::Text("'%s', bind count %d, bind point %d", name, res.BindCount, res.BindPoint);
        ImGui::Text("Input flags %d, type %d", res.InputFlags, res.InputType);
        ImGui::Text("Return type %d, view dimension %d", res.ResReturnType, res.ResViewDimension);
        ImGui::Text("Sample count %d", res.SampleNum);
    }
}

void view_cso(const void* data, u32 size) {
    vfile vf = vfile_open((void*)data, size);
    const CSOHeader* header = VFILE_READ_PTR(CSOHeader, &vf);

    for (u32 i = 0; i < header->PartCount; i++) {
        vf.pos = header->PartOffsets[i];
        const CSOPart* part = VFILE_READ_PTR(CSOPart, &vf);
        if (strncmp(part->Name, "ISGN", 4) == 0) {
            const ISGNPart* isgn = VFILE_READ_PTR(ISGNPart, &vf);
            if (ImGui::CollapsingHeader("Input Elements")) {
                view_isgn(isgn);
            }
        }
        if (strncmp(part->Name, "RDEF", 4) == 0) {
            const RDEFPart* rdef = VFILE_READ_PTR(RDEFPart, &vf);
            if (ImGui::CollapsingHeader("Resource Definitions")) {
                view_rdef(rdef);
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
