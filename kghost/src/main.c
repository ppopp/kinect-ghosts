#include "glut_main.h"
#include "gl_shader.h"
#include "gl_canvas.h"
#include "gl_pixel_buffer.h"
#include "freerec.h"
#include "vector.h"
#include "timer.h"

#include "log.h"
#include "command.h"
#include "common.h"

#include "kinect_manager.h"

#include <GLUT/glut.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

/* Maximum string length of command entered through stdin */
#define MAX_COMMAND_LENGTH (512)

/* Maximum number of textures available in shader.  This *must* match or be
 * less than the size of the texture arrays in glsl/fragment.frag
 */
#define TEXTURE_CAPACITY (7)

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

typedef struct _gl_ghosts {
	/* Shader variables */
	const char* vertex_shader_path;
	GLuint vertex_shader;
	const char* fragment_shader_path;
	GLuint fragment_shader;
	GLuint shader_program;

	gl_pixel_buffer gl_live_depth_buffer; 
	gl_pixel_buffer gl_live_video_buffer; 
	gl_pixel_buffer gl_video_buffers[TEXTURE_CAPACITY];
	gl_pixel_buffer gl_depth_buffers[TEXTURE_CAPACITY];
	gl_canvas canvas;

	struct {
		GLint live_video_texture;
		GLint live_depth_texture;
		GLint live_depth_cutoff;

		GLint video_textures[TEXTURE_CAPACITY];
		GLint depth_textures[TEXTURE_CAPACITY];
		GLint depth_cutoffs[TEXTURE_CAPACITY];
		GLint count;
	} uniforms;

	struct {
		GLint position;
	} attributes;


	float depth_scale;
	float depth_cutoff;
	char command[MAX_COMMAND_LENGTH];
	size_t commandPos;
	commander cmdr;

	freerec_handle_t freerec;
	bool_t do_record;
	layer_t layers[TEXTURE_CAPACITY];
	timer_handle_t playback_timer;

	kinect_manager_handle_t kinect_manager;
	
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

static status_t _init_opengl(gl_ghosts* p_gl_ghosts);

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
	freerec_handle_t freerec, 
	size_t freerec_clip_index, 
	layer_t* p_layer);

static status_t _layer_get_next_frame(
		layer_t* p_layer,
		double time_delta,
		freerec_handle_t freerec,
		size_t freerec_clip_index,
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
	size_t     video_bytes = 0;
	size_t     depth_bytes = 0;
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;

	/* initialize gl_ghosts structure */
	p_gl_ghosts->vertex_shader = 0;
	p_gl_ghosts->fragment_shader = 0;
	p_gl_ghosts->shader_program = 0;

	p_gl_ghosts->canvas = NULL;
	p_gl_ghosts->gl_live_depth_buffer = NULL;
	p_gl_ghosts->gl_live_video_buffer = NULL;
	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		p_gl_ghosts->gl_video_buffers[index] = NULL;
		p_gl_ghosts->gl_depth_buffers[index] = NULL;
		p_gl_ghosts->uniforms.video_textures[index] = 0;
		p_gl_ghosts->uniforms.depth_textures[index] = 0;
		p_gl_ghosts->uniforms.depth_cutoffs[index] = -1;
	}
	p_gl_ghosts->uniforms.count = 0;
	p_gl_ghosts->uniforms.live_video_texture = 0;
	p_gl_ghosts->uniforms.live_depth_texture = 0;
	p_gl_ghosts->uniforms.live_depth_cutoff = -1;
	p_gl_ghosts->attributes.position = 0;
	p_gl_ghosts->depth_cutoff = 0.1;
	p_gl_ghosts->depth_scale = 1.0f;



	memset(p_gl_ghosts->command, 0, MAX_COMMAND_LENGTH);
	p_gl_ghosts->commandPos = 0;
	p_gl_ghosts->cmdr = command_create();
	command_set_callback(p_gl_ghosts->cmdr, _command_set);

	p_gl_ghosts->freerec = NULL;
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
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_VIDEO_BYTES_SIZE_T,
		(void*)&video_bytes,
		sizeof(video_bytes));
	if (error != NO_ERROR) {
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_DEPTH_BYTES_SIZE_T,
		(void*)&depth_bytes,
		sizeof(depth_bytes));
	if (error != NO_ERROR) {
		return error;
	}


	/* initialize kinect recording */
	error = freerec_create(
		video_bytes,
		depth_bytes,
		_max_recorded_bytes,
		&(p_gl_ghosts->freerec));
	if (NO_ERROR != error) {
		LOG_ERROR("failed to init freerec");
		return error;
	}
	error = freerec_action(p_gl_ghosts->freerec);
	if (NO_ERROR != error) {
		LOG_ERROR("failed to init freerec");
		return error;
	}

	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		p_gl_ghosts->layers[index].depth = 0.0f;
		p_gl_ghosts->layers[index].time = 0.0;
		p_gl_ghosts->layers[index].frame = 0;
	}

	error = timer_create(&(p_gl_ghosts->playback_timer));
	if (NO_ERROR != error) {
		LOG_ERROR("failed to create playback timer");
		return error;
	}

	/* initialize opengl shaders, buffers etc */
	error = _init_opengl(p_gl_ghosts);
	if (error != NO_ERROR) {
		LOG_ERROR("failed to init opengl");
	}
	return error;
}

