#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_utils.hxx"

#include <cstdarg>
#include <cstdio>
#include <cmath>

#include <imgui.h>

#include <common/vfile.h>
#include <common/int.h>
#include <common/crc32.h>
#include <formats/ssb.h>

// Minor helper functions for ImGui
namespace ImGui {
void BeginChildFitContent(const char* id, float width_percent) {
    ImGui::BeginChild(id, ImVec2(ImGui::GetContentRegionAvail().x * width_percent, 260), ImGuiChildFlags_ResizeX | ImGuiChildFlags_ResizeY);
}

float CharWidth(u32 num_chars) {
    return ImGui::CalcTextSize("1").x * num_chars;
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

bool InputPDString(const char* label, u32* text1, u32* text2) {
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

    ImGui::SetNextItemWidth(ImGui::CalcTextSize("1").x * (size + 8));

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

bool InputCompressedFormat(img_fmt_compressed& fmt, const char* label) {
    const char* fmt_strings[] = {
        "DXT1/BC1",
        "DXT3/BC2",
        "DXT5/BC3",
        "BC4",
        "RGB 5/6/5",
        "RGBA 5/5/5/1",
        "RGBA 4/4/4/4",
    };
    const char* cur_fmt_name = (fmt >= ARRAY_SIZE(fmt_strings)) ? "UNKNOWN" : fmt_strings[fmt];

    bool result = false;
    if (ImGui::BeginCombo(label, cur_fmt_name)) {
        for (u32 i = 0; i < ARRAY_SIZE(fmt_strings); i++) {
            cur_fmt_name = fmt_strings[i];
            if (ImGui::Selectable(cur_fmt_name, fmt == i)) {
                fmt = (img_fmt_compressed)i;
                result = true;
            }
        }
        ImGui::EndCombo();
    }

    return result;
}

bool EditTexture(texture& tex) noexcept {
    ScopedWidth w(20);
    const u32 hash = crc32fast((u8*)&tex, sizeof(tex));

    ImGui::InputU16("Height", &tex.height);
    ImGui::InputU16("Width", &tex.width);
    ImGui::Checkbox("Mipmaps", &tex.use_mipmaps);

    ImGui::Checkbox("Compressed format", &tex.compressed);
    if (!tex.compressed) {
        ImGui::ScopedIndent indent;
        ImGui::InputU8("# Channels", &tex.channels);
        ImGui::InputU8("Bytes per channel", &tex.unit_size);
    } else {
        ImGui::InputCompressedFormat(tex.fmt, "Format");
    }

    // Use hash to check if changes were made
    return (hash != crc32fast((u8*)&tex, sizeof(tex)));
}

float ImageScaleForWindow(u16 width, u16 height) {
    // We try to fill the space available to us
    const ImVec2 avail = ImGui::GetContentRegionAvail();

    ImVec2 view_size = ImVec2((float)width, (float)height);

    // This is the scale on each axis that'll make the image fill the whole window
    const ImVec2 scale_temp = avail / view_size;

    // We scale by a uniform factor to preserve aspect ratio, so pick the
    // closer axis (to keep the entire image in frame)

    // Sometimes the scale ends up negative and I'm not sure why, so I just threw an fabsf() on it.
    // - torph
    const float new_scale = fabsf(MIN(scale_temp.x, scale_temp.y));

    return new_scale;
}

ImVec2 draw_image(gl_obj tex_id, u16 width, u16 height, bool* scale_to_window, float* scale_factor, const char* id, ImVec2 uv0, ImVec2 uv1) noexcept {
    // We need unique labels every time, so just combine some values that are
    // usually different. Texture ID is the same in a texture atlas and its
    // contents. This isn't foolproof but it works.
    char label[0x20] = {0};
    snprintf(label, sizeof(label), "Scale to window##%d%lf%s", tex_id, uv1.x, id);
    ImGui::Checkbox(label, scale_to_window);

    if (*scale_to_window) {
        // Force view size == texture size to make auto-scaling work
        *scale_factor = 1.0f;
    } else {
        snprintf(label, sizeof(label), "Render Scale ##%d%lf%s", tex_id, uv1.x, id);
        ImGui::SetNextItemWidth(ImGui::CharWidth() * 16);
        ImGui::SliderFloat(label, scale_factor, 0.001f, 10.0f);
    }

    // Scale the texture depending on the current settings.
    ImVec2 view_size = ImVec2((float)width, (float)height);
    if (*scale_to_window) {
        *scale_factor = ImageScaleForWindow(width, height);
    }
    view_size *= *scale_factor;

    const ImVec2 image_pos = ImGui::GetCursorScreenPos();
    // TODO: Look into showing mipmap contents
    ImGui::Image(tex_id, view_size, uv0, uv1);

    return image_pos;
}

void GraphData(const graph_info& info) {
    const float scale = 10.0f;

    const ImVec2 canvas_size(300, 300);
    const ImVec2 canvas_start = ImGui::GetCursorScreenPos();
    const ImVec2 canvas_end = canvas_start + canvas_size;
    const ImVec2 canvas_center = canvas_start + (ImVec2(0, canvas_size.y / 2));
    ImGui::InvisibleButton("canvas", canvas_size, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);

    // Draw graph background
    ImDrawList* drawlist = ImGui::GetWindowDrawList();
    drawlist->AddRectFilled(canvas_start, canvas_end, IM_COL32(50, 50, 50, 255));
    drawlist->AddRect(canvas_start, canvas_end, IM_COL32(255, 255, 255, 255));

    vfile vf = vfile_open(info.data, info.count * info.stride);

    ImVec2 prev_coord(0, 0);
    for (u32 i = 0; i < info.count; i++) {
        const u64 next_pos = vf.pos + info.stride;
        float frame = 0.0f;
        switch (info.type_x) {
        case ImGuiDataType_Float:
            frame = VFILE_READ(float, &vf);
            break;
        case ImGuiDataType_U8:
            frame = VFILE_READ(u8, &vf);
            break;
        default:
            break;
        }

        // Skip to the component we want and read it
        vfile_seek(&vf, info.offset_y);
        float val = 0.0f;
        switch (info.type_y) {
        case ImGuiDataType_Float:
            val = VFILE_READ(float, &vf);
            break;
        case ImGuiDataType_U16:
            val = (VFILE_READ(u16, &vf)) / (float)INT16_MAX;
            break;
        default:
            break;
        }

        const ImVec2 cur_coord = canvas_center + ImVec2(frame, -val) * scale;
        if (i == 0) {
            prev_coord = cur_coord;
        } else {
            drawlist->AddLine(prev_coord, cur_coord, 0xFF0000FF);
        }
        prev_coord = cur_coord;

        // Skip to next key
        vf.pos = next_pos;
    }
}

} // namespace ImGui
