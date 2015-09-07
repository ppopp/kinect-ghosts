#include "common.h"


const char* error_string(status_t code) {
	switch (code) {
		case NO_ERROR:
			return "no error";
		case ERR_NULL_POINTER:
			return "null pointer";
		case ERR_FAILED_ALLOC:
			return "failed alloc";
		case ERR_FAILED_CREATE:
			return "failed create";
		case ERR_EMPTY:
			return "empty";
		case ERR_FAILED_THREAD_CREATE:
			return "failed thread create";
		case ERR_RANGE_ERROR:
			return "range error";
		case ERR_INVALID_ARGUMENT:
			return "invalid argument";
		case ERR_EXCEED_ERROR:
			return "exceed error";
		case ERR_FAILED_EXPORT:
			return "failed export";
		case ERR_MUTEX_ERROR:
			return "mutex error";
		case ERR_FULL:
			return "full";
		case ERR_INVALID_TIMESTAMP:
			return "invalid timestamp";
		case ERR_FAILED_TIMER:
			return "failed timer";
		case ERR_DEVICE_ERROR:
			return "device error";
	}
	return "unhandled error";
}

