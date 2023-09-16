#pragma once
#include <stdio.h>

static const char error[] = "31";
static const char warning[] = "33";
static const char info[] = "32";
static const char debug[] = "34";

void disable_logging();
void enable_logging();

/// The function used by the LOG_MSG() macro
/// \param type Color code to show the type of message
/// \param function The name of the function printing the message. You should
/// usually pass in "__func__" which will automatically be the current function
/// name as a string.
/// \param format_str A printf() style format string
/// \param ... Extra arguments to use with the format string, printf() style.
int logging_print(const char* type, const char* function, const char* format_str, ...);

// For main(), LOG_MSG(info, "num = %d\n", 5); would print:
// "\033[32mmain()\033[0m: num = 5\n"
// In the console, this appears as "main(): num = 5" with "main" colored green.


/// A simple logging utility for color-coded output that automatically logs the
/// function name. The outer interface is a macro to avoid passing "__func__"
/// every time. Usage is identical to printf() but with a message type first.
/// \param type\n error = red\n warning = yellow\n info = green\n debug = blue
/// \param ... A format string and extra arguments, just like printf().
#define LOG_MSG(type, ...) logging_print(type, __func__, __VA_ARGS__);

