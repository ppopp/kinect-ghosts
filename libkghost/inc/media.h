#ifndef _media_h_
#define _media_h_

#include "common.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif 

	typedef int media_type_flag;
	typedef struct media_s* media_handle_t;

	status_t media_get_type(
		media_handle_t handle, 
		media_type_flag* p_type_flag);
	status_t media_get_frame_count(media_handle_t handle, size_t* p_count);
	status_t media_get_frame_size(media_handle_t handle, size_t* p_size);
	status_t media_get_frame(
		media_handle_t handle, 
		size_t index, 
		void** p_frame);
	status_t media_get_frame_rate(media_handle_t handle, double* p_rate);
	status_t media_get_dimension_count(media_handle_t handle, size_t* p_count);
	status_t media_get_dimension_size(
		media_handle_t handle, 
		size_t dimension, 
		size_t* p_size);

#ifdef __cplusplus
}
#endif

#endif
