#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <libfreenect/libfreenect.h>

#include "kinect_manager.h"
#include "common.h"
#include "timer.h"
#include "log.h"

static const freenect_loglevel _freenect_log_level = FREENECT_LOG_WARNING;
static const double _min_camera_angle = -35.0;
static const double _max_camera_angle = 35.0;


typedef struct _kinect_manager_s {
	double camera_angle;
	freenect_context *fnctx;
	freenect_device *fndevice;
	freenect_frame_mode video_mode;
	freenect_frame_mode depth_mode;
	void* video_buffer;
	void* depth_buffer;
	float depth_scale;
	struct timeval timeout;
	timer_handle_t record_timer;
	
	double record_timestamp;
	kinect_callbacks_t callbacks;
	void* user_data;
} kinect_manager_t;

static bool_t _streq(const char* str1, const char* str2);
static freenect_resolution _translate_resolution(kinect_manager_resolution_t res);

static int _init_freenect(
	kinect_manager_t* p_knctmgr, 
	freenect_resolution resolution);

static void _freenect_log_callback(
	freenect_context *dev, 
	freenect_loglevel level, 
	const char *msg);

static void _video_cb(freenect_device *dev, void *video, uint32_t timestamp);
static void _depth_cb(freenect_device *dev, void *depth, uint32_t timestamp);

status_t kinect_manager_create(
	kinect_manager_handle_t* p_handle,
	kinect_manager_resolution_t resolution,
	kinect_callbacks_t* p_callbacks,
	void* user_data)
{
	/* create manager, find and connect to the freenect device, 
	   initialize callbacks and start getting data from the device. 
	 */
	kinect_manager_t* p_knctmgr = NULL;
	status_t error = NO_ERROR;
	int freenect_error = 0;

	if (NULL == p_handle) {
		LOG_ERROR("null pointer")
		return ERR_NULL_POINTER;
	}

	/* allocate and initialize everything to zero */
	p_knctmgr = (kinect_manager_t*)malloc(sizeof(kinect_manager_t));
	if (NULL == p_knctmgr) {
		LOG_ERROR("failed alloc")
		return ERR_FAILED_ALLOC;
	}
	memset(p_knctmgr, 0, sizeof(kinect_manager_t));


	/* initialize innards of struct */
	p_knctmgr->depth_scale = 0.0f;
	p_knctmgr->camera_angle = 0.0;
	p_knctmgr->timeout.tv_sec = 0;
	p_knctmgr->timeout.tv_usec = 1000;
	p_knctmgr->user_data = user_data;
	if (NULL != p_callbacks) {
		memcpy(
			(void*)&p_knctmgr->callbacks, 
			(const void*)p_callbacks, 
			sizeof(kinect_callbacks_t));
	}

	error = timer_create(&(p_knctmgr->record_timer));
	if (0 != error) {
		kinect_manager_destroy(p_knctmgr);
		LOG_ERROR("failed to create record timer");
		return error;
	}

	/* initialize freenect */
	freenect_error = _init_freenect(
		p_knctmgr, 
		_translate_resolution(resolution));
	if (freenect_error) {
		kinect_manager_destroy(p_knctmgr);
		LOG_ERROR("failed to initialize freenect");
		return ERR_FAILED_CREATE;
	}

	*p_handle = p_knctmgr;
	return NO_ERROR;
}

void kinect_manager_destroy(kinect_manager_handle_t handle) {
	if (NULL == handle) {
		return;
	}

	timer_release(handle->record_timer);
	if (handle->fndevice) {
		freenect_stop_video(handle->fndevice);
		freenect_stop_depth(handle->fndevice);
		freenect_close_device(handle->fndevice);
	}
	if (handle->fnctx) {
		freenect_shutdown(handle->fnctx);
	}
	free(handle->video_buffer);
	free(handle->depth_buffer);
	free(handle);
}

