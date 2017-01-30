#ifndef _loop_h_
#define _loop_h_

#include "common.h"
#include "frame_store.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>
#ifdef __cplusplus
extern "C" {
#endif
	/** \addtogroup loop
	 * @{
	 */

	/* loops hold series of frames that can be repeated */
	typedef struct loop_s {
		frame_store_handle_t store;
		vector_handle_t video_addresses;
		vector_handle_t depth_addresses;
		vector_handle_t cutoffs;
		vector_handle_t timestamps;
		vector_handle_t frame_ids;
		timestamp_t till_next_frame;
		size_t next_frame;
		size_t frame_count;
	} loop_t;


	status_t loop_create(frame_store_handle_t store, loop_t** pp_loop);
	void loop_release(loop_t* p_loop);

	status_t loop_get_frame(
		loop_t* p_loop,
		size_t index,
		frame_id_t* frame_id,
		timestamp_t* timestamp,
		void** video_frame,
		void** depth_frame,
		float* cutoff);

	status_t loop_remove_frame(loop_t* p_loop, frame_id_t frame_id);

	status_t loop_get_next_frame(
		loop_t* p_loop,
		void** video_frame,
		void** depth_frame,
		float* cutoff);
	status_t loop_advance_playhead(
		loop_t* p_loop, 
		timestamp_t delta, 
		size_t* p_frames_left);
	status_t loop_frame_timestamp_delta(
		loop_t* p_loop,
		size_t from_frame,
		size_t to_frame,
		timestamp_t* p_timestamp);

	status_t loop_duration(
		loop_t* p_loop,
		timestamp_t* p_duration);
	/** @} */

#ifdef __cplusplus
}
#endif

#endif
