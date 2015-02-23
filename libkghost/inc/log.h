#ifndef _log_h_
#define _log_h_

#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


	typedef enum {
		LOG_LEVEL_ERROR,
		LOG_LEVEL_WARNING,
		LOG_LEVEL_INFO,
		LOG_LEVEL_DEBUG
	} log_level_t;

	void log_error(const char* file, int line, const char* format, ... );
	void log_warning(const char* file, int line, const char* format, ...);
	void log_info(const char* file, int line, const char* format, ...);
	void log_debug(const char* file, int line, const char* format, ...);

	void log_set_stream(FILE* file, log_level_t level);
	void log_set_stream_all(FILE* file);


#ifdef __cplusplus
}
#endif

#define LOG_ERROR(format, ...) log_error (__FILE__, __LINE__, format, ##__VA_ARGS__);
#define LOG_WARNING(format, ...) log_warning (__FILE__, __LINE__, format, ##__VA_ARGS__);
#define LOG_INFO(format, ...) log_info(__FILE__, __LINE__, format, ##__VA_ARGS__);
#define LOG_DEBUG(format, ...) log_debug(__FILE__, __LINE__, format, ##__VA_ARGS__);

#endif
