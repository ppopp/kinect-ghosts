#include "glut_main.h"
#include "frame_store.h"
#include "timer.h"
#include "median_filter.h"
#include "motion_detector.h"

#include "log.h"
#include "command.h"
#include "common.h"

#include "kinect_manager.h"
#include "display_manager.h"

#include <GLUT/glut.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <limits.h>

/* Maximum string length of command entered through stdin */
#define MAX_COMMAND_LENGTH (512)

/* TODO: logic for automatically recording.  We have presence and motion.
   some metrics might be:
   	- maximum length
	- minimum presence
	- minimum motion
	- minimum length

	There really could be any logic for determining start and end and what plays.

	The "director" controls the recording and playback.  probably want to keep things decoupled and have 
	the director's communications with the display_manager and kinect_manager in main, but lets think
	about that.

	

	[communication between director and display manager]
	[main] show the next frame (timestamp)
		[director] tells [display_manager] to display these frames (where does the director get the frames)
		or 
		[main] asks [director] for next frames and hands them to [display_manager]
		[director] asks [display_manager] how many layers it can display.

		? this sounds more like an editor

	[communication between director and kinect_manager]
		none.  

	[communication between director and frame_store]
	[director] determines wether [frame_store] should hold clip or not
	[director] tells [frame_store] when to makr clip boundary 
		- is that necessary though? can we store the sequence of frames elsewhere and just use the frame store
		  as a data housing.  But then isn't that the memory pool? 
	[director] cleans up clips by removing frames.  

	So the main functionality is
		- Cleanup clips
		- Provide flexibility in how to playback clips (lots of cool stuff could happen here)
		- Somehow manage recording.  Could just constantly record and then discard if the clip sucks.




		


*/


#define DOWNSAMPLE (8)
static int _downsample_counter = 0;

static kinect_manager_resolution_t resolution = kinect_manager_resolution_640x480;

/* integer char values used to define end of command */
static const unsigned char _newline = 10;
static const unsigned char _carriage_return = 13;
static const unsigned char _spacebar = (unsigned char)' ';

/* collect 5 seconds of video */
static const size_t _max_recorded_bytes = 640 * 480 * 4 * 30 * 5 * 4;

typedef struct _layer_s {
	float depth;
	double time;
	double next_time;
	size_t frame;
} layer_t;

typedef struct _stream_properties_s {
	size_t width;
	size_t height;
	size_t bits_per_pixel;
	size_t bytes_per_frame;
} stream_properties_t;

typedef struct _gl_ghosts {
	/* Shader variables */
	const char* vertex_shader_path;
	const char* fragment_shader_path;

	float depth_scale;
	float depth_cutoff;
	char command[MAX_COMMAND_LENGTH];
	size_t commandPos;
	commander cmdr;

	frame_store_handle_t frame_store;
	bool_t do_record;
	layer_t layers[TEXTURE_CAPACITY - 1];
	timer_handle_t playback_timer;

	kinect_manager_handle_t kinect_manager;
	display_manager_handle_t display_manager;
	stream_properties_t video_stream_properties;
	stream_properties_t depth_stream_properties;
	motion_detector_handle_t motion_detector;


	
} gl_ghosts; 


/* screen resize callback */
static void _reshape(int width, int height, void* data);
/* keyboard callback */
static void _keyboard(
	unsigned char key, 
	int mouseX, 
	int mouseY, 
	void* userData);
/* special character keyboard callback */
static void _special(
	int key,
	int mouseX, 
	int mouseY, 
	void* userData);
/* main loop callback functions */
static void _idle(void* data);
static void _display(void* data);
static int _init(void* data);
static void _cleanup(void* data);


static void _video_cb(
	kinect_manager_handle_t kinect_manager,
	void* video_data,
	timestamp_t timestamp,
	void* user_data);
static void _depth_cb(
	kinect_manager_handle_t kinect_manager,
	void* depth_data,
	timestamp_t timestamp,
	void* user_data);

