#ifndef _stack_h_
#define _stack_h_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef void* stack_value_t;
	typedef struct stack_s* stack_handle_t;

	status_t stack_create(size_t maxSize, stack_handle_t* p_handle);
	status_t stack_release(stack_handle_t handle);
	status_t stack_push(stack_handle_t handle, stack_value_t value);
	status_t stack_pop(stack_handle_t handle, stack_value_t* p_value);

	status_t stack_empty(stack_handle_t handle, int* flag);
	status_t stack_full(stack_handle_t handle, int* flag);
	status_t stack_count(stack_handle_t handle, size_t* count);

#ifdef __cplusplus
}
#endif

#endif  
