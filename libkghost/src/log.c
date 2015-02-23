#include "log.h"
#include <stdarg.h>

static int _log_not_initialized = 1;
static void _log_initialize();

static FILE* _log_error_stream = NULL;
static FILE* _log_warning_stream = NULL;
static FILE* _log_info_stream = NULL;
static FILE* _log_debug_stream = NULL;

static const char* _log_format = "[%s %s:%d]";

void log_error(const char* file, int line, const char* format, ...) {
	_log_initialize();
	if (_log_error_stream) {
		fprintf(_log_error_stream, _log_format, "error", file, line);
		va_list args;
	 	va_start(args, format);
		vfprintf(_log_error_stream, format, args);
		va_end(args);
		fprintf(_log_error_stream, "\n");
	}
}

void log_warning(const char* file, int line, const char* format, ...) {
	_log_initialize();
	if (_log_warning_stream) {
		fprintf(_log_warning_stream, _log_format, "warning", file, line);
		va_list args;
	 	va_start(args, format);
		vfprintf(_log_warning_stream, format, args);
		va_end(args);
		fprintf(_log_warning_stream, "\n");
	}
}

void log_info(const char* file, int line, const char* format, ...) {
	_log_initialize();
	if (_log_info_stream) {
		fprintf(_log_info_stream, _log_format, "info", file, line);
		va_list args;
	 	va_start(args, format);
		vfprintf(_log_info_stream, format, args);
		va_end(args);
		fprintf(_log_info_stream, "\n");
	}
}

void log_debug(const char* file, int line, const char* format, ...) {
	_log_initialize();
	if (_log_debug_stream) {
		fprintf(_log_debug_stream, _log_format, "debug", file, line);
		va_list args;
	 	va_start(args, format);
		vfprintf(_log_debug_stream, format, args);
		va_end(args);
		fprintf(_log_debug_stream, "\n");
	}
}

void log_set_stream(FILE* file, log_level_t level) {
	_log_initialize();
	switch (level) {
		case LOG_LEVEL_ERROR:
			_log_error_stream = file;
			break;
		case LOG_LEVEL_WARNING:
			_log_warning_stream = file;
			break;
		case LOG_LEVEL_INFO:
			_log_info_stream = file;
			break;
		case LOG_LEVEL_DEBUG:
			_log_debug_stream = file;
			break;
	}
}

void log_set_stream_all(FILE* file) {
	_log_initialize();
	_log_error_stream = file;
	_log_warning_stream = file;
	_log_info_stream = file;
	_log_debug_stream = file;
}

void _log_initialize() {
	if (_log_not_initialized) {
		_log_error_stream = stderr;
		_log_warning_stream = stderr;
		_log_info_stream = stdout;
		_log_debug_stream = NULL;
		_log_not_initialized = 0;
	}
}
