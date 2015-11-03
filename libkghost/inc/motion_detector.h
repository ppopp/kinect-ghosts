#ifndef _motion_detector_h_
#define _motion_detector_h_

#include "common.h"
#include <stddef.h>

typedef struct motion_detector_s* motion_detector_handle_t;

#ifdef __cplusplus
extern "C" {
#endif
	status_t motion_detector_create(
		size_t pixel_size,
		size_t pixel_count,
		bool_t skip_invalid,
		motion_detector_handle_t* p_handle);
	void motion_detector_release(motion_detector_handle_t handle);

	status_t motion_detector_reset(motion_detector_handle_t handle);
	status_t motion_detector_detect(
		motion_detector_handle_t handle,
		void* depth, 
		int cutoff, 
		double* p_motion, 
		double* p_presence);

#ifdef __cplusplus
}
#endif

#endif
