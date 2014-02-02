#include "log.h"

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

typedef struct _log_listener {
	log_cb callback;
	log_level level;
	struct _log_listener* next;
} log_listener;

static log_listener* _first_listener = NULL;
static size_t _max_message_buffer_size = 1024;

static void _add_listener_to_list(log_listener* listener);
static void _remove_listener_from_list(log_listener* listener);
static log_listener* _find_listener(log_cb callback);

int log_add_callback(log_cb callback, log_level logLevel) {
	log_listener* listener = NULL;

	if (callback == NULL) {
		return -1;
	}

	listener = (log_listener*)malloc(sizeof(log_listener));
	if (listener == NULL) {
		return -1;
	}
	listener->callback = callback;
	listener->level = logLevel;
	listener->next = NULL;
	_add_listener_to_list(listener);
	return 0;
}

void log_remove_callback(log_cb callback) {
	log_listener* listener = NULL;

	listener = _find_listener(callback);
	if (NULL == listener) {
		return;
	}
	_remove_listener_from_list(listener);
	free(listener);
}

const char* log_level_str(log_level level) {
	switch (level) {
		case LogLevelError:
			return "error";

		case LogLevelWarning:
			return "warning";

		case LogLevelInfo:
			return "info";

		case LogLevelDebug:
			return "debug";

		default:
			return "";
	}
}

void log_message(
	const char* source,
	log_level level,
	void* data,
	const char* message)
{
	log_listener* listener = _first_listener;
	while (listener != NULL) {
		if (listener->callback != NULL) {
			if (level <= listener->level) {
				listener->callback(source, level, data, message);
			}
		}
		listener = listener->next;
	}
}

void log_messagef(
	const char* source,
	log_level type,
	void* data,
	const char* format,
	...)
{
	char buffer[_max_message_buffer_size];
	va_list args;

	va_start(args, format);
	vsnprintf(buffer, _max_message_buffer_size, format, args);
	va_end(args);

	log_message(source, type, data, buffer);
}

void _add_listener_to_list(log_listener* listener) {
	log_listener* lastListener = _first_listener;

	if (_first_listener == NULL) {
		_first_listener = listener;
		return;
	}

	while (lastListener->next != NULL) {
		lastListener = lastListener->next;
	}
	lastListener->next = listener;
}

void _remove_listener_from_list(log_listener* listener) {
	log_listener* currListener = _first_listener;
	log_listener* prevListener = NULL;
	log_listener* nextListener = NULL;

	while (currListener != NULL) {
		if (listener == currListener) {
			nextListener = currListener->next;
			if (prevListener != NULL) {
				prevListener->next = nextListener;
			}
			else {
				_first_listener = nextListener;
			}
			return;
		}
		prevListener = currListener;
		currListener = currListener->next;
	}
}

log_listener* _find_listener(log_cb callback) {
	log_listener* listener = _first_listener;
	while (listener != NULL) {
		if (listener->callback == callback) {
			return listener;
		}
		listener = listener->next;
	}
	return NULL;
}

