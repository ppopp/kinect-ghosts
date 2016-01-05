#include "director.h"
#include "frame_store.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>

typedef struct loop_s {
	vector_handle_t video_addresses;
	vector_handle_t depth_addresses;
	vector_handle_t cutoffs;
	vector_handle_t frame_ids;
	size_t next_frame;
	size_t frame_count;
} loop_t;

static status_t _loop_create(loop_t** pp_loop);
static void _loop_release(loop_t* p_loop);

	
typedef struct director_s {
	size_t max_layers;
	size_t max_bytes;
	size_t bytes_per_video_frame;
	size_t bytes_per_depth_frame;
	frame_store_handle_t frame_store;
	vector_handle_t loops;
	loop_t* p_current_loop;
} director_t;

static status_t _director_handle_new_frame(director_t* p_director, frame_id_t frame_id);
static status_t _director_handle_new_loop(director_t* p_director, loop_t* p_loop);

status_t director_create(
	size_t max_layers,
	size_t max_bytes, 
	size_t bytes_per_video_frame,
	size_t bytes_per_depth_frame,
	director_handle_t* p_handle)
{
	status_t status = NO_ERROR;
	director_t* p_director = NULL;

	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}

	if (max_layers > DIRECTOR_MAX_LAYERS) {
		return ERR_INVALID_ARGUMENT;
	}

	p_director = malloc(sizeof(director_t));
	if (NULL == p_director) {
		return ERR_FAILED_ALLOC;
	}
	memset(p_director, 0, sizeof(director_t));

	p_director->max_layers = max_layers;
	p_director->max_bytes = max_bytes;
	p_director->bytes_per_video_frame = bytes_per_video_frame;
	p_director->bytes_per_depth_frame = bytes_per_depth_frame;

	status = frame_store_create(
		bytes_per_video_frame,
		bytes_per_depth_frame,
		sizeof(float), // depth cutoff storage
		max_bytes,
		&(p_director->frame_store));
	if (NO_ERROR != status) {
		director_release(p_director);
		return status;
	}

	status = vector_create(32, sizeof(loop_t*), &(p_director->loops));
	if (NO_ERROR != status) {
		director_release(p_director);
		return status;
	}

	status = _loop_create(&(p_director->p_current_loop));
	if (NO_ERROR != status) {
		director_release(p_director);
		return status;
	}

	*p_handle = p_director;

	return NO_ERROR;
}

void director_release(director_handle_t handle) {
	size_t count = 0;
	size_t i = 0;
	loop_t* p_loop = NULL;
	status_t status = NO_ERROR;

	if (NULL == handle) {
		return;
	}
	
	vector_count(handle->loops, &count);
	for (i = 0; i < count; i++) {
		status = vector_element_copy(handle->loops, i, (void**)&p_loop);
		if (NO_ERROR != status) {
			_loop_release(p_loop);
		}
	}

	frame_store_release(handle->frame_store);
	vector_release(handle->loops);
	_loop_release(handle->p_current_loop);

	free(handle);
}

status_t director_playback_layers(
	director_handle_t handle, 
	timestamp_t play_time, 
	director_frame_layers_t* p_layers)
{
	status_t status      = NO_ERROR;
	size_t   count       = 0;
	size_t   start_layer = 0;
	size_t   end_layer   = 0;
	size_t   loop_index = 0;
	size_t   layer_index = 0;
	loop_t   *p_loop;

	/* TODO: add any needed playback logic */
	if ((NULL == handle) || (NULL == p_layers)) {
		return ERR_NULL_POINTER;
	}

	status = vector_count(handle->loops, &count);
	if (NO_ERROR != status) {
		return status;
	}

	if (count > handle->max_layers) {
		start_layer = count - handle->max_layers;
		end_layer = count;
		p_layers->layer_count = handle->max_layers;
	}
	else {
		end_layer = count;
		p_layers->layer_count = count;
	}

	/* assign layers to structure */
	for (loop_index = start_layer; loop_index < end_layer; loop_index++)
	{
		status = vector_element_copy(handle->loops, loop_index, (void*)&p_loop);
		if (NO_ERROR != status) {
			return status;
		}

		/* copy data pointers to output structure */
		/* TODO: fix vector access */
		status = vector_element_copy(
			p_loop->video_addresses,
			p_loop->next_frame,
			(void*)&(p_layers->video_layers[layer_index]));
		if (NO_ERROR != status) {
			return status;
		}
		status = vector_element_copy(
			p_loop->depth_addresses,
			p_loop->next_frame,
			(void*)&(p_layers->depth_layers[layer_index]));
		if (NO_ERROR != status) {
			return status;
		}
		status = vector_element_copy(
			p_loop->cutoffs,
			p_loop->next_frame,
			(void*)&(p_layers->depth_cutoffs[layer_index]));
		if (NO_ERROR != status) {
			return status;
		}

		p_loop->next_frame++;
		if (p_loop->next_frame >= p_loop->frame_count) {
			p_loop->next_frame = 0;
		}
		layer_index++;
	}
	/* TODO: currently no live screen */

	return NO_ERROR;	
}

