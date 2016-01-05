#include "frame_store.h"
#include "vector.h"
#include "memory_pool.h"
#include "log.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FREEREC_DEFAULT_CLIP_CAPACITY (128)
#define FREEREC_DEFAULT_FRAME_CAPACITY (128)
#define FREEREC_LOG_TRACE (0)

#if 0
typedef struct clip_s {
	vector_handle_t frames;
} clip_t;

typedef struct clip_s* clip_handle_t;

typedef struct frame_store_s {
	size_t video_bytes;
	size_t depth_bytes;
	vector_handle_t clips;
	vector_handle_t frames;
	size_t video_offset;
	size_t depth_offset;
	size_t timestamp_offset;
	char* current_frame;
	size_t frame_size;
	memory_pool_handle_t memory_pool;
	clip_handle_t clip;
} frame_store_t;

static status_t _frame_store_capture_data(
	frame_store_handle_t handle,
	size_t frame_data_offset,
	size_t data_size,
	void* data,
	timestamp_t timestamp);

static status_t _frame_store_clip_frame(
	frame_store_handle_t handle,
	size_t clip_index,
	size_t frame_index,
	void** p_frame);

static status_t _frame_store_clip(
	frame_store_handle_t handle, 
	size_t index, 
	clip_handle_t* p_handle);

static status_t _clip_create(clip_handle_t* p_handle);
static void _clip_release(clip_handle_t clip, memory_pool_handle_t pool);

static status_t _frame_store_new_frame(frame_store_handle_t handle);
static status_t _frame_store_clear_current_frame(frame_store_handle_t handle);

status_t frame_store_create(
	size_t video_bytes,
	size_t depth_bytes,
	size_t max_bytes,
	frame_store_handle_t* p_handle)
{
	status_t err = NO_ERROR;
	frame_store_t* p_frame_store = NULL;

	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}

	p_frame_store = (frame_store_t*)malloc(sizeof(frame_store_t));
	if (NULL == p_frame_store) {
		return ERR_FAILED_ALLOC;
	}

	/* hopefuly you can just copy these over without some sort of copy
	 * function to get dynamically allocated data */
	p_frame_store->video_bytes = video_bytes;
	p_frame_store->depth_bytes = depth_bytes;
	p_frame_store->clips = NULL;
	p_frame_store->video_offset = 0;
	p_frame_store->depth_offset = video_bytes;
	p_frame_store->timestamp_offset = video_bytes + depth_bytes;
	p_frame_store->frame_size = video_bytes + depth_bytes + sizeof(timestamp_t);
	p_frame_store->current_frame = NULL;
	p_frame_store->memory_pool = NULL;
	p_frame_store->clip = NULL;

	/* TODO: seem to have some inconsitencies in dealing with clips vector.
	 * Clips vector makes it's own copy of the data.  But clip_create mallocs a
	 * new one.  This malloc is leaked.  Then erroneously, the clip is double
	 * freed and read after freeing.
	 */
	/*
	err = vector_create(
		FREEREC_DEFAULT_CLIP_CAPACITY, 
		sizeof(clip_handle_t),
		&(p_frame_store->clips));
	if (NO_ERROR != err) {
		frame_store_release(p_frame_store);
		return err;
	}
	*/

	err = memory_pool_create(
		p_frame_store->frame_size,
		64,
		128,
		max_bytes,
		100000,
		&(p_frame_store->memory_pool));
	if (NO_ERROR != err) {
		frame_store_release(p_frame_store);
		return err;
	}

	/* create current frame holder */
	err = _frame_store_new_frame(p_frame_store);
	if (NO_ERROR != err) {
		frame_store_release(p_frame_store);
		return err;
	}

	*p_handle = p_frame_store;
	return NO_ERROR;
}

