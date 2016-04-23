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

status_t loop_get_frame(
	loop_t* p_loop,
	void** video_frame,
	void** depth_frame,
	float* cutoff)
{
	status_t status = NO_ERROR;

	if (NULL == p_loop) {
		return ERR_NULL_POINTER;
	}
	status = vector_element_copy(
		p_loop->video_addresses,
		p_loop->next_frame,
		(void*)video_frame);
	if (NO_ERROR != status) {
		return status;
	}
	status = vector_element_copy(
		p_loop->depth_addresses,
		p_loop->next_frame,
		(void*)depth_frame);
	if (NO_ERROR != status) {
		return status;
	}
	status = vector_element_copy(
		p_loop->cutoffs,
		p_loop->next_frame,
		(void*)cutoff);
	if (NO_ERROR != status) {
		return status;
	}
	return NO_ERROR;
}

status_t loop_advance_playhead(loop_t* p_loop, timestamp_t delta, size_t* frames_left) {
	status_t    status    = NO_ERROR;
	timestamp_t timestamp = 0;

	if ((NULL == p_loop) || (NULL == frames_left)) {
		return ERR_NULL_POINTER;
	}

	/* increment frame in loop */
	if (delta > p_loop->till_next_frame) {
		/* get time till next frame */
		timestamp = delta - p_loop->till_next_frame;
		while (timestamp > 0) {
			/* increment frame */
			p_loop->next_frame++;
			if (p_loop->next_frame >= (p_loop->frame_count - 1)) {
				break;
			}
			else {
				/* get difference between two frame timestamps */
				status = loop_frame_timestamp_delta(
						p_loop,
						p_loop->next_frame,
						p_loop->next_frame + 1,
						&(p_loop->till_next_frame));
				if (NO_ERROR != status) {
					return status;
				}
				if (timestamp > p_loop->till_next_frame) {
					timestamp -= p_loop->till_next_frame;
				}
				else {
					p_loop->till_next_frame -= timestamp;
					timestamp = -1;
				}
			}
		}
	}
	else {
		p_loop->till_next_frame -= delta;
	}

	if (p_loop->next_frame < p_loop->frame_count) {
		*frames_left = p_loop->frame_count - p_loop->next_frame;
	}
	else {
		*frames_left = 0;
	}

	return NO_ERROR;
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