status_t kinect_manager_info(
	kinect_manager_handle_t handle,
	const char* field,
	void* p_data,
	size_t data_size) 
{
	if ((NULL == handle) || (NULL == field) || (NULL == p_data)) {
		return ERR_NULL_POINTER;
	}

	if (_streq(field, CMI_DEPTH_BPP_SIZE_T)) {
		size_t* p_bpp = (size_t*)p_data;
		if (data_size != sizeof(size_t)) {
			return ERR_INVALID_ARGUMENT;
		}
		*p_bpp = handle->depth_mode.data_bits_per_pixel 
			+ handle->depth_mode.padding_bits_per_pixel;
	}
	else if (_streq(field, CMI_DEPTH_WIDTH_SIZE_T)) {
		size_t* p_width = (size_t*)p_data;
		if (data_size != sizeof(size_t)) {
			return ERR_INVALID_ARGUMENT;
		}
		*p_width = handle->depth_mode.width;
	}
	else if (_streq(field, CMI_DEPTH_HEIGHT_SIZE_T)) {
		size_t* p_height = (size_t*)p_data;
		if (data_size != sizeof(size_t)) {
			return ERR_INVALID_ARGUMENT;
		}
		*p_height = handle->depth_mode.height;
	}
	else if (_streq(field, CMI_VIDEO_BPP_SIZE_T)) {
		size_t* p_bpp = (size_t*)p_data;
		if (data_size != sizeof(size_t)) {
			return ERR_INVALID_ARGUMENT;
		}
		*p_bpp = handle->video_mode.data_bits_per_pixel 
			+ handle->video_mode.padding_bits_per_pixel;
	}
	else if (_streq(field, CMI_VIDEO_WIDTH_SIZE_T)) {
		size_t* p_width = (size_t*)p_data;
		if (data_size != sizeof(size_t)) {
			return ERR_INVALID_ARGUMENT;
		}
		*p_width = handle->video_mode.width;
	}
	else if (_streq(field, CMI_VIDEO_HEIGHT_SIZE_T)) {
		size_t* p_height = (size_t*)p_data;
		if (data_size != sizeof(size_t)) {
			return ERR_INVALID_ARGUMENT;
		}
		*p_height = handle->video_mode.height;
	}
	else {
		LOG_ERROR("invalid field \"%s\"", field);
		return ERR_INVALID_ARGUMENT;
	}

	return NO_ERROR;
}

status_t kinect_manager_live_video(
	kinect_manager_handle_t handle,
	void** pp_data) 
{
	if ((NULL == handle) || (NULL == pp_data)) {
		return ERR_NULL_POINTER;
	}
	*pp_data = handle->video_buffer;
	return NO_ERROR;
}

status_t kinect_manager_live_depth(
	kinect_manager_handle_t handle,
	void** pp_data)
{
	if ((NULL == handle) || (NULL == pp_data)) {
		return ERR_NULL_POINTER;
	}
	*pp_data = handle->depth_buffer;
	return NO_ERROR;
}

status_t kinect_manager_capture_frame(
	kinect_manager_handle_t handle,
	bool_t* p_captured) 
{
	int result = 0;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	
	/* this forces the video and depth callbacks to happen */
	//freenect_process_events(p_gl_ghosts->fnctx);
	result = freenect_process_events_timeout(
		handle->fnctx, 
		&(handle->timeout));
	if (p_captured) {
		*p_captured = (result == 0);
	}
	if (result != 0) {
		LOG_DEBUG("failed to capturing frame from kinect. code %i", result);
	}

	return NO_ERROR;
}

status_t kinect_manager_reset_timestamp(kinect_manager_handle_t handle) {
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	return timer_reset(handle->record_timer);
}

status_t kinect_manager_set_camera_angle(
	kinect_manager_handle_t handle,
	double angle) 
{
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	if ((angle < _min_camera_angle) || (angle > _max_camera_angle)) {
		return ERR_INVALID_ARGUMENT;
	}
	if (freenect_set_tilt_degs(handle->fndevice, angle) != 0) {
		return ERR_DEVICE_ERROR;
	}
	handle->camera_angle = angle;
	return NO_ERROR;
}

