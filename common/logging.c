#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

bool print_msgs = true;

void disable_logging() {
    print_msgs = false;
}

void enable_logging() {
    print_msgs = true;
}

#ifdef _WIN32
#include <windows.h>
#endif

// Enables ANSI escape codes on Windows
unsigned short enable_win_ansi() {
#ifdef _WIN32
    DWORD prev_console_mode;
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console_handle  != INVALID_HANDLE_VALUE) {
        GetConsoleMode(console_handle , (LPDWORD) &prev_console_mode);
        if (prev_console_mode != 0) {
            SetConsoleMode(console_handle , prev_console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
            return 1;
        }
    }
#endif
    return 0;
}

int logging_print(char* type, char* function, char* format_str, ...) {
    if (!print_msgs) {
        return 0;
    }
    // Print "__func__(): " with function name in color and the rest in white
    printf("\033[%sm%s\033[0m(): ", type, function);

    va_list arg_list = {0};
    va_start(arg_list, format_str);
    int return_code = vprintf(format_str, arg_list);
    va_end(arg_list);

    return return_code;
}

