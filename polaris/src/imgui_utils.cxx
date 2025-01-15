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

        u8 size = ENCODED_CHAR_COUNT;
        decoded_text buf = {0};
        decode_single32(buf.data, *text1);
        if (text2 != nullptr) {
            // Also decode the next value
            decode_single32(&buf.data[ENCODED_CHAR_COUNT], *text2);

            // With a second value, we can store twice as many characters.
            size *= 2;
        }

        // TODO: Look into using a character filter callback to only allow the
        // characters that can be encoded.
        const bool edited = ImGui::InputText(label, buf.data, size + 1);

        if (edited) {
            // We need to re-encode the text
            *text1 = encode_single32(buf.data);
            if (text2 != nullptr) {
                *text2 = encode_single32(&buf.data[ENCODED_CHAR_COUNT]);
            }
        }

        return edited;
    }
}

