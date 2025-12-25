#include <imgui.h>
#include <common/int.h>
#include <common/image.h>
#include <string>

/// std::string::append(), but with printf() formatting syntax.
/// Also appends a newline after the message.
void str_format_append(std::string& output, const char* format_str, ...);

/// @brief Define an explcitly sized variant of ImGui::InputScalar()
/// @param T Type name to use in the function and enum name (e.g. U8)
/// @param U Type name to use for arguments (e.g. uint8_t)
#define IMPL_IMGUI_SCALAR_INPUT(T, U)                                                            \
static bool Input##T(const char* label, U* data, U step = 1, U step_fast = 5,                    \
                     const char* format = nullptr, ImGuiInputTextFlags flags = 0) {              \
    return ImGui::InputScalar(label, ImGuiDataType_##T, data, &step, &step_fast, format, flags); \
}                                                                                                \

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

    // Table to convert our data type enum to ImGui's
    static const ImGuiDataType type_table[] = {
        ImGuiDataType_S8, ImGuiDataType_U8,
        ImGuiDataType_S16, ImGuiDataType_U16,
        ImGuiDataType_S32, ImGuiDataType_U32,
        ImGuiDataType_Float, ImGuiDataType_Double,
        ImGuiDataType_COUNT,
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

    /// @brief Combo box (dropdown) input for a compressed texture format
    ///
    /// This uses the format enums from the bobtail library.
    /// @param fmt Compressed texture format enum to edit
    /// @param label User-facing label for the input
    /// @return Whether the user changed the format value
    bool InputCompressedFormat(img_fmt_compressed& fmt, const char* label);

    bool EditTexture(texture& tex) noexcept;

    /// @brief Get a scale factor that will make an image fill the available space
    ///
    /// This is a *uniform* scale factor to be applied on both axes.
    /// @param width Image width
    /// @param height Image height
    /// @return Uniform scale factor
    float ImageScaleForWindow(u16 width, u16 height);
    ImVec2 draw_image(gl_obj tex_id, u16 width, u16 height, bool* scale_to_window, float* scale_factor, const char* id, ImVec2 uv0 = ImVec2(0, 0), ImVec2 uv1 = ImVec2(1, 1)) noexcept;

    // These are ImGui::InputScalar wrappers, check that function for docs.
    IMPL_IMGUI_SCALAR_INPUT(U8, u8)
    IMPL_IMGUI_SCALAR_INPUT(S8, s8)
    IMPL_IMGUI_SCALAR_INPUT(U16, u16)
    IMPL_IMGUI_SCALAR_INPUT(S16, s16)
    IMPL_IMGUI_SCALAR_INPUT(U32, u32)
    IMPL_IMGUI_SCALAR_INPUT(S32, s32)
    IMPL_IMGUI_SCALAR_INPUT(U64, u64)
    IMPL_IMGUI_SCALAR_INPUT(S64, s64)

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