void frame_store_release(frame_store_handle_t handle) {
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
			status = vector_element_address(handle->clips, index, (void**)&p_clip);
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

status_t frame_store_mark_clip_boundary(frame_store_handle_t handle) {
	status_t err = NO_ERROR;
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

	err = _frame_store_clear_current_frame(handle);
	if (NO_ERROR != err) {
		return err;
	}

	return NO_ERROR;
}

status_t frame_store_clip_count(frame_store_handle_t handle, size_t* p_count) {
	status_t status = NO_ERROR;

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

	return NO_ERROR;
}

status_t frame_store_frame_count(
	frame_store_handle_t handle, 
	size_t clip_index,
	size_t* p_count)
{
	status_t err = NO_ERROR;
	clip_handle_t clip_handle = NULL;

	if (NULL == p_count) {
		return ERR_NULL_POINTER;
	}

	err = _frame_store_clip(handle, clip_index, &clip_handle);
	if (NO_ERROR != err) {
		return err;
	}

	err = vector_count(clip_handle->frames, p_count);
	if (NO_ERROR != err) {
		return err;
	}

	return NO_ERROR;
}

status_t frame_store_video_frame(
	frame_store_handle_t handle,
	size_t clip_index,
	size_t frame_index,
	void** p_data,
	timestamp_t* p_timestamp)
{
	status_t err = NO_ERROR;
	char* frame_data = NULL;

	if ((NULL == handle) || (NULL == p_data) || (NULL == p_timestamp)) {
		return ERR_NULL_POINTER;
	}

	err = _frame_store_clip_frame(
		handle, 
		clip_index, 
		frame_index, 
		(void**)&frame_data);
	if (NO_ERROR != err) {
		return err;
	}

	*p_data = (void*)&(frame_data[handle->video_offset]);
	*p_timestamp = *((timestamp_t*)&(frame_data[handle->timestamp_offset]));

	return NO_ERROR;
}

status_t frame_store_depth_frame(
	frame_store_handle_t handle,
	size_t clip_index,
	size_t frame_index,
	void** p_data,
	timestamp_t* p_timestamp)
{
	status_t err = NO_ERROR;
	char* frame_data = NULL;

	if ((NULL == handle) || (NULL == p_data) || (NULL == p_timestamp)) {
		return ERR_NULL_POINTER;
	}

	err = _frame_store_clip_frame(
		handle, 
		clip_index, 
		frame_index, 
		(void**)&frame_data);
	if (NO_ERROR != err) {
		return err;
	}

	*p_data = (void*)&(frame_data[handle->depth_offset]);
	*p_timestamp = *((timestamp_t*)&(frame_data[handle->timestamp_offset]));

	return NO_ERROR;
}

status_t frame_store_capture_video(
	frame_store_handle_t handle,
	void* data,
	timestamp_t timestamp)
{
	status_t status = NO_ERROR;
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	status = _frame_store_capture_data(
		handle, 
		handle->video_offset, 
		handle->video_bytes,
		data, 
		timestamp);

	if (NO_ERROR != status) {
		return status;
	}
	return NO_ERROR;
}

status_t frame_store_capture_depth(
	frame_store_handle_t handle,
	void* data,
	timestamp_t timestamp)
{
	status_t status = NO_ERROR;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	status = _frame_store_capture_data(
		handle, 
		handle->depth_offset, 
		handle->depth_bytes,
		data, 
		timestamp);
	if (NO_ERROR != status) {
		return status;
	}

	return NO_ERROR;
}

status_t frame_store_remove_frame(
	frame_store_handle_t handle,
	size_t clip_index,
	size_t frame_index)
{
	status_t       status  = NO_ERROR;
	clip_handle_t* p_clip  = NULL;
	void**          p_frame = NULL;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	status = vector_element_address(handle->clips, clip_index, (void**)&p_clip);
	if (NO_ERROR != status) {
		return status;
	}
	status = vector_element_address((*p_clip)->frames, frame_index, (void**)&p_frame);
	if (NO_ERROR != status) {
		return status;
	}
	status = memory_pool_unclaim(handle->memory_pool, (void*)*p_frame);
	if (NO_ERROR != status) {
		return status;
	}
	return vector_remove((*p_clip)->frames, frame_index);
}

status_t _frame_store_capture_data(
	frame_store_handle_t handle,
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
		err = _frame_store_new_frame(handle);
		if (NO_ERROR != err) {
			return err;
		}
		memcpy(
			&(handle->current_frame[handle->timestamp_offset]),
			&timestamp,
			sizeof(timestamp_t));
	}
	else {
		memset(handle->current_frame, 0, handle->frame_size);
		return ERR_INVALID_TIMESTAMP;
	}
	return NO_ERROR;
}

