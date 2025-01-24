#include <common/int.h>

namespace ImGui {
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
}
