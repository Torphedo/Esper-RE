#pragma once
#include <string>
#include <cglm/struct.h>
#include "formats/pd_common.h"

/// std::string::append(), but with printf() formatting syntax.
/// Also appends a newline after the message.
void str_format_append(std::string& output, const char* format_str, ...);

bool box_in_frustum(mat4s xform, vec3s min, vec3s max);
static bool box_in_frustum(mat4s xform, vec3f corner_min, vec3f corner_max) {
    return box_in_frustum(xform, *(vec3s*)&corner_min, *(vec3s*)&corner_max);
}
