//#if defined(MAC_X86_32) || defined(MAX_X86_64)

#include "timer.h"
#include "common.h"

#include <stdlib.h>

#include <mach/mach_time.h>

typedef struct timer_s {
	uint64_t start;
	double timebase;
} timer_t;

status_t timer_create(timer_handle_t* p_handle) {
	timer_t* p_timer = NULL;
    mach_timebase_info_data_t timebase_info = { 0 };

	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}

	p_timer = (timer_t*)malloc(sizeof(timer_t));
	if (NULL == p_timer) {
		return ERR_FAILED_ALLOC;
	}

    mach_timebase_info(&timebase_info);

	p_timer->timebase = timebase_info.numer;
	p_timer->timebase /= timebase_info.denom;
	p_timer->start = mach_absolute_time();

	*p_handle = p_timer;
	return NO_ERROR;
}

void timer_release(timer_handle_t handle) {
	free(handle);
}

status_t timer_reset(timer_handle_t handle) {
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	handle->start = mach_absolute_time();

	return NO_ERROR;
}

status_t timer_current(timer_handle_t handle, double* p_time) {
	if ((NULL == handle) || (NULL == p_time)) {
		return ERR_NULL_POINTER;
	}

  	*p_time = (mach_absolute_time() - handle->start) * handle->timebase;
	*p_time /= 1.0e9;

	return NO_ERROR;
}

//#endif
