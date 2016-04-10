#ifndef _loop_h_
#define _loop_h_

#include "common.h"
#include "frame_store.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>

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
status_t loop_frame_timestamp_delta(
	loop_t* p_loop,
	size_t from_frame,
	size_t to_frame,
	timestamp_t* p_timestamp);

#endif