/* command callback */
static int _command_set(const char* cmd, void* data);

/* recording control */
static int _start_recording(gl_ghosts* p_gl_ghosts);
static int _stop_recording(gl_ghosts* p_gl_ghosts);

/* layer stuff */
static status_t _init_layer(
	float depth, 
	frame_store_handle_t frame_store, 
	size_t frame_store_clip_index, 
	layer_t* p_layer);

static status_t _layer_get_next_frame(
		layer_t* p_layer,
		double time_delta,
		frame_store_handle_t frame_store,
		size_t frame_store_clip_index,
		void** p_video_buffer,
		void** p_depth_buffer);

int main(int argc, const char* argv[]) {
	gl_ghosts glGhosts;
	void* userData = NULL;
	int error = 0;

	log_set_stream_all(stdout);
	glut_callbacks callbacks = {
		&_init,
		&_display,
		&_reshape,
		&_idle,
		&_keyboard,
		&_special,
		&_cleanup
	};
	glGhosts.vertex_shader_path = "glsl/vertex.vert";
	glGhosts.fragment_shader_path = "glsl/fragment.frag";

	userData = &glGhosts;
	/* run main loop */
	LOG_DEBUG("start main loop");
	error = glut_main(argc, argv, "ghosts", &callbacks, userData);
	LOG_DEBUG("end main loop");

	return 0;
}

