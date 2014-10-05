#ifndef _freerec_h_
#define _freerec_h_

#include "common.h"

#include <libfreenect/libfreenect.h>

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct freerec_s* freerec_handle_t;
	typedef double timestamp_t;
	/*
	typedef unsigned timestamp_t;
	*/

	status_t freerec_create(
		freenect_frame_mode* p_video_mode,
		freenect_frame_mode* p_depth_mode,
		size_t max_bytes,
		freerec_handle_t* p_handle);

	void freerec_release(freerec_handle_t handle);

	status_t freerec_action(freerec_handle_t handle);

	status_t freerec_clip_count(freerec_handle_t handle, size_t* p_count);

	status_t freerec_clip_frame_count(
		freerec_handle_t handle, 
		size_t clip_index,
		size_t* p_count);

	status_t freerec_clip_video_frame(
		freerec_handle_t handle,
		size_t clip_index,
		size_t frame_index,
		void** p_data,
		timestamp_t* p_timestamp);

	status_t freerec_clip_depth_frame(
		freerec_handle_t handle,
		size_t clip_index,
		size_t frame_index,
		void** p_data,
		timestamp_t* p_timestamp);


	status_t freerec_capture_video(
		freerec_handle_t handle,
		void* data,
		timestamp_t timestamp);

	status_t freerec_capture_depth(
		freerec_handle_t handle,
		void* data,
		timestamp_t timestamp);

#ifdef __cplusplus
}
#endif


#endif
