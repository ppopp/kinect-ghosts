#include "freerec.h"
#include "vector.h"
#include "memory_pool.h"

#include <stdlib.h>
#include <string.h>

#define FREEREC_DEFAULT_CLIP_CAPACITY (128)

typedef struct clip_s {
	vector_handle_t frames;
} clip_t;

typedef struct clip_s* clip_handle_t;

typedef struct freerec_s {
	freenect_frame_mode video_mode;
	freenect_frame_mode depth_mode;
	vector_handle_t clips;
	size_t video_offset;
	size_t depth_offset;
	size_t timestamp_offset;
	char* current_frame;
	size_t frame_size;
	memory_pool_handle_t memory_pool;
	clip_handle_t clip;
} freerec_t;

static status_t _freerec_capture_data(
	freerec_handle_t handle,
	size_t frame_data_offset,
	size_t data_size,
	void* data,
	uint32_t timestamp);
static status_t _freerec_clip_frame(
	freerec_handle_t handle,
	size_t clip_index,
	size_t frame_index,
	void** p_frame);
static status_t _freerec_clip(
	freerec_handle_t handle, 
	size_t index, 
	clip_handle_t* p_handle);
static status_t _clip_create(clip_handle_t* p_handle);
static void _clip_release(clip_handle_t clip, memory_pool_handle_t pool);

status_t freerec_create(
	freenect_frame_mode* p_video_mode,
	freenect_frame_mode* p_depth_mode,
	size_t max_bytes,
	freerec_handle_t* p_handle)
{
	status_t err = NO_ERROR;
	freerec_t* p_freerec = NULL;

	if ((NULL == p_handle) || 
		(NULL == p_video_mode) || 
		(NULL == p_depth_mode))
	{
		return ERR_NULL_POINTER;
	}

	p_freerec = (freerec_t*)malloc(sizeof(freerec_t));
	if (NULL == p_freerec) {
		return ERR_FAILED_ALLOC;
	}

	/* hopefuly you can just copy these over without some sort of copy
	 * function to get dynamically allocated data */
	p_freerec->video_mode = *p_video_mode;
	p_freerec->depth_mode = *p_depth_mode;
	p_freerec->clips = NULL;
	p_freerec->video_offset = 0;
	p_freerec->depth_offset = p_video_mode->bytes;
	p_freerec->timestamp_offset = p_video_mode->bytes + p_depth_mode->bytes;
	p_freerec->frame_size = 
		p_video_mode->bytes + 
		p_depth_mode->bytes + 
		sizeof(uint32_t);
	p_freerec->current_frame = NULL;
	p_freerec->memory_pool = NULL;
	p_freerec->clip = NULL;

	err = vector_create(
		FREEREC_DEFAULT_CLIP_CAPACITY, 
		sizeof(clip_t),
		&(p_freerec->clips));
	if (NO_ERROR != err) {
		freerec_release(p_freerec);
		return err;
	}

	err = memory_pool_create(
		p_freerec->frame_size,
		64,
		128,
		max_bytes,
		100000,
		&(p_freerec->memory_pool));
	if (NO_ERROR != err) {
		freerec_release(p_freerec);
		return err;
	}

	/* create current frame holder */
	err = memory_pool_claim(
		p_freerec->memory_pool,
		(void**)&(p_freerec->current_frame));
	if (NO_ERROR != err) {
		freerec_release(p_freerec);
		return err;
	}
	memset(p_freerec->current_frame, 0, p_freerec->frame_size);

	/* starts a new clip */
	err = freerec_action(p_freerec);
	if (NO_ERROR != err) {
		freerec_release(p_freerec);
		return err;
	}

	*p_handle = p_freerec;
	return NO_ERROR;
}

void freerec_release(freerec_handle_t handle) {
	if (NULL == handle) {
		return; 
	}

	_clip_release(handle->clip, handle->memory_pool);
	vector_release(handle->clips);
	memory_pool_release(handle->memory_pool);
	free(handle);
}

status_t freerec_action(freerec_handle_t handle) {
	status_t err = NO_ERROR;
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	err = _clip_create(&(handle->clip));
	if (NO_ERROR != err) {
		return err;
	}

	err = vector_append(handle->clips, (void*)handle->clip);
	if (NO_ERROR != err) {
		return err;
	}

	return NO_ERROR;
}

status_t freerec_clip_count(freerec_handle_t handle, size_t* p_count) {
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	return vector_count(handle->clips, p_count);
}