int _init(void* data) {
	status_t   error       = NO_ERROR;
	size_t     index       = 0;
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;

	/* initialize gl_ghosts structure */
	p_gl_ghosts->depth_cutoff = 0.1;
	p_gl_ghosts->depth_scale = 1.0f;

	memset(p_gl_ghosts->command, 0, MAX_COMMAND_LENGTH);
	p_gl_ghosts->commandPos = 0;
	p_gl_ghosts->cmdr = command_create();
	command_set_callback(p_gl_ghosts->cmdr, _command_set);

	p_gl_ghosts->frame_store = NULL;
	p_gl_ghosts->playback_timer = NULL;
	p_gl_ghosts->do_record = FALSE;

	/* initialize kinect device and start callbacks */
	/* TODO: set appropriate callbacks */
	kinect_callbacks_t kinect_callbacks;
	kinect_callbacks.video_ready_callback = NULL;
	kinect_callbacks.video_frame_callback = &_video_cb;
	kinect_callbacks.depth_ready_callback = NULL;
	kinect_callbacks.depth_frame_callback = &_depth_cb;

	error = kinect_manager_create(
		&p_gl_ghosts->kinect_manager,
		resolution,
		&kinect_callbacks,
		(void*)p_gl_ghosts);
	if (error != NO_ERROR) {
		LOG_ERROR("failed create kinect manager");
		return error;
	}

	/* get info about sizes of kinect data */
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_DEPTH_SCALE_FLOAT,
		(void*)&p_gl_ghosts->depth_scale,
		sizeof(p_gl_ghosts->depth_scale));
	if (error != NO_ERROR) {
		LOG_ERROR("failed getting info from kinect manager");
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_VIDEO_BYTES_SIZE_T,
		(void*)&p_gl_ghosts->video_stream_properties.bytes_per_frame,
		sizeof(p_gl_ghosts->video_stream_properties.bytes_per_frame));
	if (error != NO_ERROR) {
		LOG_ERROR("failed getting info from kinect manager");
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_DEPTH_BYTES_SIZE_T,
		(void*)&p_gl_ghosts->depth_stream_properties.bytes_per_frame,
		sizeof(p_gl_ghosts->depth_stream_properties.bytes_per_frame));
	if (error != NO_ERROR) {
		LOG_ERROR("failed getting info from kinect manager");
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_VIDEO_BPP_SIZE_T,
		(void*)&p_gl_ghosts->video_stream_properties.bits_per_pixel,
		sizeof(p_gl_ghosts->video_stream_properties.bits_per_pixel));
	if (NO_ERROR != error) {
		LOG_ERROR("failed getting info from kinect manager");
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_VIDEO_WIDTH_SIZE_T,
		(void*)&p_gl_ghosts->video_stream_properties.width,
		sizeof(p_gl_ghosts->video_stream_properties.width));
	if (NO_ERROR != error) {
		LOG_ERROR("failed getting info from kinect manager");
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_VIDEO_HEIGHT_SIZE_T,
		(void*)&p_gl_ghosts->video_stream_properties.height,
		sizeof(p_gl_ghosts->video_stream_properties.height));
	if (NO_ERROR != error) {
		LOG_ERROR("failed getting info from kinect manager");
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_DEPTH_BPP_SIZE_T,
		(void*)&p_gl_ghosts->depth_stream_properties.bits_per_pixel,
		sizeof(p_gl_ghosts->depth_stream_properties.bits_per_pixel));
	if (NO_ERROR != error) {
		LOG_ERROR("failed getting info from kinect manager");
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_DEPTH_WIDTH_SIZE_T,
		(void*)&p_gl_ghosts->depth_stream_properties.width,
		sizeof(p_gl_ghosts->depth_stream_properties.width));
	if (NO_ERROR != error) {
		LOG_ERROR("failed getting info from kinect manager");
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_DEPTH_HEIGHT_SIZE_T,
		(void*)&p_gl_ghosts->depth_stream_properties.height,
		sizeof(p_gl_ghosts->depth_stream_properties.height));
	if (NO_ERROR != error) {
		LOG_ERROR("failed getting info from kinect manager");
		return error;
	}


	/* initialize kinect recording */
	error = frame_store_create(
		p_gl_ghosts->video_stream_properties.bytes_per_frame,
		p_gl_ghosts->depth_stream_properties.bytes_per_frame,
		_max_recorded_bytes,
		&(p_gl_ghosts->frame_store));
	if (NO_ERROR != error) {
		LOG_ERROR("failed to init frame_store");
		return error;
	}
	error = frame_store_mark_clip_boundary(p_gl_ghosts->frame_store);
	if (NO_ERROR != error) {
		LOG_ERROR("failed to init frame_store");
		return error;
	}

	/* initialize display layers */
	for (index = 0; index < (TEXTURE_CAPACITY - 1); index++) {
		p_gl_ghosts->layers[index].depth = 0.0f;
		p_gl_ghosts->layers[index].time = 0.0;
		p_gl_ghosts->layers[index].frame = 0;
	}

	error = timer_create(&(p_gl_ghosts->playback_timer));
	if (NO_ERROR != error) {
		LOG_ERROR("failed to create playback timer");
		return error;
	}

	/* initialize display manager */
	error = display_manager_create(
		p_gl_ghosts->vertex_shader_path,
		p_gl_ghosts->fragment_shader_path,
		p_gl_ghosts->video_stream_properties.bits_per_pixel,
		p_gl_ghosts->video_stream_properties.width,
		p_gl_ghosts->video_stream_properties.height,
		p_gl_ghosts->depth_stream_properties.bits_per_pixel,
		p_gl_ghosts->depth_stream_properties.width,
		p_gl_ghosts->depth_stream_properties.height,
		p_gl_ghosts->depth_scale,
		&p_gl_ghosts->display_manager);

	if (error != NO_ERROR) {
		LOG_ERROR("failed to init dispaly manager");
	}

	/* initialize motion detector */
	if ((p_gl_ghosts->depth_stream_properties.bits_per_pixel % CHAR_BIT) != 0) {
		LOG_ERROR("depth pixels not byte aligned");
		return ERR_UNSUPPORTED_FORMAT;
	}
	error = motion_detector_create(
		p_gl_ghosts->depth_stream_properties.bits_per_pixel / CHAR_BIT,
		p_gl_ghosts->depth_stream_properties.width * p_gl_ghosts->depth_stream_properties.height,
		TRUE,
		&p_gl_ghosts->motion_detector);
	if (error != NO_ERROR) {
		LOG_ERROR("failed to init motion detector");
	}

	return error;
}

