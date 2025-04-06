#include <cstdarg>
#include <cstdio>
#include <imgui.h>

#include <common/int.h>
#include <formats/pd_common.h>

// Minor helper functions for ImGui
namespace ImGui {
    void BeginChildFitContent(const char* id, float width_percent) {
        ImGui::BeginChild(id, ImVec2(ImGui::GetContentRegionAvail().x * width_percent, 260), ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);
    }

    void PlsReportIf(bool condition, const char* format, ...) {
        if (!condition) {
            // Failure condition wasn't hit, everything's fine.
            return;
        }

        // We use helpers from stdarg to handle the variadic (...) arguments.
        char msg[8192] = {0};
        va_list arg_list = {};
        va_start(arg_list, format);
        const int return_code = vsnprintf(msg, sizeof(msg) - 1, format, arg_list);
        va_end(arg_list);

        if (strlen(format) > sizeof(msg) - 1) {
            ImGui::Text("%s(): [Programmer error] message was too long to display.\n", __func__);
        }

        ImGui::Text("WARNING: %s", msg);
        ImGui::Text("Please report this so I can research it.");
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

        ImGui::SetNextItemWidth(ImGui::CalcTextSize("1").x * (size + 2));

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

    // Beyond this point are lots of InputScalar wrappers for common integer sizes

    bool InputU8(const char* label, u8* data, u8 step, u8 step_fast, const char* format, ImGuiInputTextFlags flags) {
        return ImGui::InputScalar(label, ImGuiDataType_U8, data, &step, &step_fast, format, flags);
    }

    bool InputS8(const char* label, s8* data, s8 step, s8 step_fast, const char* format, ImGuiInputTextFlags flags) {
        return ImGui::InputScalar(label, ImGuiDataType_S8, data, &step, &step_fast, format, flags);
    }

    bool InputU16(const char* label, u16* data, u16 step, u16 step_fast, const char* format, ImGuiInputTextFlags flags) {
        return ImGui::InputScalar(label, ImGuiDataType_U16, data, &step, &step_fast, format, flags);
    }

    bool InputS16(const char* label, s16* data, s16 step, s16 step_fast, const char* format, ImGuiInputTextFlags flags) {
        return ImGui::InputScalar(label, ImGuiDataType_S16, data, &step, &step_fast, format, flags);
    }

    bool InputU32(const char* label, u32* data, u32 step, u32 step_fast, const char* format, ImGuiInputTextFlags flags) {
        return ImGui::InputScalar(label, ImGuiDataType_U32, data, &step, &step_fast, format, flags);
    }

    bool InputS32(const char* label, s32* data, s32 step, s32 step_fast, const char* format, ImGuiInputTextFlags flags) {
        return ImGui::InputScalar(label, ImGuiDataType_S32, data, &step, &step_fast, format, flags);
    }

    bool InputU64(const char* label, u64* data, u64 step, u64 step_fast, const char* format, ImGuiInputTextFlags flags) {
        return ImGui::InputScalar(label, ImGuiDataType_U64, data, &step, &step_fast, format, flags);
    }

    bool InputS64(const char* label, s64* data, s64 step, s64 step_fast, const char* format, ImGuiInputTextFlags flags) {
        return ImGui::InputScalar(label, ImGuiDataType_S64, data, &step, &step_fast, format, flags);
    }
}
