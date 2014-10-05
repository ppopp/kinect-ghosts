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

#include <libfreenect/libfreenect.h>
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

static const freenect_loglevel _freenect_log_level = FREENECT_LOG_WARNING;

/* integer char values used to define end of command */
static const unsigned char _newline = 10;
static const unsigned char _carriage_return = 13;
static const unsigned char _spacebar = (unsigned char)' ';

static const freenect_resolution _ghosts_resolution = FREENECT_RESOLUTION_MEDIUM;
static const double _min_camera_angle = -35.0;
static const double _max_camera_angle = 35.0;
static log_level _print_log_level = LogLevelDebug;

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


	double cameraAngle;
	float depth_cutoff;
	float depth_scale;
	freenect_context *fnctx;
	freenect_device *fndevice;
	freenect_frame_mode video_mode;
	freenect_frame_mode depth_mode;
	void* video_buffer;
	void* depth_buffer;
	struct timeval timeout;
	char command[MAX_COMMAND_LENGTH];
	size_t commandPos;
	commander cmdr;

	freerec_handle_t freerec;
	bool_t do_record;
	layer_t layers[TEXTURE_CAPACITY];
	timer_handle_t record_timer;
	timer_handle_t playback_timer;
	
	double record_timestamp;
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

static int _init_opengl(gl_ghosts* p_gl_ghosts);
static int _init_freenect(
	gl_ghosts* p_gl_ghosts, 
	freenect_resolution resolution);

/* freenect callbacks */
static void _freenect_log_callback(
	freenect_context *dev, 
	freenect_loglevel level, 
	const char *msg);
static void _video_cb(freenect_device *dev, void *video, uint32_t timestamp);
static void _depth_cb(freenect_device *dev, void *depth, uint32_t timestamp);

/* system logging callback */
static void _log_cb(
	const char* source, 
	log_level level, 
	void* data, 
	const char* message);

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

	log_add_callback(_log_cb, LogLevelDebug);
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
	LOG_DEBUG("ghosts", NULL, "start main loop");
	error = glut_main(argc, argv, "ghosts", &callbacks, userData);
	LOG_DEBUG("ghosts", NULL, "end main loop");

	log_remove_callback(_log_cb);
	return 0;
}

int _init(void* data) {
	int        error       = 0;
	size_t     index       = 0;
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
	p_gl_ghosts->cameraAngle = 0.0;

	p_gl_ghosts->fnctx = NULL;
	p_gl_ghosts->fndevice = NULL;
	p_gl_ghosts->video_mode.is_valid = -1;
	p_gl_ghosts->depth_mode.is_valid = -1;
	p_gl_ghosts->video_buffer = NULL;
	p_gl_ghosts->depth_buffer = NULL;
	p_gl_ghosts->timeout.tv_sec = 0;
	p_gl_ghosts->timeout.tv_usec = 1000;


	memset(p_gl_ghosts->command, 0, MAX_COMMAND_LENGTH);
	p_gl_ghosts->commandPos = 0;
	p_gl_ghosts->cmdr = command_create();
	command_set_callback(p_gl_ghosts->cmdr, _command_set);

	p_gl_ghosts->freerec = NULL;
	p_gl_ghosts->record_timer = NULL;
	p_gl_ghosts->playback_timer = NULL;
	p_gl_ghosts->do_record = FALSE;

	/* initialize kinect device and start callbacks */
	error = _init_freenect(p_gl_ghosts, _ghosts_resolution);
	if (error != 0) {
		LOG_ERROR("ghosts", NULL, "failed to init freenect");
		return error;
	}

	/* initialize kinect recording */
	error = freerec_create(
		&(p_gl_ghosts->video_mode),
		&(p_gl_ghosts->depth_mode),
		_max_recorded_bytes,
		&(p_gl_ghosts->freerec));
	if (0 != error) {
		LOG_ERROR("ghosts", NULL, "failed to init freerec");
		return error;
	}
	error = freerec_action(p_gl_ghosts->freerec);
	if (0 != error) {
		LOG_ERROR("ghosts", NULL, "failed to init freerec");
		return error;
	}

	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		p_gl_ghosts->layers[index].depth = 0.0f;
		p_gl_ghosts->layers[index].time = 0.0;
		p_gl_ghosts->layers[index].frame = 0;
	}

	error = timer_create(&(p_gl_ghosts->record_timer));
	if (0 != error) {
		LOG_ERROR("ghosts", NULL, "failed to create record timer");
		return error;
	}

	error = timer_create(&(p_gl_ghosts->playback_timer));
	if (0 != error) {
		LOG_ERROR("ghosts", NULL, "failed to create playback timer");
		return error;
	}

	/* initialize opengl shaders, buffers etc */
	error = _init_opengl(p_gl_ghosts);
	if (error != 0) {
		LOG_ERROR("ghosts", NULL, "failed to init opengl");
	}
	return error;
}