status_t _init_opengl(gl_ghosts* p_gl_ghosts) {
	status_t error               = NO_ERROR;
	size_t   bits_per_pixel      = 0;
	size_t   width               = 0;
	size_t   height              = 0;
	size_t   index               = 0;
	char     uniform_string[256] = {'\0'};
	/* Initialize opengl to have two pixel buffers, one for video data from
	 * the kinect, and another for the depth data.  Data types / formats here
	 * have to match those used when initializing the kinect.  The pixel 
	 * buffers offer fast streaming of data from the kinect to the graphics
	 * card */
	if (p_gl_ghosts == NULL) {
		LOG_ERROR("null pointer");
		return -1;
	}
		
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR); 
	glDisable(GL_DEPTH_TEST);

	/* get width, height & bits per pixel info about video stream */
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_VIDEO_BPP_SIZE_T,
		(void*)&bits_per_pixel,
		sizeof(bits_per_pixel));
	if (NO_ERROR != error) {
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_VIDEO_WIDTH_SIZE_T,
		(void*)&width,
		sizeof(width));
	if (NO_ERROR != error) {
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_VIDEO_HEIGHT_SIZE_T,
		(void*)&height,
		sizeof(height));
	if (NO_ERROR != error) {
		return error;
	}

	/* create video pixel streaming buffer */
	p_gl_ghosts->gl_live_video_buffer = gl_pixel_buffer_create(
		width,
		height,
		GL_TEXTURE_2D,
		GL_RGBA, 
		GL_RGB,
		GL_UNSIGNED_BYTE,
		bits_per_pixel / 8,
		GL_DYNAMIC_DRAW);
	if (NULL == p_gl_ghosts->gl_live_video_buffer) {
		LOG_ERROR("failed to create screen");
		return ERR_FAILED_CREATE;
	}
	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		p_gl_ghosts->gl_video_buffers[index] = gl_pixel_buffer_create(
			width,
			height,
			GL_TEXTURE_2D,
			GL_RGBA, 
			GL_RGB,
			GL_UNSIGNED_BYTE,
			bits_per_pixel / 8,
			GL_DYNAMIC_DRAW);
		if (NULL == p_gl_ghosts->gl_video_buffers[index]) {
			LOG_ERROR("failed to create screen");
			return -1;
		}
	}

	/* get width, height & bits per pixel info about depth stream */
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_DEPTH_BPP_SIZE_T,
		(void*)&bits_per_pixel,
		sizeof(bits_per_pixel));
	if (NO_ERROR != error) {
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_DEPTH_WIDTH_SIZE_T,
		(void*)&width,
		sizeof(width));
	if (NO_ERROR != error) {
		return error;
	}
	error = kinect_manager_info(
		p_gl_ghosts->kinect_manager,
		KMI_DEPTH_HEIGHT_SIZE_T,
		(void*)&height,
		sizeof(height));
	if (NO_ERROR != error) {
		return error;
	}
	/* create depth pixel streaming buffer */
	p_gl_ghosts->gl_live_depth_buffer = gl_pixel_buffer_create(
		width,
		height,
		GL_TEXTURE_2D,
		GL_RGBA,
		GL_RED,
		GL_UNSIGNED_SHORT,
		bits_per_pixel / 8,
		GL_DYNAMIC_DRAW);
	if (NULL == p_gl_ghosts->gl_live_depth_buffer) {
		LOG_ERROR("failed to create screen");
		return ERR_FAILED_CREATE;
	}
	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		p_gl_ghosts->gl_depth_buffers[index] = gl_pixel_buffer_create(
			width,
			height,
			GL_TEXTURE_2D,
			GL_RGBA,
			GL_RED,
			GL_UNSIGNED_SHORT,
			bits_per_pixel / 8,
			GL_DYNAMIC_DRAW);
		if (NULL == p_gl_ghosts->gl_depth_buffers[index]) {
			LOG_ERROR("failed to create screen");
			return ERR_FAILED_CREATE;
		}
	}

	/* create the canvas, which is simply a square in 3D space upon which the
	 * pixel buffers are drawn */
	p_gl_ghosts->canvas = gl_canvas_create();
	if (NULL == p_gl_ghosts->canvas) {
		LOG_ERROR("failed to create canvas");
		return ERR_FAILED_CREATE;
	}
	
	/* load customized shader programs which handle depth cutoff and masking
	 * of video data by depth data */
	p_gl_ghosts->vertex_shader = gl_shader_load(
		GL_VERTEX_SHADER,
		p_gl_ghosts->vertex_shader_path);
	if (p_gl_ghosts->vertex_shader == 0) {
		LOG_ERROR("failed to load vertex shader");
		return -1;
	}
	p_gl_ghosts->fragment_shader = gl_shader_load(
		GL_FRAGMENT_SHADER,
		p_gl_ghosts->fragment_shader_path);
	if (p_gl_ghosts->fragment_shader == 0) {
		LOG_ERROR("failed to load fragment shader");
		return -1;
	}
	p_gl_ghosts->shader_program = gl_shader_program(
		p_gl_ghosts->vertex_shader,
		p_gl_ghosts->fragment_shader);
	if (p_gl_ghosts->shader_program == 0) {
		LOG_ERROR("failed to link shader program");
		return -1;
	}

	/* get handles into shader program fro variables */
	p_gl_ghosts->uniforms.live_video_texture
		= glGetUniformLocation(p_gl_ghosts->shader_program, "live_video_texture");
	p_gl_ghosts->uniforms.live_depth_texture
		= glGetUniformLocation(p_gl_ghosts->shader_program, "live_depth_texture");
	p_gl_ghosts->uniforms.live_depth_cutoff
		= glGetUniformLocation(p_gl_ghosts->shader_program, "live_depth_cutoff");
	p_gl_ghosts->uniforms.count
		= glGetUniformLocation(p_gl_ghosts->shader_program, "count");
	p_gl_ghosts->attributes.position
		= glGetAttribLocation(p_gl_ghosts->shader_program, "position");
	/* check that these uniforms retrieved successfully */
	if ((p_gl_ghosts->uniforms.count < 0) || 
		(p_gl_ghosts->uniforms.live_video_texture < 0) || 
		(p_gl_ghosts->uniforms.live_depth_texture < 0) ||
		(p_gl_ghosts->uniforms.live_depth_cutoff < 0) || 
		(p_gl_ghosts->attributes.position < 0)) 
	{
		LOG_ERROR(
			"failed to get uniform/attribute locations from shaders");
		return ERR_FAILED_CREATE;
	}	

	/* get array of video textures, depth textures and depth cutoffs */
	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		if (sprintf(uniform_string, "video_texture_%02i", (int)index) < 0) {
			LOG_ERROR("failed to create string for uniform");
			return ERR_FAILED_CREATE;
		}
		p_gl_ghosts->uniforms.video_textures[index]
			= glGetUniformLocation(p_gl_ghosts->shader_program, uniform_string);
		if (sprintf(uniform_string, "depth_texture_%02i", (int)index) < 0) {
			LOG_ERROR("failed to create string for uniform");
			return ERR_FAILED_CREATE;
		}
		p_gl_ghosts->uniforms.depth_textures[index]
			= glGetUniformLocation(p_gl_ghosts->shader_program, uniform_string);
		if (sprintf(uniform_string, "depth_cutoff_%02i", (int)index) < 0) {
			LOG_ERROR("failed to create string for uniform");
			return ERR_FAILED_CREATE;
		}
		p_gl_ghosts->uniforms.depth_cutoffs[index]
			= glGetUniformLocation(p_gl_ghosts->shader_program, uniform_string);
		if ((p_gl_ghosts->uniforms.video_textures[index] < 0) || 
			(p_gl_ghosts->uniforms.depth_textures[index] < 0) ||
			(p_gl_ghosts->uniforms.depth_cutoffs[index] < 0))
		{
			LOG_ERROR("failed to get uniform/attribute locations from shaders");
			return ERR_FAILED_CREATE;
		}	
	}
	return NO_ERROR;
}

