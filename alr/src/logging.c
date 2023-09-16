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

