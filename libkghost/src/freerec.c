#include "freerec.h"
#include "vector.h"
#include "memory_pool.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FREEREC_DEFAULT_CLIP_CAPACITY (128)
#define FREEREC_DEFAULT_FRAME_CAPACITY (128)
#define FREEREC_LOG_TRACE (0)

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
	timestamp_t timestamp);
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
static status_t _freerec_new_frame(freerec_handle_t handle);
static status_t _freerec_clear_current_frame(freerec_handle_t handle);

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
		sizeof(timestamp_t);
	p_freerec->current_frame = NULL;
	p_freerec->memory_pool = NULL;
	p_freerec->clip = NULL;

	/* TODO: seem to have some inconsitencies in dealing with clips vector.
	 * Clips vector makes it's own copy of the data.  But clip_create mallocs a
	 * new one.  This malloc is leaked.  Then erroneously, the clip is double
	 * freed and read after freeing.
	 */
	err = vector_create(
		FREEREC_DEFAULT_CLIP_CAPACITY, 
		sizeof(clip_handle_t),
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
	err = _freerec_new_frame(p_freerec);
	if (NO_ERROR != err) {
		freerec_release(p_freerec);
		return err;
	}

	*p_handle = p_freerec;
	return NO_ERROR;
}

void freerec_release(freerec_handle_t handle) {
	status_t status = NO_ERROR;
	clip_handle_t* p_clip = NULL;
	size_t index = 0;
	size_t count = 0;

	if (NULL == handle) {
		return; 
	}

	status = vector_count(handle->clips, &count);
	if (NO_ERROR == status) {
		for (index = 0; index < count; index++) {
			status = vector_element(handle->clips, index, (void**)&p_clip);
			if (NO_ERROR == status) {
				_clip_release(*p_clip, handle->memory_pool);
			}
		}
	}

	//_clip_release(handle->clip, handle->memory_pool);
	vector_release(handle->clips);
	memory_pool_release(handle->memory_pool);
	free(handle);
}

status_t freerec_action(freerec_handle_t handle) {
	status_t err = NO_ERROR;
#if FREEREC_LOG_TRACE
	LOG_DEBUG("enter");
#endif
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	err = _clip_create(&(handle->clip));
	if (NO_ERROR != err) {
		return err;
	}

	err = vector_append(handle->clips, (void*)&(handle->clip));
	if (NO_ERROR != err) {
		return err;
	}

	err = _freerec_clear_current_frame(handle);
	if (NO_ERROR != err) {
		return err;
	}

#if FREEREC_LOG_TRACE
	LOG_DEBUG("exit");
#endif
	return NO_ERROR;
}

status_t freerec_clip_count(freerec_handle_t handle, size_t* p_count) {
	status_t status = NO_ERROR;

#if FREEREC_LOG_TRACE
	LOG_DEBUG("enter");
#endif
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	status = vector_count(handle->clips, p_count);
	if (NO_ERROR != status) {
		return status;
	}

	/* We want to return one less than what exists in the clip vector because
	 * the current clip is still recording */
	if (*p_count > 0) {
		(*p_count)--;
	}

#if FREEREC_LOG_TRACE
	LOG_DEBUG("exit");
#endif
	return NO_ERROR;
}

status_t freerec_clip_frame_count(
	freerec_handle_t handle, 
	size_t clip_index,
	size_t* p_count)
{
	status_t err = NO_ERROR;
	clip_handle_t clip_handle = NULL;
#if FREEREC_LOG_TRACE
	LOG_DEBUG("enter");
#endif

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

#if FREEREC_LOG_TRACE
	LOG_DEBUG("exit");
#endif
	return NO_ERROR;
}

status_t freerec_clip_video_frame(
	freerec_handle_t handle,
	size_t clip_index,
	size_t frame_index,
	void** p_data,
	timestamp_t* p_timestamp)
{
	status_t err = NO_ERROR;
	char* frame_data = NULL;
#if FREEREC_LOG_TRACE
	LOG_DEBUG("enter");
#endif

	if ((NULL == handle) || (NULL == p_data) || (NULL == p_timestamp)) {
		return ERR_NULL_POINTER;
	}

	err = _freerec_clip_frame(
		handle, 
		clip_index, 
		frame_index, 
		(void**)&frame_data);
	if (NO_ERROR != err) {
		return err;
	}

	*p_data = (void*)&(frame_data[handle->video_offset]);
	*p_timestamp = *((timestamp_t*)&(frame_data[handle->timestamp_offset]));

#if FREEREC_LOG_TRACE
	LOG_DEBUG("enter");
#endif
	return NO_ERROR;
}

status_t freerec_clip_depth_frame(
	freerec_handle_t handle,
	size_t clip_index,
	size_t frame_index,
	void** p_data,
	timestamp_t* p_timestamp)
{
	status_t err = NO_ERROR;
	char* frame_data = NULL;
#if FREEREC_LOG_TRACE
	LOG_DEBUG("enter");
#endif

	if ((NULL == handle) || (NULL == p_data) || (NULL == p_timestamp)) {
		return ERR_NULL_POINTER;
	}

	err = _freerec_clip_frame(
		handle, 
		clip_index, 
		frame_index, 
		(void**)&frame_data);
	if (NO_ERROR != err) {
		return err;
	}

	*p_data = (void*)&(frame_data[handle->depth_offset]);
	*p_timestamp = *((timestamp_t*)&(frame_data[handle->timestamp_offset]));

#if FREEREC_LOG_TRACE
	LOG_DEBUG("exit");
#endif
	return NO_ERROR;
}

