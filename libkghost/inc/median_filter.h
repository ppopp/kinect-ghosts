#ifndef _median_filter_h_
#define _median_filter_h_

#include "common.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	/** \addtogroup median_filter
	 *  @{
	 */
	typedef struct median_filter_s* median_filter_handle_t;

	typedef struct median_filter_shape_s {
		size_t x;
		size_t y;
		size_t z;
	} median_filter_shape_t;

	typedef struct median_filter_input_spec_s {
		size_t element_size;
		size_t channel_count;
		size_t width;
		size_t height;
	} median_filter_input_spec_t;

	
	status_t median_filter_create(
		median_filter_input_spec_t input_spec,
		median_filter_shape_t filter_shape,
		comparison_function compare,
		median_filter_handle_t* p_handle);

	void median_filter_release(median_filter_handle_t handle);

	status_t median_filter_append(median_filter_handle_t handle, void* data);

	status_t median_filter_apply(median_filter_handle_t handle);

	status_t median_filter_clear(median_filter_handle_t handle);
	/** @} */

#ifdef __cplusplus
}
#endif

#endif
