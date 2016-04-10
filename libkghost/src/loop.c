#include "loop.h"

status_t loop_create(frame_store_handle_t store, loop_t** pp_loop) {
	status_t status = NO_ERROR;
	loop_t* p_loop = NULL;

	if (NULL == pp_loop) {
		return ERR_NULL_POINTER;
	}

	p_loop = (loop_t*)malloc(sizeof(loop_t));
	if (NULL == p_loop) {
		return ERR_FAILED_ALLOC;
	}
	memset(p_loop, 0, sizeof(loop_t));

	p_loop->frame_count = 0;
	p_loop->next_frame = 0;
	p_loop->till_next_frame = 0;
	p_loop->store = store;

	status = vector_create(128, sizeof(void*), &(p_loop->video_addresses));
	if (NO_ERROR != status) {
		loop_release(p_loop);
		return status;
	}
	status = vector_create(128, sizeof(void*), &(p_loop->depth_addresses));
	if (NO_ERROR != status) {
		loop_release(p_loop);
		return status;
	}
	status = vector_create(128, sizeof(frame_id_t), &(p_loop->frame_ids));
	if (NO_ERROR != status) {
		loop_release(p_loop);
		return status;
	}
	status = vector_create(128, sizeof(timestamp_t), &(p_loop->timestamps));
	if (NO_ERROR != status) {
		loop_release(p_loop);
		return status;
	}
	status = vector_create(128, sizeof(float), &(p_loop->cutoffs));
	if (NO_ERROR != status) {
		loop_release(p_loop);
		return status;
	}
	
	*pp_loop = p_loop;
	return NO_ERROR;
}

void loop_release(loop_t* p_loop) {
	frame_id_t* frame_ids = NULL;
	size_t frame_count = 0;
	size_t i = 0;
	status_t status = NO_ERROR;

	if (NULL == p_loop) {
		return;
	}

	/* release frame data from frame store */
	status = vector_array(p_loop->frame_ids, (void**)&frame_ids);
	if (NO_ERROR == status) {
		status = vector_count(p_loop->frame_ids, &frame_count);
		if (NO_ERROR == status) {
			for (i = 0; i < frame_count; i++) {
				status = frame_store_remove_frame(p_loop->store, frame_ids[i]);
				if (NO_ERROR != status) {
					break;
				}
			}
		}
	}

	vector_release(p_loop->video_addresses);
	vector_release(p_loop->depth_addresses);
	vector_release(p_loop->frame_ids);
	vector_release(p_loop->timestamps);
	vector_release(p_loop->cutoffs);
	free(p_loop);
}

status_t loop_frame_timestamp_delta(
	loop_t* p_loop,
	size_t from_frame,
	size_t to_frame,
	timestamp_t* p_timestamp)
{
	status_t    status    = NO_ERROR;
	timestamp_t from_time = 0;
	timestamp_t to_time   = 0;

	status = vector_element_copy(
		p_loop->timestamps,
		from_frame,
		(void*)&from_time);
	if (NO_ERROR != status) {
		return status;
	}
	status = vector_element_copy(
		p_loop->timestamps,
		to_frame,
		(void*)&to_time);
	if (NO_ERROR != status) {
		return status;
	}
	*p_timestamp = to_time - from_time;
	return NO_ERROR;
}