status_t freerec_clip_frame_count(
	freerec_handle_t handle, 
	size_t clip_index,
	size_t* p_count)
{
	status_t err = NO_ERROR;
	clip_handle_t clip_handle = NULL;	

	if (NULL == p_count) {
		return ERR_NULL_POINTER;
	}

	err = _freerec_clip(handle, clip_index, &clip_handle);
	if (NO_ERROR != err) {
		return err;
	}

	err = vector_count(clip_handle->frames, p_count);
	if (NO_ERROR != err) {
		return err;
	}

	return NO_ERROR;
}

status_t freerec_clip_video_frame(
	freerec_handle_t handle,
	size_t clip_index,
	size_t frame_index,
	void** p_data,
	uint32_t* p_timestamp)
{
	status_t err = NO_ERROR;
	char* frame_data = NULL;

	if ((NULL == handle) || (NULL == p_data) || (NULL == p_timestamp)) {
		return ERR_NULL_POINTER;
	}

	err = _freerec_clip_frame(
		handle, 
		clip_index, 
		frame_index, 
		(void*)&frame_data);
	if (NO_ERROR != err) {
		return err;
	}

	*p_data = (void*)&(frame_data[handle->video_offset]);
	*p_timestamp = *((uint32_t*)&(frame_data[handle->timestamp_offset]));

	return NO_ERROR;
}

status_t freerec_clip_depth_frame(
	freerec_handle_t handle,
	size_t clip_index,
	size_t frame_index,
	void** p_data,
	uint32_t* p_timestamp)
{
	status_t err = NO_ERROR;
	char* frame_data = NULL;

	if ((NULL == handle) || (NULL == p_data) || (NULL == p_timestamp)) {
		return ERR_NULL_POINTER;
	}

	err = _freerec_clip_frame(
		handle, 
		clip_index, 
		frame_index, 
		(void*)&frame_data);
	if (NO_ERROR != err) {
		return err;
	}

	*p_data = (void*)&(frame_data[handle->depth_offset]);
	*p_timestamp = *((uint32_t*)&(frame_data[handle->timestamp_offset]));

	return NO_ERROR;
}

status_t freerec_capture_video(
	freerec_handle_t handle,
	void* data,
	uint32_t timestamp)
{
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	return _freerec_capture_data(
		handle, 
		handle->video_offset, 
		handle->video_mode.bytes,
		data, 
		timestamp);
}

status_t freerec_capture_depth(
	freerec_handle_t handle,
	void* data,
	uint32_t timestamp)
{
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	return _freerec_capture_data(
		handle, 
		handle->depth_offset, 
		handle->depth_mode.bytes,
		data, 
		timestamp);
}

status_t _freerec_capture_data(
	freerec_handle_t handle,
	size_t frame_data_offset,
	size_t data_size,
	void* data,
	uint32_t timestamp)
{
	uint32_t current_timestamp = 0;
	status_t err = NO_ERROR;
	if ((NULL == handle) || (NULL == data)) {
		return ERR_NULL_POINTER;
	}

	current_timestamp = 
		*((uint32_t*)&(handle->current_frame[handle->timestamp_offset]));
	if (timestamp > current_timestamp) {
		/* copy data and wait for other half */
		memcpy(
			&(handle->current_frame[handle->timestamp_offset]),
			&timestamp,
			sizeof(uint32_t));
		memcpy(
			&(handle->current_frame[frame_data_offset]),
			data,
			data_size);
	}
	else if (timestamp == current_timestamp) {
		err = vector_append(handle->clip->frames, handle->current_frame);
		if (NO_ERROR != err) {
			return err;
		}
		err = memory_pool_claim(
			handle->memory_pool, 
			(void**)&(handle->current_frame));
		if (NO_ERROR != err) {
			return err;
		}
	}
	else {
		return ERR_INVALID_TIMESTAMP;
	}
	return NO_ERROR;
}

status_t _freerec_clip_frame(
	freerec_handle_t handle,
	size_t clip_index,
	size_t frame_index,
	void** p_frame)
{
	status_t err = NO_ERROR;
	clip_handle_t clip = NULL;
	if ((NULL == handle) || (NULL == p_frame)) {
		return ERR_NULL_POINTER;
	}

	err = _freerec_clip(handle, clip_index, &clip);
	if (NO_ERROR != err) {
		return err;
	}

	err = vector_element(clip->frames, frame_index, p_frame);
	if (NO_ERROR != err) {
		return err;
	}

	return NO_ERROR;
}

status_t _freerec_clip(
	freerec_handle_t handle, 
	size_t index, 
	clip_handle_t* p_handle);
status_t _clip_create(clip_handle_t* p_handle);
void _clip_release(clip_handle_t clip, memory_pool_handle_t pool);
