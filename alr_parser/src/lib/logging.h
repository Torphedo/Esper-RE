#pragma once

typedef enum error_type {
	INFO = 0,
	WARNING = 1,
	CRITICAL = 2
}error_type;

void enable_win_ansi();
void log_error(error_type error, char* format_str, ...);