status_t director_capture_video(
	director_handle_t handle,
	void* data,
	timestamp_t timestamp)
{
	status_t   status   = NO_ERROR;
	frame_id_t frame_id = invalid_frame_id;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	status = frame_store_capture_video(
		handle->frame_store, 
		data, 
		timestamp,
		&frame_id);
	if (NO_ERROR != status) {
		return status;
	}

	if (invalid_frame_id != frame_id) {
		/* we have a new frame! */
		status = _director_handle_new_frame(handle, frame_id);
	}

	return status;
}

status_t director_capture_depth(
	director_handle_t handle,
	void* data,
	float cutoff,
	timestamp_t timestamp)
{
	status_t status = NO_ERROR;
	frame_id_t frame_id = invalid_frame_id;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	status = frame_store_capture_depth(
		handle->frame_store, 
		data, 
		timestamp,
		&frame_id);
	if (NO_ERROR != status) {
		return status;
	}
	status = frame_store_capture_meta(
		handle->frame_store, 
		(void*)&cutoff, 
		timestamp,
		&frame_id);
	if (NO_ERROR != status) {
		return status;
	}

	if (invalid_frame_id != frame_id) {
		/* we have a new frame! */
		status = _director_handle_new_frame(handle, frame_id);
	}

	return status;
}

status_t _loop_create(loop_t** pp_loop) {
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

	status = vector_create(128, sizeof(void*), &(p_loop->video_addresses));
	if (NO_ERROR != status) {
		_loop_release(p_loop);
		return status;
	}
	status = vector_create(128, sizeof(void*), &(p_loop->depth_addresses));
	if (NO_ERROR != status) {
		_loop_release(p_loop);
		return status;
	}
	status = vector_create(128, sizeof(frame_id_t), &(p_loop->frame_ids));
	if (NO_ERROR != status) {
		_loop_release(p_loop);
		return status;
	}
	status = vector_create(128, sizeof(float), &(p_loop->cutoffs));
	if (NO_ERROR != status) {
		_loop_release(p_loop);
		return status;
	}
	
	*pp_loop = p_loop;
	return NO_ERROR;
}

void _loop_release(loop_t* p_loop) {
	if (NULL == p_loop) {
		return;
	}

	vector_release(p_loop->video_addresses);
	vector_release(p_loop->depth_addresses);
	vector_release(p_loop->frame_ids);
	vector_release(p_loop->cutoffs);
	free(p_loop);
}

status_t _director_handle_new_frame(
	director_t* p_director, 
	frame_id_t frame_id) 
{
	status_t    status     = NO_ERROR;
	void*       depth      = NULL;
	void*       video      = NULL;
	float       *p_cutoff  = NULL;
	loop_t*     p_loop     = NULL;
	timestamp_t timestamp;

	p_loop = p_director->p_current_loop;

	status = frame_store_video_frame(
		p_director->frame_store,
		frame_id,
		&video,
		&timestamp);
	if (NO_ERROR != status) {
		return status;
	}
	status = frame_store_depth_frame(
		p_director->frame_store,
		frame_id,
		&depth,
		&timestamp);
	if (NO_ERROR != status) {
		return status;
	}
	status = frame_store_meta_frame(
		p_director->frame_store,
		frame_id,
		(void*)&p_cutoff,
		&timestamp);
	if (NO_ERROR != status) {
		return status;
	}

	status = vector_append(p_loop->video_addresses, &video);
	if (NO_ERROR != status) {
		return status;
	}

	status = vector_append(p_loop->depth_addresses, &depth);
	if (NO_ERROR != status) {
		return status;
	}

	status = vector_append(p_loop->cutoffs, (void*)p_cutoff);
	if (NO_ERROR != status) {
		return status;
	}

	status = vector_append(p_loop->frame_ids, (void*)&frame_id);
	if (NO_ERROR != status) {
		return status;
	}

	p_loop->frame_count += 1;
	if (p_loop->frame_count > 100) {
		status = _director_handle_new_loop(p_director, p_loop);
        p_director->p_current_loop = NULL;
        status = _loop_create(&(p_director->p_current_loop));
	}
	
	return status;
}

status_t _director_handle_new_loop(director_t* p_director, loop_t* p_loop) {
	status_t status = NO_ERROR;
	/* TODO: do something with the loop (post processing) */
	if (p_loop->frame_count > 0) {
		status = vector_append(p_director->loops, (void*)&p_loop);
		if (NO_ERROR != status) {
			return status;
		}
	}
	else {
		_loop_release(p_loop);
	}
	return NO_ERROR;
}