status_t _frame_store_clip_frame(
	frame_store_handle_t handle,
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

	err = _frame_store_clip(handle, clip_index, &clip);
	if (NO_ERROR != err) {
		return err;
	}
	
	err = vector_element_address(clip->frames, frame_index, (void**)&p_frame_address);
	if (NO_ERROR != err) {
		return err;
	}
	
	if (NULL == p_frame_address) {
		return ERR_NULL_POINTER;
	}
	
	*p_frame = *p_frame_address;

	return NO_ERROR;
}

status_t _frame_store_clip(
	frame_store_handle_t handle, 
	size_t index, 
	clip_handle_t* p_handle)
{
	status_t err = NO_ERROR;
	clip_handle_t* p_vector_handle = NULL;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	err = vector_element_address(handle->clips, index, (void**)&p_vector_handle);
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

status_t _frame_store_new_frame(frame_store_handle_t handle) {
	status_t err = NO_ERROR;
	err = memory_pool_claim(
		handle->memory_pool, 
		(void**)&(handle->current_frame));
	if (NO_ERROR != err) {
		return err;
	}
	return _frame_store_clear_current_frame(handle);
}

status_t _frame_store_clear_current_frame(frame_store_handle_t handle) {
	memset(handle->current_frame, 0, handle->frame_size);
	/* set timestamp to negative value to make sure it doesn't get confused
	 * with a timestamp of zero */
	timestamp_t* p_timestamp = 
		((timestamp_t*)&(handle->current_frame[handle->timestamp_offset]));
	*p_timestamp = (timestamp_t)-1;
	return NO_ERROR;
}
#endif


const frame_id_t invalid_frame_id = (frame_id_t)-1;

typedef struct frame_store_s {
	size_t video_bytes;
	size_t depth_bytes;
	size_t meta_bytes;
	vector_handle_t frames;
	size_t video_offset;
	size_t depth_offset;
	size_t meta_offset;
	size_t timestamp_offset;
	unsigned char* current_frame;
	size_t frame_size;
	size_t current_frame_stored_size;
	size_t frame_count;
	memory_pool_handle_t memory_pool;
} frame_store_t;

static status_t _frame_store_capture_data(
	frame_store_handle_t handle,
	size_t frame_data_offset,
	size_t data_size,
	void* data,
	timestamp_t timestamp,
	frame_id_t* p_frame_id);


static status_t _frame_store_new_frame(frame_store_handle_t handle);
static status_t _frame_store_clear_current_frame(frame_store_handle_t handle);
static status_t _frame_store_sub_frame(
	frame_store_handle_t handle,
	frame_id_t frame_id,
	size_t data_offset,
	void** p_data,
	timestamp_t* p_timestamp);

status_t frame_store_create(
	size_t video_bytes,
	size_t depth_bytes,
	size_t meta_bytes,
	size_t max_bytes,
	frame_store_handle_t* p_handle)
{
	status_t err = NO_ERROR;
	frame_store_t* p_frame_store = NULL;

	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}

	p_frame_store = (frame_store_t*)malloc(sizeof(frame_store_t));
	if (NULL == p_frame_store) {
		return ERR_FAILED_ALLOC;
	}
	memset(p_frame_store, 0, sizeof(frame_store_t));

	/* hopefuly you can just copy these over without some sort of copy
	 * function to get dynamically allocated data */
	p_frame_store->frame_count = 0;
	p_frame_store->video_bytes = video_bytes;
	p_frame_store->depth_bytes = depth_bytes;
	p_frame_store->depth_offset = video_bytes;
	p_frame_store->meta_bytes = meta_bytes;
	p_frame_store->meta_offset = video_bytes + depth_bytes;
	p_frame_store->timestamp_offset = video_bytes + depth_bytes + meta_bytes;
	p_frame_store->frame_size = 
		video_bytes + depth_bytes + meta_bytes + sizeof(timestamp_t);

	err = memory_pool_create(
		p_frame_store->frame_size,
		64,
		128,
		max_bytes,
		100000,
		&(p_frame_store->memory_pool));
	if (NO_ERROR != err) {
		frame_store_release(p_frame_store);
		return err;
	}

	err = vector_create(32, sizeof(unsigned char*), &p_frame_store->frames);
	if (NO_ERROR != err) {
		frame_store_release(p_frame_store);
		return err;
	}

	/* create current frame holder */
	err = _frame_store_new_frame(p_frame_store);
	if (NO_ERROR != err) {
		frame_store_release(p_frame_store);
		return err;
	}

	*p_handle = p_frame_store;
	return NO_ERROR;
}