int _init_opengl(gl_ghosts* p_gl_ghosts) {
	size_t bits_per_pixel      = 0;
	size_t index               = 0;
	char   uniform_string[256] = {'\0'};
	/* Initialize opengl to have two pixel buffers, one for video data from
	 * the kinect, and another for the depth data.  Data types / formats here
	 * have to match those used when initializing the kinect.  The pixel 
	 * buffers offer fast streaming of data from the kinect to the graphics
	 * card */
	if (p_gl_ghosts == NULL) {
		LOG_ERROR("ghosts", NULL, "null pointer");
		return -1;
	}
		
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR); 
	glDisable(GL_DEPTH_TEST);

	/* create video pixel streaming buffer */
	bits_per_pixel = 
		p_gl_ghosts->video_mode.data_bits_per_pixel + 
		p_gl_ghosts->video_mode.padding_bits_per_pixel;
	p_gl_ghosts->gl_live_video_buffer = gl_pixel_buffer_create(
		p_gl_ghosts->video_mode.width,
		p_gl_ghosts->video_mode.height,
		GL_TEXTURE_2D,
		GL_RGBA, 
		GL_RGB,
		GL_UNSIGNED_BYTE,
		bits_per_pixel / 8,
		GL_DYNAMIC_DRAW);
	if (NULL == p_gl_ghosts->gl_live_video_buffer) {
		LOG_ERROR("ghosts", NULL, "failed to create screen");
		return -1;
	}
	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		p_gl_ghosts->gl_video_buffers[index] = gl_pixel_buffer_create(
			p_gl_ghosts->video_mode.width,
			p_gl_ghosts->video_mode.height,
			GL_TEXTURE_2D,
			GL_RGBA, 
			GL_RGB,
			GL_UNSIGNED_BYTE,
			bits_per_pixel / 8,
			GL_DYNAMIC_DRAW);
		if (NULL == p_gl_ghosts->gl_video_buffers[index]) {
			LOG_ERROR("ghosts", NULL, "failed to create screen");
			return -1;
		}
	}

	/* create depth pixel streaming buffer */
	bits_per_pixel = 
		p_gl_ghosts->depth_mode.data_bits_per_pixel + 
		p_gl_ghosts->depth_mode.padding_bits_per_pixel;
	p_gl_ghosts->gl_live_depth_buffer = gl_pixel_buffer_create(
		p_gl_ghosts->depth_mode.width,
		p_gl_ghosts->depth_mode.height,
		GL_TEXTURE_2D,
		GL_RGBA,
		GL_RED,
		GL_UNSIGNED_SHORT,
		bits_per_pixel / 8,
		GL_DYNAMIC_DRAW);
	if (NULL == p_gl_ghosts->gl_live_depth_buffer) {
		LOG_ERROR("ghosts", NULL, "failed to create screen");
		return -1;
	}
	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		p_gl_ghosts->gl_depth_buffers[index] = gl_pixel_buffer_create(
			p_gl_ghosts->depth_mode.width,
			p_gl_ghosts->depth_mode.height,
			GL_TEXTURE_2D,
			GL_RGBA,
			GL_RED,
			GL_UNSIGNED_SHORT,
			bits_per_pixel / 8,
			GL_DYNAMIC_DRAW);
		if (NULL == p_gl_ghosts->gl_depth_buffers[index]) {
			LOG_ERROR("ghosts", NULL, "failed to create screen");
			return -1;
		}
	}

	/* create the canvas, which is simply a square in 3D space upon which the
	 * pixel buffers are drawn */
	p_gl_ghosts->canvas = gl_canvas_create();
	if (NULL == p_gl_ghosts->canvas) {
		LOG_ERROR("ghosts", NULL, "failed to create canvas");
		return -1;
	}
	
	/* load customized shader programs which handle depth cutoff and masking
	 * of video data by depth data */
	p_gl_ghosts->vertex_shader = gl_shader_load(
		GL_VERTEX_SHADER,
		p_gl_ghosts->vertex_shader_path);
	if (p_gl_ghosts->vertex_shader == 0) {
		LOG_ERROR("ghosts", NULL, "failed to load vertex shader");
		return -1;
	}
	p_gl_ghosts->fragment_shader = gl_shader_load(
		GL_FRAGMENT_SHADER,
		p_gl_ghosts->fragment_shader_path);
	if (p_gl_ghosts->fragment_shader == 0) {
		LOG_ERROR("ghosts", NULL, "failed to load fragment shader");
		return -1;
	}
	p_gl_ghosts->shader_program = gl_shader_program(
		p_gl_ghosts->vertex_shader,
		p_gl_ghosts->fragment_shader);
	if (p_gl_ghosts->shader_program == 0) {
		LOG_ERROR("ghosts", NULL, "failed to link shader program");
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
			"ghosts", 
			NULL, 
			"failed to get uniform/attribute locations from shaders");
		return -1;
	}	

	/* get array of video textures, depth textures and depth cutoffs */
	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		if (sprintf(uniform_string, "video_texture_%02i", (int)index) < 0) {
			LOG_ERROR(
				"ghosts", 
				NULL, 
				"failed to create string for uniform");
			return -1;
		}
		p_gl_ghosts->uniforms.video_textures[index]
			= glGetUniformLocation(p_gl_ghosts->shader_program, uniform_string);
		if (sprintf(uniform_string, "depth_texture_%02i", (int)index) < 0) {
			LOG_ERROR(
				"ghosts", 
				NULL, 
				"failed to create string for uniform");
			return -1;
		}
		p_gl_ghosts->uniforms.depth_textures[index]
			= glGetUniformLocation(p_gl_ghosts->shader_program, uniform_string);
		if (sprintf(uniform_string, "depth_cutoff_%02i", (int)index) < 0) {
			LOG_ERROR(
				"ghosts", 
				NULL, 
				"failed to create string for uniform");
			return -1;
		}
		p_gl_ghosts->uniforms.depth_cutoffs[index]
			= glGetUniformLocation(p_gl_ghosts->shader_program, uniform_string);
		if ((p_gl_ghosts->uniforms.video_textures[index] < 0) || 
			(p_gl_ghosts->uniforms.depth_textures[index] < 0) ||
			(p_gl_ghosts->uniforms.depth_cutoffs[index] < 0))
		{
			LOG_ERROR(
				"ghosts", 
				NULL, 
				"failed to get uniform/attribute locations from shaders");
			return -1;
		}	
	}
	return 0;
}

