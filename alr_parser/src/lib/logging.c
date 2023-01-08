#include <stdio.h>

#include "logging.h"

unsigned int ansi_enabled = 0;

#ifdef _WIN32
#include <Windows.h>

void enable_win_ansi()
{
    DWORD prev_console_mode;
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (console_handle  != INVALID_HANDLE_VALUE) {
        GetConsoleMode(console_handle , (LPDWORD) &prev_console_mode);
        if (prev_console_mode != 0) {
            SetConsoleMode(console_handle , prev_console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT);
            ansi_enabled = 1;
        }
    }
}

#else
ansi_enabled = 1;
#endif

void log_error(error_type error, char* format_str, ...)
{
    if (ansi_enabled == 0)
    {
        enable_win_ansi();
    }
    switch (error) {
    case INFO:
        printf("[\033[32mINFO\033[0m] ");
        break;
    case WARNING:
        printf("[\033[33mWARNING\033[0m] ");
        break;
    case CRITICAL:
        printf("[\033[31mCRITICAL\033[0m] ");
    }

    va_list arg_list;
    va_start(arg_list, format_str);
    vprintf(format_str, arg_list);
    va_end(arg_list);
}
