#include <common/int.h>

namespace ImGui {
    void BeginChildFitContent(const char* id, float width_percent);

    bool PlsReportIf(bool condition, const char* format, ...);
    bool InputPDString(const char* label, u32* text1, u32* text2 = nullptr);
}