int _init_freenect(gl_ghosts* p_gl_ghosts, freenect_resolution resolution) {
	/* find and connect to the freenect device, initialize callbacks and start
	 * getting data from the device */
	int error = 0;
	int num_devices = 0;
	int bit_count = 0;

	if (p_gl_ghosts == NULL) {
		LOG_ERROR("ghosts", NULL, "null pointer");
		return -1;
	}

	error = freenect_init(&(p_gl_ghosts->fnctx), NULL);
	freenect_set_log_callback(p_gl_ghosts->fnctx, _freenect_log_callback);
	freenect_set_log_level(p_gl_ghosts->fnctx, _freenect_log_level);

	num_devices = freenect_num_devices(p_gl_ghosts->fnctx);
	if (num_devices < 1) {
		LOG_WARNING("ghosts", NULL, "no kinect devices found");
		return -1;
	}
	/* assume only one device is connected.  open the first device */
	error = freenect_open_device(
		p_gl_ghosts->fnctx, 
		&(p_gl_ghosts->fndevice), 
		0);
	if (error) {
		LOG_ERROR("ghosts", NULL, "failed to connect to freenect device");
		return error;
	}

	/* set user data for device so we can access this struct in video and
	 * depth callbacks */
	freenect_set_user(p_gl_ghosts->fndevice, (void*)p_gl_ghosts);
	
	/* setup video mode and buffers */
	p_gl_ghosts->video_mode = freenect_find_video_mode(
		resolution, 
		FREENECT_VIDEO_RGB);
	if (!(p_gl_ghosts->video_mode.is_valid)) {
		LOG_ERROR("ghosts", NULL, "invalid video mode");
		return -1;
	}
	error = freenect_set_video_mode(p_gl_ghosts->fndevice, p_gl_ghosts->video_mode);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to set video mode");
		return error;
	}
	p_gl_ghosts->video_buffer = malloc(p_gl_ghosts->video_mode.bytes);
	if (p_gl_ghosts->video_buffer == NULL) {
		LOG_ERROR("ghosts", NULL, "failed to allocate video buffer");
		return -1;
	}
	error = freenect_set_video_buffer(
		p_gl_ghosts->fndevice, 
		p_gl_ghosts->video_buffer);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to set video buffer");
		return error;
	}
	freenect_set_video_callback(p_gl_ghosts->fndevice, _video_cb);

	/* setup detph mode and buffers */
	/*
	p_gl_ghosts->depth_mode = freenect_find_depth_mode(
		resolution, 
		FREENECT_DEPTH_11BIT);
		*/
	p_gl_ghosts->depth_mode = freenect_find_depth_mode(
		resolution, 
		FREENECT_DEPTH_REGISTERED);
	if (!(p_gl_ghosts->depth_mode.is_valid)) {
		LOG_ERROR("ghosts", NULL, "invalid depth mode");
		return -1;
	}
	error = freenect_set_depth_mode(p_gl_ghosts->fndevice, p_gl_ghosts->depth_mode);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to set depth mode");
		return error;
	}
	p_gl_ghosts->depth_buffer = malloc(p_gl_ghosts->depth_mode.bytes);
	if (p_gl_ghosts->depth_buffer == NULL) {
		LOG_ERROR("ghosts", NULL, "failed to allocate depth buffer");
		return -1;
	}
	error = freenect_set_depth_buffer(
		p_gl_ghosts->fndevice, 
		p_gl_ghosts->depth_buffer);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to set depth buffer");
		return error;
	}
	freenect_set_depth_callback(p_gl_ghosts->fndevice, _depth_cb);
	/* set scaling for depth data */
	bit_count = p_gl_ghosts->depth_mode.data_bits_per_pixel 
		+ p_gl_ghosts->depth_mode.padding_bits_per_pixel;
	p_gl_ghosts->depth_scale = (float)pow(2, bit_count) 
		/ (float)FREENECT_DEPTH_MM_MAX_VALUE;

	/* start collecting from device */
	error = freenect_start_video(p_gl_ghosts->fndevice);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to start video");
		return error;
	}
	error = freenect_start_depth(p_gl_ghosts->fndevice);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to start depth");
	}

	return error;
}