void _display(void* data) {
	status_t   error        = NO_ERROR;
	gl_ghosts* p_gl_ghosts  = (gl_ghosts*)data;
	size_t     num_frames   = 0;
	size_t     num_clips    = 0;
	size_t     index        = 0;
	void*      video_buffer = NULL;
	void*      depth_buffer = NULL;
	double     delta        = 0.0;

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

	error = freerec_clip_count(p_gl_ghosts->freerec, &num_clips);
	if (0 != error) {
		LOG_ERROR("error getting clip count");
		return; 
	}

	/* prepare graphics card to accept new image data */
	glUseProgram(p_gl_ghosts->shader_program);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
	glUniform1i(p_gl_ghosts->uniforms.count, (int)num_clips);

	/* loop through clips adding image data to opengl */
	for (index = 0; index < num_clips; index++) {
		/* TODO: currently setting same depth cutoff for all.  Will probably
		 * want to make this specific to each clip in the future */
		glUniform1f(
			p_gl_ghosts->uniforms.depth_cutoffs[index], 
			p_gl_ghosts->layers[index].depth);
		/* get video and depth information from freerec */
		error = freerec_clip_frame_count(p_gl_ghosts->freerec, index, &num_frames);
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
				p_gl_ghosts->freerec,
				index,
				&video_buffer,
				&depth_buffer);
			if (0 != error) {
				LOG_ERROR("error getting frame");
				num_frames = 0;
			}


			/*
			p_gl_ghosts->frame_counters[index]++;
			if (p_gl_ghosts->frame_counters[index] >= num_frames) {
				p_gl_ghosts->frame_counters[index] = 0;
			}
			*/
			
			if (video_buffer) {
				gl_pixel_buffer_set_data(
					p_gl_ghosts->gl_video_buffers[index],
					video_buffer);
				gl_pixel_buffer_display(
					p_gl_ghosts->gl_video_buffers[index],
					p_gl_ghosts->uniforms.video_textures[index]);
			}

			if (depth_buffer) {
				gl_pixel_buffer_set_data(
					p_gl_ghosts->gl_depth_buffers[index],
					depth_buffer);
				/* use GL_RED_SCALE because the depth data only has 1 number 
				 * per pixel */
				glPixelTransferf(GL_RED_SCALE, p_gl_ghosts->depth_scale);
				gl_pixel_buffer_display(
					p_gl_ghosts->gl_depth_buffers[index],
					p_gl_ghosts->uniforms.depth_textures[index]);
				glPixelTransferf(GL_RED_SCALE, 1.0f);
			}
		}
	}
	/* display live data */
	error = kinect_manager_live_video(
		p_gl_ghosts->kinect_manager,
		&video_buffer);
	if (NO_ERROR == error) {
		glUniform1f(
			p_gl_ghosts->uniforms.live_depth_cutoff,
			p_gl_ghosts->depth_cutoff);
		gl_pixel_buffer_set_data(p_gl_ghosts->gl_live_video_buffer, video_buffer);
		gl_pixel_buffer_display(
			p_gl_ghosts->gl_live_video_buffer,
			p_gl_ghosts->uniforms.live_video_texture);
	}
	else {
		LOG_WARNING("failed to get live video frame");
	}

	error = kinect_manager_live_depth(
		p_gl_ghosts->kinect_manager,
		&depth_buffer);
	if (NO_ERROR == error) {
		gl_pixel_buffer_set_data(p_gl_ghosts->gl_live_depth_buffer, depth_buffer);
		/* use GL_RED_SCALE because the depth data only has 1 number 
		 * per pixel */
		glPixelTransferf(GL_RED_SCALE, p_gl_ghosts->depth_scale);
		gl_pixel_buffer_display(
			p_gl_ghosts->gl_live_depth_buffer,
			p_gl_ghosts->uniforms.live_depth_texture);
		glPixelTransferf(GL_RED_SCALE, 1.0f);
	}
	else {
		LOG_WARNING("failed to get live video frame");
	}

	/* set canvas to draw */
	gl_canvas_display(
		p_gl_ghosts->canvas,
		p_gl_ghosts->attributes.position);
	
	/* apply new buffer */
	glutSwapBuffers();
}