status_t kinect_manager_get_camera_angle(
	kinect_manager_handle_t handle,
	double* p_angle)
{
	if ((NULL == handle) || (NULL == p_angle)) {
		return ERR_NULL_POINTER;
	}
	
	*p_angle = handle->camera_angle;
	return NO_ERROR;
}

int _init_freenect(kinect_manager_t* p_knctmgr, freenect_resolution resolution) {
	int error = 0;
	int num_devices = 0;
	int bit_count = 0;

	/* initialize freenect context and logging */
	error = freenect_init(&(p_knctmgr->fnctx), NULL);
	if (0 != error) {
		LOG_ERROR("failed to initialize freenect. code %i", error);
		return ERR_DEVICE_ERROR;
	}
	freenect_set_log_callback(p_knctmgr->fnctx, _freenect_log_callback);
	freenect_set_log_level(p_knctmgr->fnctx, _freenect_log_level);

	/* query and open device */
	num_devices = freenect_num_devices(p_knctmgr->fnctx);
	if (num_devices < 1) {
		LOG_WARNING("no kinect devices found");
		return -1;
	}
	/* assume only one device is connected.  open the first device */
	error = freenect_open_device(
		p_knctmgr->fnctx, 
		&(p_knctmgr->fndevice), 
		0);
	if (error) {
		LOG_ERROR("failed to connect to freenect device");
		return error;
	}

	/* set user data for device so we can access this struct in video and
	 * depth callbacks */
	freenect_set_user(p_knctmgr->fndevice, (void*)p_knctmgr);
	
	/* setup video mode and buffers */
	p_knctmgr->video_mode = freenect_find_video_mode(
		resolution, 
		FREENECT_VIDEO_RGB);
	if (!(p_knctmgr->video_mode.is_valid)) {
		LOG_ERROR("invalid video mode");
		return -1;
	}
	error = freenect_set_video_mode(p_knctmgr->fndevice, p_knctmgr->video_mode);
	if (error < 0) {
		LOG_ERROR("failed to set video mode");
		return error;
	}
	p_knctmgr->video_buffer = malloc(p_knctmgr->video_mode.bytes);
	if (p_knctmgr->video_buffer == NULL) {
		LOG_ERROR("failed to allocate video buffer");
		return -1;
	}
	error = freenect_set_video_buffer(
		p_knctmgr->fndevice, 
		p_knctmgr->video_buffer);
	if (error < 0) {
		LOG_ERROR("failed to set video buffer");
		return error;
	}
	freenect_set_video_callback(p_knctmgr->fndevice, _video_cb);


	/* setup detph mode and buffers */
	/*
	p_knctmgr->depth_mode = freenect_find_depth_mode(
		resolution, 
		FREENECT_DEPTH_11BIT);
		*/
	p_knctmgr->depth_mode = freenect_find_depth_mode(
		resolution, 
		FREENECT_DEPTH_REGISTERED);
	if (!(p_knctmgr->depth_mode.is_valid)) {
		LOG_ERROR("invalid depth mode");
		return -1;
	}
	error = freenect_set_depth_mode(p_knctmgr->fndevice, p_knctmgr->depth_mode);
	if (error < 0) {
		LOG_ERROR("failed to set depth mode");
		return error;
	}
	p_knctmgr->depth_buffer = malloc(p_knctmgr->depth_mode.bytes);
	if (p_knctmgr->depth_buffer == NULL) {
		LOG_ERROR("failed to allocate depth buffer");
		return -1;
	}
	error = freenect_set_depth_buffer(
		p_knctmgr->fndevice, 
		p_knctmgr->depth_buffer);
	if (error < 0) {
		LOG_ERROR("failed to set depth buffer");
		return error;
	}
	freenect_set_depth_callback(p_knctmgr->fndevice, _depth_cb);
	/* set scaling for depth data */
	bit_count = p_knctmgr->depth_mode.data_bits_per_pixel 
		+ p_knctmgr->depth_mode.padding_bits_per_pixel;
	p_knctmgr->depth_scale = (float)pow(2, bit_count) 
		/ (float)FREENECT_DEPTH_MM_MAX_VALUE;

	/* perform ready callbacks */
	if (p_knctmgr->callbacks.video_ready_callback) {
		p_knctmgr->callbacks.video_ready_callback(
			p_knctmgr,
			p_knctmgr->user_data);
	}
	if (p_knctmgr->callbacks.depth_ready_callback) {
		p_knctmgr->callbacks.depth_ready_callback(
			p_knctmgr,
			p_knctmgr->user_data);
	}

	/* start collecting from device */
	error = freenect_start_video(p_knctmgr->fndevice);
	if (error < 0) {
		LOG_ERROR("failed to start video");
		return error;
	}
	error = freenect_start_depth(p_knctmgr->fndevice);
	if (error < 0) {
		LOG_ERROR("failed to start depth");
	}

	return error;
}

