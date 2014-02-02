#ifndef _log_h_
#define _log_h_

#define LOG_LEVEL_NONE (-1)
#define LOG_LEVEL_ERROR (0)
#define LOG_LEVEL_WARNING (1)
#define LOG_LEVEL_INFO (2)
#define LOG_LEVEL_DEBUG (3)

#ifndef LOG_LEVEL
	#ifdef NDEBUG
		#define LOG_LEVEL (LOG_LEVEL_ERROR)
	#else
		#define LOG_LEVEL (LOG_LEVEL_DEBUG)
	#endif
#endif

#define LOG_HELPER(source, level, data, message) \
{\
	const char* _log_msg = (message);\
	const char* _log_source = (source);\
	void* _log_data = (data);\
	log_messagef(_log_source,\
				 level,\
				 _log_data,\
				 "%s @%s:%u",\
				 _log_msg,\
				 __FILE__,\
			     __LINE__);\
}

#if LOG_LEVEL >= LOG_LEVEL_ERROR
	#define LOG_ERROR(source, data, message) \
		LOG_HELPER(source, LogLevelError, data, message)
#else
	#define LOG_ERROR(source, data, message)
#endif

#if LOG_LEVEL >= LOG_LEVEL_WARNING
	#define LOG_WARNING(source, data, message) \
		LOG_HELPER(source, LogLevelWarning, data, message)
#else
	#define LOG_WARNING(source, data, message)
#endif

#if LOG_LEVEL >= LOG_LEVEL_INFO
	#define LOG_INFO(source, data, message) \
		LOG_HELPER(source, LogLevelInfo, data, message)
#else
	#define LOG_INFO(source, data, message)
#endif

#if LOG_LEVEL >= LOG_LEVEL_DEBUG
	#define LOG_DEBUG(source, data, message) \
		LOG_HELPER(source, LogLevelDebug, data, message)
#else
	#define LOG_DEBUG(source, data, message)
#endif

#ifdef __cplusplus
extern "C" {
#endif

	typedef enum _log_level {
		LogLevelError = 0,
		LogLevelWarning = 1,
		LogLevelInfo = 2,
		LogLevelDebug = 3
	} log_level;

	typedef void (*log_cb)(
			const char* source, 
			log_level level,
			void* data,
			const char* message);

	int log_add_callback(log_cb callback, log_level logLevel);
	void log_remove_callback(log_cb callback);
	const char* log_level_str(log_level level);

	void log_message(
		const char* source,
		log_level level,
		void* data,
		const char* message);

	void log_messagef(
		const char* source,
		log_level level,
		void* data,
		const char* format,
		...);

#ifdef __cplusplus
}
#endif

#endif
