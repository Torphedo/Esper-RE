#include "utils.hxx"
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