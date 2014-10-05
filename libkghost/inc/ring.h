#ifndef _ring_h_
#define _ring_h_

#include "common.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct ring_s* ring_handle_t;

	status_t ring_create(
		size_t capacity,
		size_t element_size,
		ring_handle_t* p_handle);

	void ring_release(ring_handle_t handle);
	status_t ring_count(ring_handle_t handle, size_t* p_count);
	status_t ring_capacity(ring_handle_t handle, size_t* p_capacity);
	/* pushes to front of ring so that new element is now at index 0 */
	status_t ring_push(ring_handle_t handle, const void* p_element);
	status_t ring_clear(ring_handle_t handle);
	status_t ring_element(
		ring_handle_t handle, 
		size_t index, 
		void** pp_element);

#ifdef __cplusplus
}
#endif

#endif
