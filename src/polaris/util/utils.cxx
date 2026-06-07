#include "utils.hxx"
#include "scope_timer.hxx"
#include <stdarg.h>

void str_format_append(std::string& output, const char* format_str, ...) {
    char buf[2048] = {0};

    // We use helpers from stdarg.h to handle the variadic (...) arguments.
    va_list arg_list = {};
    va_start(arg_list, format_str);
    const int return_code = vsnprintf(buf, sizeof(buf) - 1, format_str, arg_list);
    va_end(arg_list);

    output.append(buf);
    output.append("\n");
}

bool box_in_frustum(mat4s xform, vec3s min, vec3s max) {
    scope_timer overallTimer("calcFrustumCulling", true);
    vec4s planes[6] = {};
    glms_frustum_planes(xform, planes);
    vec3s objBox[2] = {min, max};
    if (glms_aabb_frustum(objBox, planes)) {
        return true;
    }
    return false;

}
