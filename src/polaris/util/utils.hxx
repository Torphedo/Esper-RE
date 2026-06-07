#pragma once
#include <string>
#include <cglm/struct.h>

/// std::string::append(), but with printf() formatting syntax.
/// Also appends a newline after the message.
void str_format_append(std::string& output, const char* format_str, ...);

bool box_in_frustum(mat4s xform, vec3s min, vec3s max);
static bool box_in_frustum(mat4s xform, const float corner_min[3], const float corner_max[3]) {
    return box_in_frustum(xform, *(vec3s*)corner_min, *(vec3s*)corner_max);
}