void _display(void* data) {
	gl_ghosts* p_gl_ghosts  = (gl_ghosts*)data;
	size_t     num_frames   = 0;
	size_t     num_clips    = 0;
	size_t     index        = 0;
	void*      video_buffer = NULL;
	void*      depth_buffer = NULL;
	int        error        = 0;
	double     delta        = 0.0;

	_downsample_counter++;
	if ((_downsample_counter % DOWNSAMPLE) != 0) {
		return;
	}
	if (p_gl_ghosts == NULL) {
		LOG_ERROR("ghosts", NULL, "null pointer");
		return;
	}

	/* get time since last callback */
	error = timer_current(p_gl_ghosts->playback_timer, &delta);
	if (0 != error) {
		LOG_ERROR("ghosts", NULL, "error getting playback time");
		return; 
	}
	error = timer_reset(p_gl_ghosts->playback_timer);
	if (0 != error) {
		LOG_ERROR("ghosts", NULL, "error reseting playback time");
		return; 
	}

	error = freerec_clip_count(p_gl_ghosts->freerec, &num_clips);
	if (0 != error) {
		LOG_ERROR("ghosts", NULL, "error getting clip count");
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
			LOG_ERROR("ghosts", NULL, "error getting frame count");
			num_frames = 0;
		}

		if (0 == num_frames) {
			LOG_WARNING("ghosts", NULL, "clip with zero frames");
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
				LOG_ERROR("ghosts", NULL, "error getting frame");
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
	glUniform1f(
		p_gl_ghosts->uniforms.live_depth_cutoff,
		p_gl_ghosts->depth_cutoff);
	
	gl_pixel_buffer_set_data(
		p_gl_ghosts->gl_live_video_buffer,
		p_gl_ghosts->video_buffer);
	gl_pixel_buffer_display(
		p_gl_ghosts->gl_live_video_buffer,
		p_gl_ghosts->uniforms.live_video_texture);

	gl_pixel_buffer_set_data(
		p_gl_ghosts->gl_live_depth_buffer,
		p_gl_ghosts->depth_buffer);
	/* use GL_RED_SCALE because the depth data only has 1 number 
	 * per pixel */
	glPixelTransferf(GL_RED_SCALE, p_gl_ghosts->depth_scale);
	gl_pixel_buffer_display(
		p_gl_ghosts->gl_live_depth_buffer,
		p_gl_ghosts->uniforms.live_depth_texture);
	glPixelTransferf(GL_RED_SCALE, 1.0f);

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
	int result = 0;

	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;
	if (p_gl_ghosts == NULL) {
		LOG_WARNING("ghosts", NULL, "null pointer");
		return;
	}
	/* this forces the video and depth callbacks to happen */
	//freenect_process_events(p_gl_ghosts->fnctx);
	result = freenect_process_events_timeout(
		p_gl_ghosts->fnctx, 
		&(p_gl_ghosts->timeout));
	if (0 == result) {
		glutPostRedisplay();
	}
	else {
		LOG_WARNING("ghosts", NULL, "error while process freenect events");
	}
}

void _keyboard(unsigned char key, int mouseX, int mouseY, void* data) {
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;
	if (p_gl_ghosts == NULL) {
		LOG_WARNING("ghosts", NULL, "null pointer");
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
			log_messagef(
				"ghosts", 
				LogLevelWarning, 
				NULL,
				"unhandled command: %s @%s:%u",
				p_gl_ghosts->command, 
				__FILE__, 
				__LINE__);
		}
		else if (cmdResult == COMMAND_ERROR) {
			log_messagef(
				"ghosts", 
				LogLevelError, 
				NULL,
				"error handling command: %s @%s:%u",
				p_gl_ghosts->command, 
				__FILE__, 
				__LINE__);
		}

		p_gl_ghosts->commandPos = 0;
		memset(p_gl_ghosts->command, 0, MAX_COMMAND_LENGTH);
	}
	else if ((key == _spacebar) && (p_gl_ghosts->commandPos == 0)) {
		if (p_gl_ghosts->do_record) {
			_stop_recording(p_gl_ghosts);
			LOG_INFO("ghosts", NULL, "stopping recording");
		}
		else {
			_start_recording(p_gl_ghosts);
			LOG_INFO("ghosts", NULL, "starting recording");
		}
	}
	else {
		(p_gl_ghosts->command)[p_gl_ghosts->commandPos] = (char)key;
		p_gl_ghosts->commandPos++;
		if (p_gl_ghosts->commandPos >= MAX_COMMAND_LENGTH) {
			p_gl_ghosts->commandPos = 0;
			memset(p_gl_ghosts->command, 0, MAX_COMMAND_LENGTH);
			fprintf(stdout, "\n");
			LOG_WARNING("ghosts", NULL, "command exceeds maximum command length");
		}
	}
}

