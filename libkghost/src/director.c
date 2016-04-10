#include "director.h"
#include "frame_store.h"
#include "vector.h"
#include "motion_detector.h"
#include "log.h"
#include "loop.h"

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* TODO: add way to remove a loop */
/* TODO: loops could be more musical.  simple loop logic should do */



/* director sturcture */
typedef struct director_s {
	size_t max_layers;
	size_t max_bytes;
	size_t bytes_per_video_frame;
	size_t bytes_per_depth_frame;
	size_t bytes_per_depth_pixel;
	float depth_scale;

	double valid_frame_min_presence;
	double valid_frame_min_motion;
	unsigned valid_frame_patience;
	unsigned invalid_frame_count;
	unsigned loop_min_frame_count;
	bool_t is_recording;

	frame_store_handle_t frame_store;
	vector_handle_t loops;
	loop_t* p_current_loop;
	loop_t** playing_loops;
	motion_detector_handle_t motion_detector;
	pthread_mutex_t loops_mutex;
	pthread_mutex_t frame_store_mutex;
} director_t;

static status_t _director_handle_new_frame(director_t* p_director, frame_id_t frame_id);
static status_t _director_handle_new_loop(director_t* p_director, loop_t* p_loop);

/* thread data used to handle new loops */
typedef struct thread_data_s {
	pthread_t thread;
	director_t* director;
	loop_t* loop;
} thread_data_t;

static status_t _thread_data_create(
	director_t* p_director,
	loop_t* p_loop,
	thread_data_t** pp_td);
static void _thread_data_release(thread_data_t* p_td);
static void* _handle_new_loop(void* data);

status_t director_create(
	size_t max_layers,
	size_t max_bytes, 
	size_t bytes_per_video_frame,
	size_t bytes_per_depth_frame,
	size_t bytes_per_depth_pixel,
	float depth_scale,
	director_handle_t* p_handle)
{
	int pthreadErr = 0;
	status_t status = NO_ERROR;
	director_t* p_director = NULL;

	/* check inputs */
	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}

	if (max_layers > DIRECTOR_MAX_LAYERS) {
		return ERR_INVALID_ARGUMENT;
	}

	/* allocate / initialize structure */
	p_director = malloc(sizeof(director_t));
	if (NULL == p_director) {
		return ERR_FAILED_ALLOC;
	}
	memset(p_director, 0, sizeof(director_t));

	p_director->max_layers = max_layers;
	p_director->max_bytes = max_bytes;
	p_director->bytes_per_video_frame = bytes_per_video_frame;
	p_director->bytes_per_depth_frame = bytes_per_depth_frame;
	p_director->bytes_per_depth_pixel = bytes_per_depth_pixel;
	p_director->depth_scale = depth_scale;
	p_director->valid_frame_min_presence = 0.05;
	p_director->valid_frame_min_motion = 0.01;
	p_director->valid_frame_patience = 5;
	p_director->invalid_frame_count = 0;
	p_director->loop_min_frame_count = 10;
	p_director->is_recording = FALSE;

	/* create internal storage */
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

	p_director->playing_loops = (loop_t**)malloc(sizeof(loop_t*) * max_layers);
	if (NULL == p_director->playing_loops) {
		director_release(p_director);
		return ERR_FAILED_ALLOC;
	}
	memset(p_director->playing_loops, 0, sizeof(loop_t*) * max_layers);

	status = loop_create(p_director->frame_store, &(p_director->p_current_loop));
	if (NO_ERROR != status) {
		director_release(p_director);
		return status;
	}

	/* create motion detector to trigger start of recording */
	status = motion_detector_create(
		bytes_per_depth_pixel,
		bytes_per_depth_frame / bytes_per_depth_pixel,
		TRUE,
		&(p_director->motion_detector));
	if (NO_ERROR != status) {
		director_release(p_director);
		return status;
	}

	/* create mutex for handling new loops */
	pthreadErr = pthread_mutex_init(&(p_director->loops_mutex), NULL);
	if (pthreadErr) {
		director_release(p_director);
		return ERR_FAILED_THREAD_CREATE;
	}

	srand(time(NULL));
	*p_handle = p_director;

	return NO_ERROR;
}