void _reshape(int width, int height, void* data) {
	/* nothing todo here... yet */
}

void _idle(void* data) {
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
		glutPostRedisplay();
	}
	else {
		LOG_WARNING("no frame captured");
	}
}

void _keyboard(unsigned char key, int mouseX, int mouseY, void* data) {
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;
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
			_stop_recording(p_gl_ghosts);
			LOG_INFO("stopping recording");
		}
		else {
			_start_recording(p_gl_ghosts);
			LOG_INFO("starting recording");
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
		error = freerec_capture_video(p_ghosts->freerec, video_data, timestamp);
		if (NO_ERROR != error) {
			LOG_ERROR("error capturing video frame");
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
	
	p_ghosts = (gl_ghosts*)user_data;
	if (NULL == p_ghosts) {
		LOG_ERROR("null pointer");
		return;
	}

	if (p_ghosts->do_record) {
		error = freerec_capture_depth(p_ghosts->freerec, depth_data, timestamp);
		if (NO_ERROR != error) {
			LOG_ERROR("error capturing depth frame");
			return;
		}
	}
}

void _cleanup(void* data) {
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;
	if (p_gl_ghosts == NULL) {
		return;
	}
	/* clean up */
	kinect_manager_destroy(p_gl_ghosts->kinect_manager);
	command_destroy(p_gl_ghosts->cmdr);
	freerec_release(p_gl_ghosts->freerec);
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
	status = freerec_clip_count(p_gl_ghosts->freerec, &num_clips);
	if (NO_ERROR != status) {
		LOG_ERROR("failed to get clip count");
		return status;
	}

	if (num_clips >= TEXTURE_CAPACITY) {
		LOG_WARNING("cannot create any more clips");
		return NO_ERROR;
	}

	status = kinect_manager_reset_timestamp(p_gl_ghosts->kinect_manager);
	if (NO_ERROR != status) {
		LOG_ERROR("failed to reset record timer");
		return status;
	}

	p_gl_ghosts->do_record = TRUE;

	return NO_ERROR;
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
	status = freerec_action(p_gl_ghosts->freerec);
	if (NO_ERROR != status) {
		LOG_ERROR("failed to start new clip");
		return status;
	}

	/* initialize new layer for display */
	status = freerec_clip_count(p_gl_ghosts->freerec, &count);
	if (NO_ERROR != status) {
		LOG_ERROR("failed to get clip count");
		return status;
	}
	if (count >= TEXTURE_CAPACITY) {
		LOG_WARNING("cannot create any more clips");
		return NO_ERROR;
	}

	last_clip = count - 1;
	status = _init_layer(
		p_gl_ghosts->depth_cutoff,
		p_gl_ghosts->freerec,
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
	freerec_handle_t freerec, 
	size_t freerec_clip_index, 
	layer_t* p_layer)
{
	status_t status  = NO_ERROR;
	void*    p_dummy = NULL;

	p_layer->depth = depth;
	p_layer->frame = 0;
	/* get timestamp of first frame */
	status = freerec_clip_video_frame(
		freerec,
		freerec_clip_index,
		0,
		&p_dummy,
		&(p_layer->time));

	if (NO_ERROR != status) {
		LOG_ERROR("failed to get first timestamp");
		return status;
	}
	/* get next timestamp */
	status = freerec_clip_video_frame(
		freerec,
		freerec_clip_index,
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
		freerec_handle_t freerec,
		size_t freerec_clip_index,
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
		error = freerec_clip_frame_count(freerec, freerec_clip_index, &count);
		if (0 != error) {
			LOG_ERROR("error getting frames");
			return error;
		}
		/* check for rollover of clip and loop if necessary */
		if (p_layer->frame >= count) {
			p_layer->frame = 0;
			error = freerec_clip_video_frame(
				freerec, 
				freerec_clip_index,
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
		error = freerec_clip_video_frame(
			freerec, 
			freerec_clip_index,
			next_frame,
			&dummy,
			&(p_layer->next_time));
		if (0 != error) {
			LOG_ERROR("error getting frames");
			return error;
		}
	}

	error = freerec_clip_video_frame(
		freerec,
		freerec_clip_index,
		p_layer->frame,
		p_video_buffer,
		&timestamp);
	if (0 != error) {
		LOG_ERROR("error getting frames");
		return error;
	}
	error = freerec_clip_depth_frame(
		freerec,
		freerec_clip_index,
		p_layer->frame,
		p_depth_buffer,
		&timestamp);
	if (0 != error) {
		LOG_ERROR("error getting frames");
		return error;
	}

	return NO_ERROR;
}


