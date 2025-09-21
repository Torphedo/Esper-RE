#include <imgui.h>
#include <common/int.h>
#include <common/image.h>
#include <string>

/// std::string::append(), but with printf() formatting syntax.
/// Also appends a newline after the message.
void str_format_append(std::string& output, const char* format_str, ...);

namespace ImGui {
    struct ScopedIndent {
        const float indent;
        ScopedIndent(float indent = 0.0f) : indent(indent) {
            ImGui::Indent(indent);
        }

        ~ScopedIndent() {
            ImGui::Unindent(indent);
        }
    };

    float CharWidth(u32 num_chars = 1);

    // Be careful using this near ImGui::Begin()/End(), because it'll hit an
    // assert if the destructor runs after an ImGui::End().
    struct ScopedWidth {
        ScopedWidth(u32 char_width) {
            ImGui::PushItemWidth(ImGui::CharWidth() * float(char_width));
        }
        ~ScopedWidth() {
            ImGui::PopItemWidth();
        }
    };

    void BeginChildFitContent(const char* id, float width_percent);

    /// @brief A sort of backwards assert that displays a message in the GUI
    ///
    /// @param condition The failure condition. If this is *true*, your custom
    /// message is displayed using ImGui::Text, along with a line asking the
    /// user to report the issue.
    /// @param format A format string for sprintf()
    /// @param ... Format string arguments
    void PlsReportIf(bool condition, const char* format, ...);

    /// @brief Text input box for a Phantom Dust encoded string
    ///
    /// This is for the Phantom Dust text format that encodes 6 characters into
    /// a 32-bit integer. It's commonly used in SSB and ALR files.
    /// @param label Label text to describe the input box
    /// @param text1 The encoded 32-bit integer to modify
    /// @param text2 Optional encoded integer, allowing up to 12 characters total
    /// @return Whether the text was edited
    bool InputPDString(const char* label, u32* text1, u32* text2 = nullptr);

    bool InputCompressedFormat(img_fmt_compressed& fmt, const char* label);

    bool EditTexture(texture& tex) noexcept;

    float ImageScaleForWindow(u16 width, u16 height);
    ImVec2 draw_image(gl_obj tex_id, u16 width, u16 height, bool* scale_to_window, float* scale_factor, const char* id, ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1)) noexcept;

    // These are just less verbose wrappers around ImGui::InputScalar.
    // I don't want to duplicate all the ImGui docs here, so just check
    // ImGui::InputScalar for docs.
    // Sorry for the extremely long function signatures :( - Torph

    bool InputU8(const char* label, u8* data, u8 step = 1, u8 step_fast = 5, const char* format = nullptr, ImGuiInputTextFlags flags = 0);

    bool InputS8(const char* label, s8* data, s8 step = 1, s8 step_fast = 5, const char* format = nullptr, ImGuiInputTextFlags flags = 0);

    bool InputU16(const char* label, u16* data, u16 step = 1, u16 step_fast = 5, const char* format = nullptr, ImGuiInputTextFlags flags = 0);

    bool InputS16(const char* label, s16* data, s16 step = 1, s16 step_fast = 5, const char* format = nullptr, ImGuiInputTextFlags flags = 0);

    bool InputU32(const char* label, u32* data, u32 step = 1, u32 step_fast = 5, const char* format = nullptr, ImGuiInputTextFlags flags = 0);

    bool InputS32(const char* label, s32* data, s32 step = 1, s32 step_fast = 5, const char* format = nullptr, ImGuiInputTextFlags flags = 0);

    bool InputU64(const char* label, u64* data, u64 step = 1, u64 step_fast = 5, const char* format = nullptr, ImGuiInputTextFlags flags = 0);

    bool InputS64(const char* label, s64* data, s64 step = 1, s64 step_fast = 5, const char* format = nullptr, ImGuiInputTextFlags flags = 0);

    struct graph_info {
        void* data;
        u16 count;
        u16 stride;
        u16 offset_x;
        u16 offset_y;
        ImGuiDataType type_x;
        ImGuiDataType type_y;
        ImVec2 translation;
        float scale;
    };

    void GraphData(const graph_info& info);
}
