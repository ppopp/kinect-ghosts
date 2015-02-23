#ifndef _vector_h_
#define _vector_h_

#include "common.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

	/** \addtogroup vector
	 * @{
	 */

	typedef struct vector_s* vector_handle_t;

	status_t vector_create(
		size_t initial_capacity,
		size_t element_size,
		vector_handle_t* p_handle);

	void vector_release(vector_handle_t handle);

	status_t vector_count(vector_handle_t handle, size_t* p_count);
	status_t vector_capacity(vector_handle_t handle, size_t* p_capacity);
	status_t vector_append(vector_handle_t handle, void* p_element);
	status_t vector_pop(vector_handle_t handle);
	status_t vector_clear(vector_handle_t handle);
	status_t vector_element(
		vector_handle_t handle, 
		size_t index, 
		void** pp_element);
	status_t vector_remove(vector_handle_t handle, size_t index);
	status_t vector_array(
		vector_handle_t handle, 
		void** pp_first_element);

	/** @} */

#ifdef __cplusplus
}
#endif

#endif