void _display(void* data) {
	/* GLUT display callback.  
	 *
	 * Take all stored layers/frames and push to the screen.
	 */
	status_t   error        = NO_ERROR;
	gl_ghosts* p_gl_ghosts  = (gl_ghosts*)data;
	size_t     num_frames   = 0;
	size_t     num_clips    = 0;
	size_t     index        = 0;
	void*      video_buffer = NULL;
	void*      depth_buffer = NULL;
	double     delta        = 0.0;

	/* This was added to speed things up by skipping frames. 
	 * TODO: is there a better way to make sure that playback does not slow
	 * down? */
	_downsample_counter++;
	if ((_downsample_counter % DOWNSAMPLE) != 0) {
		return;
	}
	if (p_gl_ghosts == NULL) {
		LOG_ERROR("null pointer");
		return;
	}

	/* get time since last callback */
	error = timer_current(p_gl_ghosts->playback_timer, &delta);
	if (0 != error) {
		LOG_ERROR("error getting playback time");
		return; 
	}
	error = timer_reset(p_gl_ghosts->playback_timer);
	if (0 != error) {
		LOG_ERROR("error reseting playback time");
		return; 
	}

	error = frame_store_clip_count(p_gl_ghosts->frame_store, &num_clips);
	if (0 != error) {
		LOG_ERROR("error getting clip count");
		return; 
	}

	/* prepare graphics card to accept new image data */
	error = display_manager_prepare_frame(p_gl_ghosts->display_manager);
	if (NO_ERROR != error) {
		LOG_ERROR("error preparing frame");
		return; 
	}

	/* loop through clips adding image data to opengl */
	for (index = 0; index < num_clips; index++) {
		/* get video and depth information from frame_store */
		error = frame_store_frame_count(p_gl_ghosts->frame_store, index, &num_frames);
		if (0 != error) {
			LOG_ERROR("error getting frame count");
			num_frames = 0;
		}

		if (0 == num_frames) {
			LOG_WARNING("clip with zero frames");
		}

		if (num_frames > 0) {
			video_buffer = NULL;
			depth_buffer = NULL;
			error = _layer_get_next_frame(
				&(p_gl_ghosts->layers[index]),
				delta,
				p_gl_ghosts->frame_store,
				index,
				&video_buffer,
				&depth_buffer);
			if (0 != error) {
				LOG_ERROR("error getting frame");
				num_frames = 0;
			}
			error = display_manager_set_frame_layer(
				p_gl_ghosts->display_manager,
				p_gl_ghosts->layers[index].depth,
				video_buffer,
				depth_buffer);
			if (NO_ERROR != error) {
				LOG_ERROR("error setting frame layer");
			}
		}
	}
	/* display live data */
	error = kinect_manager_live_video(
		p_gl_ghosts->kinect_manager,
		&video_buffer);
	if (NO_ERROR != error) {
		LOG_ERROR("failed to get live video frame");
	}
	error = kinect_manager_live_depth(
		p_gl_ghosts->kinect_manager,
		&depth_buffer);
	if (NO_ERROR != error) {
		LOG_ERROR("failed to get live video frame");
	}
	error = display_manager_set_frame_layer(
		p_gl_ghosts->display_manager,
		p_gl_ghosts->depth_cutoff,
		video_buffer,
		depth_buffer);
	if (NO_ERROR != error) {
		LOG_ERROR("error setting frame layer");
	}

	error = display_manager_display_frame(p_gl_ghosts->display_manager);
	if (NO_ERROR != error) {
		LOG_ERROR("error displaying frame");
	}
}

void _reshape(int width, int height, void* data) {
	/* nothing todo here... yet */
}

