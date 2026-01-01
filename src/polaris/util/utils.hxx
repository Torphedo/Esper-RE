#pragma once
#include <string>

/// std::string::append(), but with printf() formatting syntax.
/// Also appends a newline after the message.
void str_format_append(std::string& output, const char* format_str, ...);