void director_release(director_handle_t handle) {
	/* TODO: some possible troubles with release code in case
	 * a thread is still running.  we could hold on to references
	 * of the threads, or have a counter w/ a mutex.
	 */
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
			loop_release(p_loop);
		}
	}

	frame_store_release(handle->frame_store);
	vector_release(handle->loops);
	loop_release(handle->p_current_loop);
	pthread_mutex_destroy(&(handle->loops_mutex));
	pthread_mutex_destroy(&(handle->frame_store_mutex));
	motion_detector_release(handle->motion_detector);

	free(handle);
}

status_t director_playback_layers(
	director_handle_t handle, 
	timestamp_t delta, 
	director_frame_layers_t* p_layers)
{
	status_t    status        = NO_ERROR;
	size_t      count         = 0;
	size_t      layer_index   = 0;
	size_t      loop_index    = 0;
	loop_t      *p_loop       = NULL;
	timestamp_t timestamp     = 0;

	if ((NULL == handle) || (NULL == p_layers)) {
		return ERR_NULL_POINTER;
	}

	p_layers->layer_count = 0;
	/* get number of existing loops */
	pthread_mutex_lock(&(handle->loops_mutex));
	status = vector_count(handle->loops, &count);
	if (NO_ERROR != status) {
		pthread_mutex_unlock(&(handle->loops_mutex));
		return status;
	}

	if (count < 1) {
		/* nothing to play */
		pthread_mutex_unlock(&(handle->loops_mutex));
		return NO_ERROR;
	}


	/* fill any empty loops */
	for (layer_index = 0; layer_index < handle->max_layers; layer_index++) {
		/* check if loop is free */
		if (NULL == handle->playing_loops[layer_index]) {
			/* choose random loop */
			loop_index = rand() % count;
			/* copy internal loop to playing loops */
			status = vector_element_copy(
				handle->loops, 
				loop_index, 
				(void*)&(handle->playing_loops[layer_index]));
			if (NO_ERROR != status) {
				pthread_mutex_unlock(&(handle->loops_mutex));
				return status;
			}
			/* prepare loop for playback */
			p_loop = handle->playing_loops[layer_index];
			p_loop->next_frame = 0;
			if (p_loop->frame_count > 1) {
				p_loop->till_next_frame = 0;
			}
			else {
				/* get difference between two frame timestamps */
				status = loop_frame_timestamp_delta(
						p_loop,
						0,
						1,
						&(p_loop->till_next_frame));
				if (NO_ERROR != status) {
					return status;
				}
			}
		}
	}


	/* assign layers to structure */
	for (layer_index = 0; layer_index < handle->max_layers; layer_index++) {
		p_loop = handle->playing_loops[layer_index];

		/* copy data pointers to output structure */
		status = vector_element_copy(
			p_loop->video_addresses,
			p_loop->next_frame,
			(void*)&(p_layers->video_layers[layer_index]));
		if (NO_ERROR != status) {
			pthread_mutex_unlock(&(handle->loops_mutex));
			return status;
		}
		status = vector_element_copy(
			p_loop->depth_addresses,
			p_loop->next_frame,
			(void*)&(p_layers->depth_layers[layer_index]));
		if (NO_ERROR != status) {
			pthread_mutex_unlock(&(handle->loops_mutex));
			return status;
		}
		status = vector_element_copy(
			p_loop->cutoffs,
			p_loop->next_frame,
			(void*)&(p_layers->depth_cutoffs[layer_index]));
		if (NO_ERROR != status) {
			pthread_mutex_unlock(&(handle->loops_mutex));
			return status;
		}

		/* increment frame in loop */
		if (delta > p_loop->till_next_frame) {
			/* get time till next frame */
			timestamp = delta - p_loop->till_next_frame;
			while (timestamp > 0) {
				/* increment frame */
				p_loop->next_frame++;
				if (p_loop->next_frame >= p_loop->frame_count) {
					/* finish loop */
					p_loop->next_frame = 0;
					handle->playing_loops[layer_index] = NULL;
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
	}
	p_layers->layer_count = handle->max_layers;
	/* TODO: currently no live screen */

	pthread_mutex_unlock(&(handle->loops_mutex));
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

	pthread_mutex_lock(&(handle->frame_store_mutex));
	status = frame_store_capture_video(
		handle->frame_store, 
		data, 
		timestamp,
		&frame_id);
	pthread_mutex_unlock(&(handle->frame_store_mutex));
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

	pthread_mutex_lock(&(handle->frame_store_mutex));
	status = frame_store_capture_depth(
		handle->frame_store, 
		data, 
		timestamp,
		&frame_id);
	if (NO_ERROR != status) {
		pthread_mutex_unlock(&(handle->frame_store_mutex));
		return status;
	}
	status = frame_store_capture_meta(
		handle->frame_store, 
		(void*)&cutoff, 
		timestamp,
		&frame_id);
	pthread_mutex_unlock(&(handle->frame_store_mutex));
	if (NO_ERROR != status) {
		return status;
	}

	if (invalid_frame_id != frame_id) {
		/* we have a new frame! */
		status = _director_handle_new_frame(handle, frame_id);
	}

	return status;
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
	unsigned short motion_cutoff = 0;
	double motion = 0.0;
	double presence = 0.0;
	bool_t valid_frame = FALSE;
	bool_t loop_ended = FALSE;
	timestamp_t timestamp;

	p_loop = p_director->p_current_loop;

	/* retrieve frame from frame store */
	pthread_mutex_lock(&(p_director->frame_store_mutex));
	status = frame_store_video_frame(
		p_director->frame_store,
		frame_id,
		&video,
		&timestamp);
	if (NO_ERROR != status) {
		pthread_mutex_unlock(&(p_director->frame_store_mutex));
		return status;
	}
	status = frame_store_depth_frame(
		p_director->frame_store,
		frame_id,
		&depth,
		&timestamp);
	if (NO_ERROR != status) {
		pthread_mutex_unlock(&(p_director->frame_store_mutex));
		return status;
	}
	status = frame_store_meta_frame(
		p_director->frame_store,
		frame_id,
		(void*)&p_cutoff,
		&timestamp);
	if (NO_ERROR != status) {
		pthread_mutex_unlock(&(p_director->frame_store_mutex));
		return status;
	}
	pthread_mutex_unlock(&(p_director->frame_store_mutex));

	/* run through motion detector */
	motion_cutoff = (short)((unsigned)(*p_cutoff * 65536) / p_director->depth_scale);
	status = motion_detector_detect(
		p_director->motion_detector,
		depth,
		motion_cutoff,
		&motion,
		&presence);

	/* determine if frame is valid */
	if (motion >= p_director->valid_frame_min_motion) {
		if (presence >= p_director->valid_frame_min_presence) {
			valid_frame = TRUE;
			p_director->invalid_frame_count = 0;
            p_director->is_recording = TRUE;
		}
	}

	/* determine whether this marks end of loop */
	if (FALSE == valid_frame) {
		if (TRUE == p_director->is_recording) {
			p_director->invalid_frame_count++;
			if (p_director->invalid_frame_count > p_director->valid_frame_patience) {
				p_director->is_recording = FALSE;
				loop_ended = TRUE;
			}
		}
	}
	
	if (p_director->is_recording) {
		/* append pointrs if recording */
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

		status = vector_append(p_loop->timestamps, (void*)&timestamp);
		if (NO_ERROR != status) {
			return status;
		}

		status = vector_append(p_loop->frame_ids, (void*)&frame_id);
		if (NO_ERROR != status) {
			return status;
		}

		p_loop->frame_count += 1;
	}
	else {
		/* release frame id because it will not be used */
		pthread_mutex_lock(&(p_director->frame_store_mutex));
		status = frame_store_remove_frame(p_director->frame_store, frame_id);
		pthread_mutex_unlock(&(p_director->frame_store_mutex));
	}

	if (TRUE == loop_ended) {
		if (p_loop->frame_count >= p_director->loop_min_frame_count) {
			status = _director_handle_new_loop(p_director, p_loop);
			if (NO_ERROR != status) {
				return status;
			}
		}
		else {
			loop_release(p_loop);	
		}
		p_director->p_current_loop = NULL;
		status = loop_create(p_director->frame_store, &(p_director->p_current_loop));
	}
	
	return status;
}

status_t _director_handle_new_loop(director_t* p_director, loop_t* p_loop) {
	status_t status = NO_ERROR;
	thread_data_t* p_thread_data = NULL;
	int pthread_error = 0;

	status = _thread_data_create(p_director, p_loop, &p_thread_data);
	if (NO_ERROR != status) {
		loop_release(p_loop);
		return status;
	}

	pthread_error = pthread_create(
		&(p_thread_data->thread),
		NULL,
		&_handle_new_loop,
		p_thread_data);
	
	if (0 != pthread_error) {
		loop_release(p_loop);
		return ERR_FAILED_THREAD_CREATE;
	}

	return NO_ERROR;
}

status_t _thread_data_create(
	director_t* p_director,
	loop_t* p_loop,
	thread_data_t** pp_td)
{
	thread_data_t* p_thread_data = NULL;
	if (NULL == pp_td) {
		return ERR_NULL_POINTER;
	}

	p_thread_data = malloc(sizeof(thread_data_t));
	if (NULL == p_thread_data) {
		return ERR_FAILED_ALLOC;
	}
	memset(p_thread_data, 0, sizeof(thread_data_t));

	p_thread_data->thread = 0;
	p_thread_data->director = p_director;
	p_thread_data->loop = p_loop;

	*pp_td = p_thread_data;

	return NO_ERROR;
}

void _thread_data_release(thread_data_t* p_td) {
	if (NULL != p_td) {
		free(p_td);
	}
}


void* _handle_new_loop(void* data) {
	status_t status = NO_ERROR;
	thread_data_t* p_td = (thread_data_t*)data;
	//motion_detector_handle_t motion_detector = NULL;
	size_t pixel_count = 0;
	//size_t frame_count = 0;
	//size_t i = 0;
	director_t* director = NULL;

	if (NULL == p_td) {
		LOG_ERROR("null thread data")
		pthread_exit(NULL);
	}

	director = p_td->director;
	pixel_count = director->bytes_per_depth_frame / director->bytes_per_depth_pixel;

	/* create motion detector 
	status = motion_detector_create(
		p_td->director->bytes_per_depth_pixel,
		pixel_count,
		TRUE,
		&motion_detector);
	if (NO_ERROR != status) {
		LOG_ERROR("failed to create motion detector");
		return NULL;
	}

		*/

	/* TODO: trim frames at beginning and end w/o enough presence or motion */
	/* TODO: remove old loops if too many loops exist */
	/*
	for (i = 0; i < frame_count; i++) {
		status_t motion_detector_detect(
			motion_detector_handle_t handle,
			void* depth, 
			int cutoff, 
			double* p_motion, 
			double* p_presence);
	}
	*/

	pthread_mutex_lock(&(director->loops_mutex));
	status = vector_append(director->loops, (void*)&(p_td->loop));
	pthread_mutex_unlock(&(director->loops_mutex));

	if (NO_ERROR != status) {
		loop_release(p_td->loop);
		LOG_ERROR("failed to append loop to loops");
		_thread_data_release(p_td);
		pthread_exit(NULL);
	}
	_thread_data_release(p_td);
	pthread_exit(NULL);
}