void _idle(void* data) {
	/* During idle time the kinect device is queried in order to capture the 
	 * next frame.
	 */
	bool_t captured = FALSE;
	status_t status = NO_ERROR;

	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;
	if (p_gl_ghosts == NULL) {
		LOG_WARNING("null pointer");
		return;
	}
	/* this forces the video and depth callbacks to happen */
	status = kinect_manager_capture_frame(
		p_gl_ghosts->kinect_manager,
		&captured);

	if (NO_ERROR != status) {
		LOG_ERROR("error while capturing frame");
	}
	if (captured) {
		/* If a new frame was captured, we redisplay the output */
		glutPostRedisplay();
	}
	else {
		LOG_WARNING("no frame captured");
	}
}

void _keyboard(unsigned char key, int mouseX, int mouseY, void* data) {
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;
	status_t status = NO_ERROR;

	if (p_gl_ghosts == NULL) {
		LOG_WARNING("null pointer");
		return;
	}
	putc(key, stdout);
	fflush(stdout);
	if ((key == _newline) || (key == _carriage_return)) {
		int cmdResult = 0;
		if (key == _carriage_return) {
			/* force newline on systems that do carriage return */
			fprintf(stdout, "\n");
		}
		/* check if it's a specific command */
		cmdResult = command_handle(
			p_gl_ghosts->cmdr, 
			p_gl_ghosts->command, 
			p_gl_ghosts);
		if (cmdResult == COMMAND_CONTINUE) {
			LOG_WARNING(
				"unhandled command \"%s\"",
				p_gl_ghosts->command);
		}
		else if (cmdResult == COMMAND_ERROR) {
			LOG_ERROR(
				"error handling command \"%s\"",
				p_gl_ghosts->command);
		}

		p_gl_ghosts->commandPos = 0;
		memset(p_gl_ghosts->command, 0, MAX_COMMAND_LENGTH);
	}
	else if ((key == _spacebar) && (p_gl_ghosts->commandPos == 0)) {
		if (p_gl_ghosts->do_record) {
			status = _stop_recording(p_gl_ghosts);
			if (NO_ERROR == status) {
				LOG_INFO("stopping recording");
			}
		}
		else {
			status = _start_recording(p_gl_ghosts);
			if (NO_ERROR == status) {
				LOG_INFO("starting recording");
			}
		}
	}
	else {
		(p_gl_ghosts->command)[p_gl_ghosts->commandPos] = (char)key;
		p_gl_ghosts->commandPos++;
		if (p_gl_ghosts->commandPos >= MAX_COMMAND_LENGTH) {
			p_gl_ghosts->commandPos = 0;
			memset(p_gl_ghosts->command, 0, MAX_COMMAND_LENGTH);
			fprintf(stdout, "\n");
			LOG_WARNING("command exceeds maximum command length");
		}
	}
}

void _special(
	int key,
	int mouseX, 
	int mouseY, 
	void* data)
{
	status_t error = NO_ERROR;
	double camera_angle = 0.0;
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;
	if (p_gl_ghosts == NULL) {
		LOG_WARNING("null pointer");
		return;
	}

	error = kinect_manager_get_camera_angle(
		p_gl_ghosts->kinect_manager,
		&camera_angle);
	if (NO_ERROR != error) {
		LOG_ERROR("error getting camera angle");
		return;
	}

	switch (key) {
		case GLUT_KEY_UP:
			camera_angle += 2.0;
		case GLUT_KEY_DOWN:
			camera_angle -= 1.0;
			error = kinect_manager_set_camera_angle(
				p_gl_ghosts->kinect_manager,
				camera_angle);
			if (NO_ERROR != error) {
				LOG_ERROR("error setting camera angle");
			}
			break;
		case GLUT_KEY_LEFT:
			break;
		case GLUT_KEY_RIGHT:
			break;
	}
}