void frame_store_release(frame_store_handle_t handle) {

	if (NULL == handle) {
		return; 
	}

	/* TODO: I don't think we need to do this because we're releasing the memory
	 * pool which should free all the memory it allocated.
	status = vector_count(handle->frames, &count);
	if (NO_ERROR == status) {
		for (index = 0; index < count; index++) {
			status = vector_element_address(handle->frames, index, (void**)&frame);
			if (NO_ERROR == status) {
				status = memory_pool_unclaim(handle->memory_pool, frame);
			}
		}
	}
	*/

	vector_release(handle->frames);
	memory_pool_release(handle->memory_pool);
	free(handle);
}


status_t frame_store_frame_count(frame_store_handle_t handle, size_t* p_count) {
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	*p_count = handle->frame_count;

	return NO_ERROR;
}

status_t _frame_store_sub_frame(
	frame_store_handle_t handle,
	frame_id_t frame_id,
	size_t data_offset,
	void** p_data,
	timestamp_t* p_timestamp)
{
	status_t err = NO_ERROR;
	unsigned char** p_frame_data = NULL;
	unsigned char* frame_data = NULL;

	if ((NULL == handle) || (NULL == p_data) || (NULL == p_timestamp)) {
		return ERR_NULL_POINTER;
	}

	err = vector_element_address(handle->frames, frame_id, (void**)&p_frame_data);
	if (NO_ERROR != err) {
		return err;
	}
	frame_data = *p_frame_data;

	*p_data = (void*)&(frame_data[data_offset]);
	*p_timestamp = *((timestamp_t*)&(frame_data[handle->timestamp_offset]));

	return NO_ERROR;
}

status_t frame_store_video_frame(
	frame_store_handle_t handle,
	frame_id_t frame_id,
	void** p_data,
	timestamp_t* p_timestamp)
{
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	return _frame_store_sub_frame(
		handle, 
		frame_id, 
		handle->video_offset, 
		p_data, 
		p_timestamp);
}

status_t frame_store_depth_frame(
	frame_store_handle_t handle,
	frame_id_t frame_id,
	void** p_data,
	timestamp_t* p_timestamp)
{
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	return _frame_store_sub_frame(
		handle, 
		frame_id, 
		handle->depth_offset, 
		p_data, 
		p_timestamp);
}

status_t frame_store_meta_frame(
	frame_store_handle_t handle,
	frame_id_t frame_id,
	void** p_data,
	timestamp_t* p_timestamp)
{
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	return _frame_store_sub_frame(
		handle, 
		frame_id, 
		handle->meta_offset, 
		p_data, 
		p_timestamp);
}

status_t frame_store_capture_video(
	frame_store_handle_t handle,
	void* data,
	timestamp_t timestamp,
	frame_id_t* p_frame_id)
{
	status_t status = NO_ERROR;
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	status = _frame_store_capture_data(
		handle, 
		handle->video_offset, 
		handle->video_bytes,
		data, 
		timestamp,
		p_frame_id);

	if (NO_ERROR != status) {
		return status;
	}
	return NO_ERROR;
}

status_t frame_store_capture_depth(
	frame_store_handle_t handle,
	void* data,
	timestamp_t timestamp,
	frame_id_t* p_frame_id)
{
	status_t status = NO_ERROR;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	status = _frame_store_capture_data(
		handle, 
		handle->depth_offset, 
		handle->depth_bytes,
		data, 
		timestamp,
		p_frame_id);
	if (NO_ERROR != status) {
		return status;
	}

	return NO_ERROR;
}

