#ifndef _timer_h_
#define _timer_h_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct timer_s* timer_handle_t;

	status_t timer_create(timer_handle_t* p_handle);
	void timer_release(timer_handle_t handle);

	status_t timer_reset(timer_handle_t handle);
	status_t timer_current(timer_handle_t handle, double* p_time);

#ifdef __cplusplus
}
#endif

#endif
