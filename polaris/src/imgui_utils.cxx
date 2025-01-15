#include <imgui.h>

#include <common/int.h>
#include <formats/pd_common.h>

// Minor helper functions for ImGui
namespace ImGui {
    void BeginChildFitContent(const char* id, float width_percent) {
        ImGui::BeginChild(id, ImVec2(ImGui::GetContentRegionAvail().x * width_percent, 260), ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);
    }

    bool InputPDString(const char* label, u32* text1, u32* text2 = nullptr) {
        if (text1 == nullptr) {
            // We need something to edit...
            return false;
        }

        u8 size = PD_ENCODED_CHAR_COUNT / 2;
        char buf[PD_ENCODED_CHAR_COUNT + 1] = {0};
        decode_single32(buf, *text1);
        if (text2 != nullptr) {
            // Also decode the next value
            decode_single32(&buf[size], *text2);

            // With a second value, we can store twice as many characters.
            size *= 2;
        }

        // TODO: Look into using a character filter callback to only allow the
        // characters that can be encoded.
        const bool edited = ImGui::InputText(label, buf, size);

        if (edited) {
            char* text = buf;
            // We need to re-encode the text
            *text1 = encode_single32(text);
            if (text2 != nullptr) {
                *text2 = encode_single32(&text[6]);
            }
        }

        return edited;
    }
}