void _freenect_log_callback(
	freenect_context *dev, 
	freenect_loglevel level, 
	const char *msg) 
{
	/* route freenect specific log callbacks into global logging system */
	/* TODO: remove the trailing newline from message */
	switch (level) {
		case FREENECT_LOG_FATAL:
		case FREENECT_LOG_ERROR:
			LOG_ERROR("freenect error - %s", msg);
			break;
		case FREENECT_LOG_WARNING:
		case FREENECT_LOG_NOTICE:
			LOG_WARNING("freenect warning - %s", msg);
			break;
		case FREENECT_LOG_INFO:
			LOG_INFO("freenect info - %s", msg);
			break;
		case FREENECT_LOG_DEBUG:
		case FREENECT_LOG_SPEW:
		case FREENECT_LOG_FLOOD:
			LOG_DEBUG("freenect debug - %s", msg);
			break;
		default:
			LOG_ERROR("freenect error - %s", msg);
			LOG_WARNING("unhandled freenect log level");
	}
}

/* freenect callback for video data */
void _video_cb(freenect_device *dev, void *video, uint32_t timestamp) {
	/* If in record mode, store video and timestamp */
	kinect_manager_t* p_knctmgr = NULL;
	int error = 0;
	
	p_knctmgr = (kinect_manager_t*)freenect_get_user(dev);
	if (NULL == p_knctmgr) {
		LOG_ERROR("null pointer");
		return;
	}

	error = timer_current(
		p_knctmgr->record_timer, 
		&(p_knctmgr->record_timestamp));
	if (0 != error) {
		LOG_ERROR("error getting record timestamp");
		return;
	}

	if (p_knctmgr->callbacks.video_frame_callback) {
		p_knctmgr->callbacks.video_frame_callback(
			p_knctmgr,
			video,
			p_knctmgr->record_timestamp,
			p_knctmgr->user_data);
	}
}

/* freenect callback for depth data */
void _depth_cb(freenect_device *dev, void *depth, uint32_t timestamp) {
	/* If in record mode, store depth.  Timestamp handled in video callback */
	/* TODO: does video callback happen b4 depth? */
	kinect_manager_t* p_knctmgr = NULL;
	
	p_knctmgr = (kinect_manager_t*)freenect_get_user(dev);
	if (NULL == p_knctmgr) {
		LOG_ERROR("null pointer");
		return;
	}
	if (p_knctmgr->callbacks.depth_frame_callback) {
		p_knctmgr->callbacks.depth_frame_callback(
			p_knctmgr,
			depth,
			p_knctmgr->record_timestamp,
			p_knctmgr->user_data);
	}
}

bool_t _streq(const char* str1, const char* str2) {
	return strcmp(str1, str2) == 0;
}

freenect_resolution _translate_resolution(kinect_manager_resolution_t res) {
	switch (res) {
		case kinect_manager_resolution_320x240:
			return FREENECT_RESOLUTION_LOW;
		case kinect_manager_resolution_640x480:
			return FREENECT_RESOLUTION_MEDIUM;
		case kinect_manager_resolution_1280x1024:
			return FREENECT_RESOLUTION_HIGH;
	}
	LOG_WARNING("unhandled kinect resolution.  defaulting to 1280x1024");
	return FREENECT_RESOLUTION_HIGH;
}