void _video_cb(
	kinect_manager_handle_t kinect_manager,
	void* video_data,
	timestamp_t timestamp,
	void* user_data)
{
	gl_ghosts* p_ghosts = NULL;
	status_t error = NO_ERROR;
	
	p_ghosts = (gl_ghosts*)user_data;
	if (NULL == p_ghosts) {
		LOG_ERROR("null pointer");
		return;
	}

	if (TRUE == p_ghosts->do_record) {
		error = frame_store_capture_video(p_ghosts->frame_store, video_data, timestamp);
		if (NO_ERROR != error) {
			LOG_ERROR("error capturing video frame[%i](%s)", error, error_string(error));
			LOG_ERROR("timestamp %f", timestamp);
			return;
		}
	}
}

void _depth_cb(
	kinect_manager_handle_t kinect_manager,
	void* depth_data,
	timestamp_t timestamp,
	void* user_data)
{
	gl_ghosts* p_ghosts = NULL;
	status_t error = NO_ERROR;
	double motion = 0.0;
	double presence = 0.0;
	unsigned short cutoff = 0;
	
	p_ghosts = (gl_ghosts*)user_data;
	if (NULL == p_ghosts) {
		LOG_ERROR("null pointer");
		return;
	}


	if (p_ghosts->do_record) {
		error = frame_store_capture_depth(p_ghosts->frame_store, depth_data, timestamp);
		if (NO_ERROR != error) {
			LOG_ERROR("error capturing depth frame [%i](%s)", error, error_string(error));
			LOG_ERROR("timestamp %f", timestamp);
			return;
		}
	}

	cutoff = (short)((unsigned)(p_ghosts->depth_cutoff * 65536) / p_ghosts->depth_scale);
	error = motion_detector_detect(
		p_ghosts->motion_detector,
		depth_data,
		//(unsigned)(p_ghosts->depth_cutoff * 65536) / p_ghosts->depth_scale,
		(unsigned)cutoff,
		&motion,
		&presence);

	if (NO_ERROR != error) {
		LOG_ERROR("failed to calculate motion");
		return;
	}

	fprintf(stdout, "c: %u\tp: %f\tm:%f\n", cutoff, presence, motion);
}

void _cleanup(void* data) {
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;
	if (p_gl_ghosts == NULL) {
		return;
	}
	/* clean up */
	display_manager_destroy(p_gl_ghosts->display_manager);
	kinect_manager_destroy(p_gl_ghosts->kinect_manager);
	command_destroy(p_gl_ghosts->cmdr);
	frame_store_release(p_gl_ghosts->frame_store);
	timer_release(p_gl_ghosts->playback_timer);
	p_gl_ghosts->playback_timer = NULL;

	/* TODO: lots of other cleanup */
}


int _command_set(const char* cmd, void* data) {
	const char *value = NULL;
	size_t result = 0;
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;

	if (p_gl_ghosts == NULL) {
		return COMMAND_CONTINUE;
	}

	value = command_find_target(cmd, "depth", '=', &isspace);
	if (value) {
		result = sscanf(value, "%f", &(p_gl_ghosts->depth_cutoff));
		if (result != 1) {
			return COMMAND_ERROR;
		}
		return COMMAND_HANDLE_CONTINUE;
	}


	return COMMAND_CONTINUE;
}


int _start_recording(gl_ghosts* p_gl_ghosts) {
	status_t status    = NO_ERROR;
	size_t   num_clips = 0;

	if(TRUE == p_gl_ghosts->do_record) {
		LOG_DEBUG("attempt to start recording when already recording");
	}

	/* check to make sure we can handle a new clip */
	status = frame_store_clip_count(p_gl_ghosts->frame_store, &num_clips);
	if (NO_ERROR != status) {
		LOG_ERROR("failed to get clip count");
		return status;
	}

	if (num_clips >= (TEXTURE_CAPACITY - 1)) {
		LOG_WARNING("cannot create any more clips");
		return ERR_FULL;
	}

	status = kinect_manager_reset_timestamp(p_gl_ghosts->kinect_manager);
	if (NO_ERROR != status) {
		LOG_ERROR("failed to reset record timer");
		return status;
	}

	p_gl_ghosts->do_record = TRUE;

	return NO_ERROR;
}