void _special(
	int key,
	int mouseX, 
	int mouseY, 
	void* data)
{
	gl_ghosts* p_gl_ghosts = (gl_ghosts*)data;
	if (p_gl_ghosts == NULL) {
		LOG_WARNING("ghosts", NULL, "null pointer");
		return;
	}

	switch (key) {
		case GLUT_KEY_UP:
			p_gl_ghosts->cameraAngle += 2.0;
		case GLUT_KEY_DOWN:
			p_gl_ghosts->cameraAngle -= 1.0;
			if (p_gl_ghosts->cameraAngle > _max_camera_angle) {
				p_gl_ghosts->cameraAngle = _max_camera_angle;
			}
			else if (p_gl_ghosts->cameraAngle < _min_camera_angle) {
				p_gl_ghosts->cameraAngle = _min_camera_angle;
			}
			if (freenect_set_tilt_degs(
					p_gl_ghosts->fndevice, 
					p_gl_ghosts->cameraAngle) != 0)
			{
				LOG_WARNING("ghosts", NULL, "error when titlting kinect");
			}
			break;
		case GLUT_KEY_LEFT:
			break;
		case GLUT_KEY_RIGHT:
			break;
	}
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
			LOG_ERROR("freenect", dev, msg);
			break;
		case FREENECT_LOG_WARNING:
		case FREENECT_LOG_NOTICE:
			LOG_WARNING("freenect", dev, msg);
			break;
		case FREENECT_LOG_INFO:
			LOG_INFO("freenect", dev, msg);
			break;
		case FREENECT_LOG_DEBUG:
		case FREENECT_LOG_SPEW:
		case FREENECT_LOG_FLOOD:
			LOG_DEBUG("freenect", dev, msg);
			break;
		default:
			LOG_ERROR("freenect", dev, msg);
			LOG_WARNING("ghosts", NULL, "unhandled freenect log level");
	}
}