status_t freerec_capture_video(
	freerec_handle_t handle,
	void* data,
	timestamp_t timestamp)
{
	status_t status = NO_ERROR;
#if FREEREC_LOG_TRACE
	LOG_DEBUG("enter");
#endif
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	status = _freerec_capture_data(
		handle, 
		handle->video_offset, 
		handle->video_mode.bytes,
		data, 
		timestamp);

	if (NO_ERROR != status) {
		return status;
	}
#if FREEREC_LOG_TRACE
	LOG_DEBUG("exit");
#endif
	return NO_ERROR;
}

status_t freerec_capture_depth(
	freerec_handle_t handle,
	void* data,
	timestamp_t timestamp)
{
	status_t status = NO_ERROR;
#if FREEREC_LOG_TRACE
	LOG_DEBUG("enter");
#endif

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	status = _freerec_capture_data(
		handle, 
		handle->depth_offset, 
		handle->depth_mode.bytes,
		data, 
		timestamp);
	if (NO_ERROR != status) {
		return status;
	}

#if FREEREC_LOG_TRACE
	LOG_DEBUG("exit");
#endif
	return NO_ERROR;
}

status_t _freerec_capture_data(
	freerec_handle_t handle,
	size_t frame_data_offset,
	size_t data_size,
	void* data,
	timestamp_t timestamp)
{
	timestamp_t current_timestamp = 0;
	status_t    err               = NO_ERROR;

	if ((NULL == handle) || (NULL == data)) {
		return ERR_NULL_POINTER;
	}

	current_timestamp = 
		*((timestamp_t*)&(handle->current_frame[handle->timestamp_offset]));
	if (timestamp > current_timestamp) {
		/* copy data and wait for other half */
		memcpy(
			&(handle->current_frame[handle->timestamp_offset]),
			&timestamp,
			sizeof(timestamp_t));
		memcpy(
			&(handle->current_frame[frame_data_offset]),
			data,
			data_size);
	}
	else if (timestamp == current_timestamp) {
		memcpy(
			&(handle->current_frame[frame_data_offset]),
			data,
			data_size);
		err = vector_append(
			handle->clip->frames, 
			(void*)&(handle->current_frame));
		if (NO_ERROR != err) {
			return err;
		}
		err = _freerec_new_frame(handle);
		if (NO_ERROR != err) {
			return err;
		}
		memcpy(
			&(handle->current_frame[handle->timestamp_offset]),
			&timestamp,
			sizeof(timestamp_t));
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
	void** p_frame_address = NULL;
	
	if ((NULL == handle) || (NULL == p_frame)) {
		return ERR_NULL_POINTER;
	}

	err = _freerec_clip(handle, clip_index, &clip);
	if (NO_ERROR != err) {
		return err;
	}
	
	err = vector_element(clip->frames, frame_index, (void**)&p_frame_address);
	if (NO_ERROR != err) {
		return err;
	}
	
	if (NULL == p_frame_address) {
		return ERR_NULL_POINTER;
	}
	
	*p_frame = *p_frame_address;

	return NO_ERROR;
}

status_t _freerec_clip(
	freerec_handle_t handle, 
	size_t index, 
	clip_handle_t* p_handle)
{
	status_t err = NO_ERROR;
	clip_handle_t* p_vector_handle = NULL;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	err = vector_element(handle->clips, index, (void**)&p_vector_handle);
	if (NO_ERROR == err) {
		*p_handle = *p_vector_handle;
	}
	return err;
}

status_t _clip_create(clip_handle_t* p_handle) {
	status_t err = NO_ERROR;
	clip_t* p_clip = NULL;

	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}

	p_clip = (clip_t*)malloc(sizeof(clip_t));
	if (NULL == p_clip) {
		return ERR_FAILED_ALLOC;
	}

	p_clip->frames = NULL;
	err = vector_create(
		FREEREC_DEFAULT_FRAME_CAPACITY,
		sizeof(char*),
		&(p_clip->frames));
	if (NO_ERROR != err) {
		_clip_release(p_clip, NULL);
		return err;
	}

	*p_handle = p_clip;
	return NO_ERROR;
}

void _clip_release(clip_handle_t clip, memory_pool_handle_t pool) {
	status_t err = NO_ERROR;
	size_t frame_count = 0;
	size_t index = 0;
	char** p_first_frame = NULL;

	if (NULL == clip) {
		return;
	}

	err = vector_count(clip->frames, &frame_count);
	if (NO_ERROR == err) {
		err = vector_array(clip->frames, (void**)p_first_frame);
		if (NO_ERROR == err) {
			for (index = 0; index < frame_count; index++) {
				err = memory_pool_unclaim(pool, (void*)p_first_frame[index]);
				if (NO_ERROR != err) {
					/* TODO: log error */
					break;
				}	
			}
			/* else {
				 TODO: log error 
			 } */
		}
		/* else {
			 TODO: log error 
		 } */
	}
	/* else {
		 TODO: log error 
	 } */
	vector_release(clip->frames);
	free(clip);
}

status_t _freerec_new_frame(freerec_handle_t handle) {
	status_t err = NO_ERROR;
	err = memory_pool_claim(
		handle->memory_pool, 
		(void**)&(handle->current_frame));
	if (NO_ERROR != err) {
		return err;
	}
	return _freerec_clear_current_frame(handle);
}

status_t _freerec_clear_current_frame(freerec_handle_t handle) {
	memset(handle->current_frame, 0, handle->frame_size);
	return NO_ERROR;
}