static int _compare_pixel_uint16(const void* p_left, const void* p_right) {
	/* TODO: going to hack this up, but really need this to be contigent on the
	 * number of bits in the depth frame's pixels */
	const unsigned short* p_char_left = (const unsigned short*)p_left;
	const unsigned short* p_char_right = (const unsigned short*)p_right;

	/* TODO: I always forget what the order is supposed to be for this... */
	if (*p_char_left < *p_char_right) {
		return -1;
	}
	if (*p_char_right < *p_char_left) {
		return 1;
	}
	return 0;
}

static void _median_filter_last(gl_ghosts* p_gl_ghosts) {
	median_filter_handle_t med_filt = NULL;
	status_t status = NO_ERROR;
	median_filter_shape_t shape;
	median_filter_input_spec_t input_spec;
	size_t clip_count = 0;
	size_t frame_count = 0;
	size_t clip_to_modify = 0;
	size_t frame = 0;

	/* set median filter shape */
	shape.x = 3;
	shape.y = 3;
	shape.z = 3;

	/* determine which clip we'll be modifying */
	status = frame_store_clip_count(p_gl_ghosts->frame_store, &clip_count);
	if (NO_ERROR != status) {
		LOG_ERROR("shit");
		return;
	}
	if (clip_count < 1) {
		LOG_ERROR("shit");
		return;
	}

	/* get number of frames in clip */
	clip_to_modify = clip_count - 1;
	status = frame_store_frame_count(
		p_gl_ghosts->frame_store,
		clip_to_modify,
		&frame_count);
	if (NO_ERROR != status) {
		LOG_ERROR("shit");
		return;
	}
	if (frame_count < 1) {
		LOG_ERROR("shit");
		return;
	}

	/* set median filter input spec */
	input_spec.element_size = 
		p_gl_ghosts->depth_stream_properties.bits_per_pixel / CHAR_BIT;
	input_spec.channel_count = 1;
	input_spec.width = p_gl_ghosts->depth_stream_properties.width;
	input_spec.height = p_gl_ghosts->depth_stream_properties.height;

	status = median_filter_create(
		input_spec,
		shape,
		&_compare_pixel_uint16,
		&med_filt);
	if (NO_ERROR != status) {
		LOG_ERROR("shit");
		return;
	}

	for (frame = 0; frame < frame_count; frame++) {
		void* data = NULL;
		timestamp_t timestamp;

		status = frame_store_depth_frame(
			p_gl_ghosts->frame_store,
			clip_to_modify,
			frame,
			&data,
			&timestamp);
		if (NO_ERROR != status) {
			LOG_ERROR("shit");
			break;
		}
		status = median_filter_append(med_filt, data);
		if (NO_ERROR != status) {
			LOG_ERROR("shit");
			break;
		}
	}
	if (NO_ERROR == status) {
		status = median_filter_apply(med_filt);
		if (NO_ERROR != status) {
			LOG_ERROR("shit");
		}
	}
	median_filter_release(med_filt);
}