void _video_cb(freenect_device *dev, void *video, uint32_t timestamp) {
	gl_ghosts* p_ghosts = NULL;
	int error = 0;
	
	p_ghosts = (gl_ghosts*)freenect_get_user(dev);
	if (NULL == p_ghosts) {
		LOG_ERROR("ghosts", NULL, "null pointer");
		return;
	}

	if (TRUE == p_ghosts->do_record) {
		error = timer_current(
			p_ghosts->record_timer, 
			&(p_ghosts->record_timestamp));
		if (0 != error) {
			LOG_ERROR("ghosts", NULL, "error getting record timestamp");
			return;
		}
		error = freerec_capture_video(
			p_ghosts->freerec, 
			video, 
			p_ghosts->record_timestamp);
		if (0 != error) {
			LOG_ERROR("ghosts", NULL, "error capturing video frame");
			return;
		}
	}
}

void _depth_cb(freenect_device *dev, void *depth, uint32_t timestamp) {
	gl_ghosts* p_ghosts = NULL;
	int error = 0;
	
	p_ghosts = (gl_ghosts*)freenect_get_user(dev);
	if (NULL == p_ghosts) {
		LOG_ERROR("ghosts", NULL, "null pointer");
		return;
	}

	if (p_ghosts->do_record) {
		error = freerec_capture_depth(
			p_ghosts->freerec, 
			depth, 
			p_ghosts->record_timestamp);
		if (0 != error) {
			LOG_ERROR("ghosts", NULL, "error capturing depth frame");
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
	freenect_stop_video(p_gl_ghosts->fndevice);
	freenect_close_device(p_gl_ghosts->fndevice);
	freenect_shutdown(p_gl_ghosts->fnctx);
	free(p_gl_ghosts->video_buffer);
	p_gl_ghosts->video_buffer = NULL;
	free(p_gl_ghosts->depth_buffer);
	p_gl_ghosts->depth_buffer = NULL;
	command_destroy(p_gl_ghosts->cmdr);
	freerec_release(p_gl_ghosts->freerec);
	timer_release(p_gl_ghosts->playback_timer);
	p_gl_ghosts->playback_timer = NULL;
	timer_release(p_gl_ghosts->record_timer);
	p_gl_ghosts->record_timer = NULL;
	/* TODO: lots of other cleanup */
}

void _log_cb(
	const char* source, 
	log_level level, 
	void* data, 
	const char* message)
{
	FILE* out = stderr;
	if (level >= LogLevelInfo) {
		out = stdout;
	}

	if (level <= _print_log_level) {
		fprintf(
			out, 
			"[%s][%s] %s\n",
			source,
			log_level_str(level),
			message);
	}
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

	value = command_find_target(cmd, "log", '=', &isspace);
	if (value) {
		static const log_level logLevels[] = {
			LogLevelError, 
			LogLevelWarning, 
			LogLevelInfo, 
			LogLevelDebug
		};
		static const size_t numLogLevels = 
			sizeof(logLevels) / sizeof(log_level);
		size_t i = 0;
		for (i = 0; i < numLogLevels; i++) {
			if (strcmp(value, log_level_str(logLevels[i])) == 0) {
				_print_log_level = logLevels[i];
				return COMMAND_HANDLE_CONTINUE;
			}
		}
		return COMMAND_ERROR;
	}


	return COMMAND_CONTINUE;
}


int _start_recording(gl_ghosts* p_gl_ghosts) {
	status_t status    = NO_ERROR;
	size_t   num_clips = 0;

	if(TRUE == p_gl_ghosts->do_record) {
		LOG_DEBUG("ghosts", NULL, "attempt to start recording when already recording");
	}

	/* check to make sure we can handle a new clip */
	status = freerec_clip_count(p_gl_ghosts->freerec, &num_clips);
	if (NO_ERROR != status) {
		LOG_ERROR("ghosts", NULL, "failed to get clip count");
		return status;
	}

	if (num_clips >= TEXTURE_CAPACITY) {
		LOG_WARNING("ghosts", NULL, "cannot create any more clips");
		return NO_ERROR;
	}

	status = timer_reset(p_gl_ghosts->record_timer);
	if (NO_ERROR != status) {
		LOG_ERROR("ghosts", NULL, "failed to reset record timer");
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
		LOG_DEBUG("ghosts", NULL, "attemp to stop recording when not recording");
		return NO_ERROR;
	}

	/* set flag to stop recording */
	p_gl_ghosts->do_record = FALSE;

	/* start new clip */
	status = freerec_action(p_gl_ghosts->freerec);
	if (NO_ERROR != status) {
		LOG_ERROR("ghosts", NULL, "failed to start new clip");
		return status;
	}

	/* initialize new layer for display */
	status = freerec_clip_count(p_gl_ghosts->freerec, &count);
	if (NO_ERROR != status) {
		LOG_ERROR("ghosts", NULL, "failed to get clip count");
		return status;
	}
	if (count >= TEXTURE_CAPACITY) {
		LOG_WARNING("ghosts", NULL, "cannot create any more clips");
		return NO_ERROR;
	}

	last_clip = count - 1;
	status = _init_layer(
		p_gl_ghosts->depth_cutoff,
		p_gl_ghosts->freerec,
		last_clip,
		&(p_gl_ghosts->layers[last_clip]));
	if (NO_ERROR != status) {
		LOG_ERROR("ghosts", NULL, "failed to init layer");
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
		LOG_ERROR("ghosts", NULL, "failed to get first timestamp");
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
		LOG_ERROR("ghosts", NULL, "failed to get next timestamp");
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
			LOG_ERROR("ghosts", NULL, "error getting frames");
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
				LOG_ERROR("ghosts", NULL, "error getting frames");
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
			LOG_ERROR("ghosts", NULL, "error getting frames");
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
		LOG_ERROR("ghosts", NULL, "error getting frames");
		return error;
	}
	error = freerec_clip_depth_frame(
		freerec,
		freerec_clip_index,
		p_layer->frame,
		p_depth_buffer,
		&timestamp);
	if (0 != error) {
		LOG_ERROR("ghosts", NULL, "error getting frames");
		return error;
	}

	return NO_ERROR;
}