status_t frame_store_capture_meta(
	frame_store_handle_t handle,
	void* data,
	timestamp_t timestamp,
	frame_id_t* p_frame_id)
{
	status_t status = NO_ERROR;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	status = _frame_store_capture_data(
		handle, 
		handle->meta_offset, 
		handle->meta_bytes,
		data, 
		timestamp,
		p_frame_id);
	if (NO_ERROR != status) {
		return status;
	}

	return NO_ERROR;
}

status_t frame_store_remove_frame(
	frame_store_handle_t handle,
	frame_id_t frame_id)
{
	status_t       status  = NO_ERROR;
	void**         p_frame = NULL;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	status = vector_element_address(handle->frames, frame_id, (void**)&p_frame);
	if (NO_ERROR != status) {
		return status;
	}
	handle->frame_count--;

	status = memory_pool_unclaim(handle->memory_pool, (void*)*p_frame);
	if (NO_ERROR != status) {
		return status;
	}
	*p_frame = NULL;
	/* TODO: double check, used to release this here, but now that we're using 
	 * frame_ids, removing an element would screw up the ID space.
		vector_remove((*p_clip)->frames, frame_index);
	*/
	return NO_ERROR;
}

status_t _frame_store_capture_data(
	frame_store_handle_t handle,
	size_t frame_data_offset,
	size_t data_size,
	void* data,
	timestamp_t timestamp,
	frame_id_t* p_frame_id)
{
	timestamp_t current_timestamp = 0;
	status_t    err               = NO_ERROR;

	if ((NULL == handle) || (NULL == data) || (NULL == p_frame_id)) {
		return ERR_NULL_POINTER;
	}

	*p_frame_id = invalid_frame_id;
	current_timestamp = 
		*((timestamp_t*)&(handle->current_frame[handle->timestamp_offset]));
	//if (timestamp > current_timestamp) {
    if (timestamp != current_timestamp) {
		/* copy data and wait for other half */
		memcpy(
			&(handle->current_frame[handle->timestamp_offset]),
			&timestamp,
			sizeof(timestamp_t));
		memcpy(
			&(handle->current_frame[frame_data_offset]),
			data,
			data_size);
		handle->current_frame_stored_size = data_size + sizeof(timestamp_t);
	}
	else if (timestamp == current_timestamp) {
		/* copy data and store */
		memcpy(
			&(handle->current_frame[frame_data_offset]),
			data,
			data_size);
		handle->current_frame_stored_size += data_size;

		if (handle->current_frame_stored_size == handle->frame_size) {
			/* we've collected all the necessary data, so time to
			 * store the frame */
            err = vector_count(handle->frames, p_frame_id);
			if (NO_ERROR != err) {
				return err;
			}
			err = vector_append(
				handle->frames, 
				(void*)&(handle->current_frame));
			if (NO_ERROR != err) {
				return err;
			}
			handle->frame_count++;
			err = _frame_store_new_frame(handle);
			if (NO_ERROR != err) {
				return err;
			}
			memcpy(
				&(handle->current_frame[handle->timestamp_offset]),
				&timestamp,
				sizeof(timestamp_t));
		}
	}
    /*
	else {
		memset(handle->current_frame, 0, handle->frame_size);
		return ERR_INVALID_TIMESTAMP;
	}
    */
	return NO_ERROR;
}

status_t _frame_store_new_frame(frame_store_handle_t handle) {
	status_t err = NO_ERROR;
	err = memory_pool_claim(
		handle->memory_pool, 
		(void**)&(handle->current_frame));
	if (NO_ERROR != err) {
		return err;
	}
	return _frame_store_clear_current_frame(handle);
}

status_t _frame_store_clear_current_frame(frame_store_handle_t handle) {
	memset(handle->current_frame, 0, handle->frame_size);
	/* set timestamp to negative value to make sure it doesn't get confused
	 * with a timestamp of zero */
	timestamp_t* p_timestamp = 
		((timestamp_t*)&(handle->current_frame[handle->timestamp_offset]));
	*p_timestamp = (timestamp_t)-1;
	return NO_ERROR;
}