int _stop_recording(gl_ghosts* p_gl_ghosts) {
	status_t status    = NO_ERROR;
	size_t   count     = 0;
	size_t   last_clip = 0;

	/* check that we were recording in the first place */
	if (FALSE == p_gl_ghosts->do_record) {
		LOG_DEBUG("attemp to stop recording when not recording");
		return NO_ERROR;
	}

	/* set flag to stop recording */
	p_gl_ghosts->do_record = FALSE;

	/* start new clip */
	status = frame_store_mark_clip_boundary(p_gl_ghosts->frame_store);
	if (NO_ERROR != status) {
		LOG_ERROR("failed to start new clip");
		return status;
	}


	/* TODO: HACK:
	 * testing out median filtering of the depth channel here.  All the stuff
	 * can be removed and should be integrated in a better place */
	//_median_filter_last(p_gl_ghosts);
	/** END TODO: END HACK */

	/* initialize new layer for display */
	status = frame_store_clip_count(p_gl_ghosts->frame_store, &count);
	if (NO_ERROR != status) {
		LOG_ERROR("failed to get clip count");
		return status;
	}
	if (count >= (TEXTURE_CAPACITY - 1)) {
		LOG_WARNING("cannot create any more clips");
		return NO_ERROR;
	}

	last_clip = count - 1;
	status = _init_layer(
		p_gl_ghosts->depth_cutoff,
		p_gl_ghosts->frame_store,
		last_clip,
		&(p_gl_ghosts->layers[last_clip]));
	if (NO_ERROR != status) {
		LOG_ERROR("failed to init layer");
		return status;
	}

	return NO_ERROR;
}

status_t _init_layer(
	float depth, 
	frame_store_handle_t frame_store, 
	size_t frame_store_clip_index, 
	layer_t* p_layer)
{
	status_t status  = NO_ERROR;
	void*    p_dummy = NULL;

	p_layer->depth = depth;
	p_layer->frame = 0;
	/* get timestamp of first frame */
	status = frame_store_video_frame(
		frame_store,
		frame_store_clip_index,
		0,
		&p_dummy,
		&(p_layer->time));

	if (NO_ERROR != status) {
		LOG_ERROR("failed to get first timestamp");
		return status;
	}
	/* get next timestamp */
	status = frame_store_video_frame(
		frame_store,
		frame_store_clip_index,
		1,
		&p_dummy,
		&(p_layer->next_time));

	if (NO_ERROR != status) {
		LOG_ERROR("failed to get next timestamp");
		return status;
	}

	return NO_ERROR;
}

status_t _layer_get_next_frame(
		layer_t* p_layer,
		double time_delta,
		frame_store_handle_t frame_store,
		size_t frame_store_clip_index,
		void** p_video_buffer,
		void** p_depth_buffer)
{
	status_t error = NO_ERROR;
	double   timestamp = 0;
	size_t   count = 0;
	size_t   next_frame = 0;
	void*    dummy = NULL;

	/* determine index of next frame */
	p_layer->time += time_delta;
	if (p_layer->time >= p_layer->next_time) {
		/* increment frame */
		p_layer->frame += 1;
		error = frame_store_frame_count(frame_store, frame_store_clip_index, &count);
		if (0 != error) {
			LOG_ERROR("error getting frames");
			return error;
		}
		/* check for rollover of clip and loop if necessary */
		if (p_layer->frame >= count) {
			p_layer->frame = 0;
			error = frame_store_video_frame(
				frame_store, 
				frame_store_clip_index,
				0,
				&dummy,
				&(p_layer->time));
			if (0 != error) {
				LOG_ERROR("error getting frames");
				return error;
			}
		}
		/* get timestamp of next frame */
		next_frame = p_layer->frame + 1;
		if (next_frame >= count) {
			next_frame = 0;
		}
		error = frame_store_video_frame(
			frame_store, 
			frame_store_clip_index,
			next_frame,
			&dummy,
			&(p_layer->next_time));
		if (0 != error) {
			LOG_ERROR("error getting frames");
			return error;
		}
	}

	error = frame_store_video_frame(
		frame_store,
		frame_store_clip_index,
		p_layer->frame,
		p_video_buffer,
		&timestamp);
	if (0 != error) {
		LOG_ERROR("error getting frames");
		return error;
	}
	error = frame_store_depth_frame(
		frame_store,
		frame_store_clip_index,
		p_layer->frame,
		p_depth_buffer,
		&timestamp);
	if (0 != error) {
		LOG_ERROR("error getting frames");
		return error;
	}

	return NO_ERROR;
}


