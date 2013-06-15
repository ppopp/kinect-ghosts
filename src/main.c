#include "glut_main.h"
#include "gl_shader.h"
#include "gl_canvas.h"
#include "gl_pixel_buffer.h"
#include "test_image.h"

#include "log.h"
#include "command.h"

#include <libfreenect/libfreenect.h>
#include <GLUT/glut.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MAX_COMMAND_LENGTH (512)

static const freenect_resolution _ghosts_resolution = FREENECT_RESOLUTION_MEDIUM;
static const double _min_camera_angle = -35.0;
static const double _max_camera_angle = 35.0;
static const unsigned char _newline = 10;
static const unsigned char _carriage_return = 13;
static log_level _print_log_level = LogLevelWarning;

typedef struct _gl_ghosts {
	const char* vertexShaderPath;
	GLuint vertexShader;
	const char* fragmentShaderPath;
	GLuint fragment_shader;
	GLuint shaderProgram;

	gl_pixel_buffer glDepthBuffer; 
	gl_pixel_buffer glVideoBuffer; 
	gl_canvas canvas;

	struct {
		GLint videoTexture;
		GLint depthTexture;
		GLint depthCutoff;
	} uniforms;

	struct {
		GLint position;
	} attributes;

	double cameraAngle;
	float depthCutoff;
	float depthScale;
	freenect_context *fnctx;
	freenect_device *fndevice;
	freenect_frame_mode videoMode;
	freenect_frame_mode depthMode;
	void* videoBuffer;
	void* depthBuffer;
	char command[MAX_COMMAND_LENGTH];
	size_t commandPos;
	commander cmdr;
} gl_ghosts; 


static void _reshape(int width, int height, void* data);
static void _keyboard(
	unsigned char key, 
	int mouseX, 
	int mouseY, 
	void* userData);
static void _special(
	int key,
	int mouseX, 
	int mouseY, 
	void* userData);
static void _idle(void* data);
static void _display(void* data);
static int _init(void* data);
static void _cleanup(void* data);
static int _init_opengl(gl_ghosts* pGlGhosts);
static int _init_freenect(
	gl_ghosts* pGlGhosts, 
	freenect_resolution resolution);
static void _freenect_log_callback(
	freenect_context *dev, 
	freenect_loglevel level, 
	const char *msg);
static void _video_cb(freenect_device *dev, void *video, uint32_t timestamp);
static void _depth_cb(freenect_device *dev, void *depth, uint32_t timestamp);
static void _log_cb(
	const char* source, 
	log_level level, 
	void* data, 
	const char* message);

static int _command_set(const char* cmd, void* data);

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
	glGhosts.vertexShaderPath = "glsl/vertex.vert";
	glGhosts.fragmentShaderPath = "glsl/fragment.frag";

	userData = &glGhosts;
	/* run main loop */
	LOG_DEBUG("ghosts", NULL, "start main loop");
	error = glut_main(argc, argv, "ghosts", &callbacks, userData);
	LOG_DEBUG("ghosts", NULL, "end main loop");

	log_remove_callback(_log_cb);
	return 0;
}

int _init(void* data) {
	int error = 0;
	gl_ghosts* pGlGhosts = (gl_ghosts*)data;
	pGlGhosts->vertexShader = 0;
	pGlGhosts->fragment_shader = 0;
	pGlGhosts->shaderProgram = 0;

	pGlGhosts->glDepthBuffer = NULL;
	pGlGhosts->glVideoBuffer = NULL;
	pGlGhosts->canvas = NULL;
	pGlGhosts->uniforms.videoTexture = 0;
	pGlGhosts->uniforms.depthTexture = 0;
	pGlGhosts->uniforms.depthCutoff = -1;
	pGlGhosts->attributes.position = 0;
	pGlGhosts->depthCutoff = 0;
	pGlGhosts->depthScale = 1.0f;
	pGlGhosts->cameraAngle = 0.0;

	pGlGhosts->fnctx = NULL;
	pGlGhosts->fndevice = NULL;
	pGlGhosts->videoMode.is_valid = -1;
	pGlGhosts->depthMode.is_valid = -1;
	pGlGhosts->videoBuffer = NULL;
	pGlGhosts->depthBuffer = NULL;

	memset(pGlGhosts->command, 0, MAX_COMMAND_LENGTH);
	pGlGhosts->commandPos = 0;
	pGlGhosts->cmdr = command_create();
	command_set_callback(pGlGhosts->cmdr, _command_set);

	error = _init_freenect(pGlGhosts, _ghosts_resolution);
	if (error != 0) {
		LOG_ERROR("ghosts", NULL, "failed to init freenect");
		return error;
	}
	error = _init_opengl(pGlGhosts);
	if (error != 0) {
		LOG_ERROR("ghosts", NULL, "failed to init opengl");
	}
	return error;
}

int _init_opengl(gl_ghosts* pGlGhosts) {

	if (pGlGhosts == NULL) {
		LOG_ERROR("ghosts", NULL, "null pointer");
		return -1;
	}
		
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
	glDisable(GL_DEPTH_TEST);

	size_t bitsPerPixel = 
		pGlGhosts->videoMode.data_bits_per_pixel + 
		pGlGhosts->videoMode.padding_bits_per_pixel;
	pGlGhosts->glVideoBuffer = gl_pixel_buffer_create(
		pGlGhosts->videoMode.width,
		pGlGhosts->videoMode.height,
		GL_TEXTURE_2D,
		GL_RGBA, 
		GL_RGB,
		GL_UNSIGNED_BYTE,
		bitsPerPixel / 8,
		GL_DYNAMIC_DRAW);
	if (NULL == pGlGhosts->glVideoBuffer) {
		LOG_ERROR("ghosts", NULL, "failed to create screen");
		return -1;
	}

	bitsPerPixel = 
		pGlGhosts->depthMode.data_bits_per_pixel + 
		pGlGhosts->depthMode.padding_bits_per_pixel;
	pGlGhosts->glDepthBuffer = gl_pixel_buffer_create(
		pGlGhosts->depthMode.width,
		pGlGhosts->depthMode.height,
		GL_TEXTURE_2D,
		GL_RGBA,
		GL_RED,
		GL_UNSIGNED_SHORT,
		bitsPerPixel / 8,
		GL_DYNAMIC_DRAW);
	if (NULL == pGlGhosts->glDepthBuffer) {
		LOG_ERROR("ghosts", NULL, "failed to create screen");
		return -1;
	}

	pGlGhosts->canvas= gl_canvas_create();
	if (NULL == pGlGhosts->canvas) {
		LOG_ERROR("ghosts", NULL, "failed to create canvas");
		return -1;
	}
	
	pGlGhosts->vertexShader = gl_shader_load(
		GL_VERTEX_SHADER,
		pGlGhosts->vertexShaderPath);
	if (pGlGhosts->vertexShader == 0) {
		LOG_ERROR("ghosts", NULL, "failed to load vertex shader");
		return -1;
	}
	pGlGhosts->fragment_shader = gl_shader_load(
		GL_FRAGMENT_SHADER,
		pGlGhosts->fragmentShaderPath);
	if (pGlGhosts->fragment_shader == 0) {
		LOG_ERROR("ghosts", NULL, "failed to load fragment shader");
		return -1;
	}
	pGlGhosts->shaderProgram = gl_shader_program(
		pGlGhosts->vertexShader,
		pGlGhosts->fragment_shader);
	if (pGlGhosts->shaderProgram == 0) {
		LOG_ERROR("ghosts", NULL, "failed to link shader program");
		return -1;
	}

	pGlGhosts->uniforms.videoTexture
		= glGetUniformLocation(pGlGhosts->shaderProgram, "videoTexture");
	pGlGhosts->uniforms.depthTexture
		= glGetUniformLocation(pGlGhosts->shaderProgram, "depthTexture");
	pGlGhosts->uniforms.depthCutoff
		= glGetUniformLocation(pGlGhosts->shaderProgram, "depthCutoff");
	pGlGhosts->attributes.position
		= glGetAttribLocation(pGlGhosts->shaderProgram, "position");
	if ((pGlGhosts->uniforms.videoTexture < 0) || 
		(pGlGhosts->uniforms.depthTexture < 0) ||
		(pGlGhosts->uniforms.depthCutoff < 0) || 
		(pGlGhosts->attributes.position < 0)) 
	{
		LOG_ERROR(
			"ghosts", 
			NULL, 
			"failed to get uniform/attribute locations from shaders");
		return -1;
	}	
	return 0;
}

int _init_freenect(gl_ghosts* pGlGhosts, freenect_resolution resolution) {
	int error = 0;
	int numDevices = 0;
	int bitCount = 0;

	if (pGlGhosts == NULL) {
		LOG_ERROR("ghosts", NULL, "null pointer");
		return -1;
	}

	error = freenect_init(&(pGlGhosts->fnctx), NULL);
	freenect_set_log_callback(pGlGhosts->fnctx, _freenect_log_callback);
	freenect_set_log_level(pGlGhosts->fnctx, FREENECT_LOG_DEBUG);

	numDevices = freenect_num_devices(pGlGhosts->fnctx);
	if (numDevices < 1) {
		LOG_WARNING("ghosts", NULL, "no kinect devices found");
		return -1;
	}
	/* assume only one device is connected.  open the first device */
	error = freenect_open_device(
		pGlGhosts->fnctx, 
		&(pGlGhosts->fndevice), 
		0);
	if (error) {
		LOG_ERROR("ghosts", NULL, "failed to connect to freenect device");
		return error;
	}
	
	/* setup video mode and buffers */
	pGlGhosts->videoMode = freenect_find_video_mode(
		resolution, 
		FREENECT_VIDEO_RGB);
	if (!(pGlGhosts->videoMode.is_valid)) {
		LOG_ERROR("ghosts", NULL, "invalid video mode");
		return -1;
	}
	error = freenect_set_video_mode(pGlGhosts->fndevice, pGlGhosts->videoMode);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to set video mode");
		return error;
	}
	pGlGhosts->videoBuffer = malloc(pGlGhosts->videoMode.bytes);
	if (pGlGhosts->videoBuffer == NULL) {
		LOG_ERROR("ghosts", NULL, "failed to allocate video buffer");
		return -1;
	}
	error = freenect_set_video_buffer(
		pGlGhosts->fndevice, 
		pGlGhosts->videoBuffer);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to set video buffer");
		return error;
	}
	freenect_set_video_callback(pGlGhosts->fndevice, _video_cb);

	/* setup detph mode and buffers */
	/*
	pGlGhosts->depthMode = freenect_find_depth_mode(
		resolution, 
		FREENECT_DEPTH_11BIT);
		*/
	pGlGhosts->depthMode = freenect_find_depth_mode(
		resolution, 
		FREENECT_DEPTH_REGISTERED);
	if (!(pGlGhosts->depthMode.is_valid)) {
		LOG_ERROR("ghosts", NULL, "invalid depth mode");
		return -1;
	}
	error = freenect_set_depth_mode(pGlGhosts->fndevice, pGlGhosts->depthMode);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to set depth mode");
		return error;
	}
	pGlGhosts->depthBuffer = malloc(pGlGhosts->depthMode.bytes);
	if (pGlGhosts->depthBuffer == NULL) {
		LOG_ERROR("ghosts", NULL, "failed to allocate depth buffer");
		return -1;
	}
	error = freenect_set_depth_buffer(
		pGlGhosts->fndevice, 
		pGlGhosts->depthBuffer);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to set depth buffer");
		return error;
	}
	freenect_set_depth_callback(pGlGhosts->fndevice, _depth_cb);
	/* set scaling for depth data */
	bitCount = pGlGhosts->depthMode.data_bits_per_pixel 
		+ pGlGhosts->depthMode.padding_bits_per_pixel;
	pGlGhosts->depthScale = (float)pow(2, bitCount) 
		/ (float)FREENECT_DEPTH_MM_MAX_VALUE;

	/* start collecting from device */
	error = freenect_start_video(pGlGhosts->fndevice);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to start video");
		return error;
	}
	error = freenect_start_depth(pGlGhosts->fndevice);
	if (error < 0) {
		LOG_ERROR("ghosts", NULL, "failed to start depth");
	}

	return error;
}

void _display(void* data) {
	gl_ghosts* pGlGhosts = (gl_ghosts*)data;
	if (pGlGhosts == NULL) {
		LOG_ERROR("ghosts", NULL, "null pointer");
		return;
	}
	
	glUseProgram(pGlGhosts->shaderProgram);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

	glUniform1f(pGlGhosts->uniforms.depthCutoff, pGlGhosts->depthCutoff);
	
	gl_pixel_buffer_set_data(
		pGlGhosts->glVideoBuffer,
		pGlGhosts->videoBuffer);

	gl_pixel_buffer_set_data(
		pGlGhosts->glDepthBuffer,
		pGlGhosts->depthBuffer);
	
	glPixelTransferf(GL_RED_SCALE, pGlGhosts->depthScale);
	gl_pixel_buffer_display(
		pGlGhosts->glDepthBuffer,
		pGlGhosts->uniforms.depthTexture);
	glPixelTransferf(GL_RED_SCALE, 1.0f);

	
	gl_pixel_buffer_display(
		pGlGhosts->glVideoBuffer,
		pGlGhosts->uniforms.videoTexture);

	gl_canvas_display(
		pGlGhosts->canvas,
		pGlGhosts->attributes.position);
	
	glutSwapBuffers();
}

void _reshape(int width, int height, void* data) {
}

void _idle(void* data) {
	gl_ghosts* pGlGhosts = (gl_ghosts*)data;
	if (pGlGhosts == NULL) {
		LOG_WARNING("ghosts", NULL, "null pointer");
		return;
	}
	freenect_process_events(pGlGhosts->fnctx);
	glutPostRedisplay();
}

void _keyboard(unsigned char key, int mouseX, int mouseY, void* data) {
	gl_ghosts* pGlGhosts = (gl_ghosts*)data;
	if (pGlGhosts == NULL) {
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
		cmdResult = command_handle(
			pGlGhosts->cmdr, 
			pGlGhosts->command, 
			pGlGhosts);
		if (cmdResult == COMMAND_CONTINUE) {
			log_messagef(
				"ghosts", 
				LogLevelWarning, 
				NULL,
				"unhandled command: %s @%s:%u",
				pGlGhosts->command, 
				__FILE__, 
				__LINE__);
		}
		else if (cmdResult == COMMAND_ERROR) {
			log_messagef(
				"ghosts", 
				LogLevelError, 
				NULL,
				"error handling command: %s @%s:%u",
				pGlGhosts->command, 
				__FILE__, 
				__LINE__);
		}

		pGlGhosts->commandPos = 0;
		memset(pGlGhosts->command, 0, MAX_COMMAND_LENGTH);
	}
	else {
		(pGlGhosts->command)[pGlGhosts->commandPos] = (char)key;
		pGlGhosts->commandPos++;
		if (pGlGhosts->commandPos >= MAX_COMMAND_LENGTH) {
			pGlGhosts->commandPos = 0;
			memset(pGlGhosts->command, 0, MAX_COMMAND_LENGTH);
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
	gl_ghosts* pGlGhosts = (gl_ghosts*)data;
	if (pGlGhosts == NULL) {
		LOG_WARNING("ghosts", NULL, "null pointer");
		return;
	}

	switch (key) {
		case GLUT_KEY_UP:
			pGlGhosts->cameraAngle += 2.0;
		case GLUT_KEY_DOWN:
			pGlGhosts->cameraAngle -= 1.0;
			if (pGlGhosts->cameraAngle > _max_camera_angle) {
				pGlGhosts->cameraAngle = _max_camera_angle;
			}
			else if (pGlGhosts->cameraAngle < _min_camera_angle) {
				pGlGhosts->cameraAngle = _min_camera_angle;
			}
			if (freenect_set_tilt_degs(
					pGlGhosts->fndevice, 
					pGlGhosts->cameraAngle) != 0)
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
	/* remove the trailing newline from message */
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
	//LOG_DEBUG("ghosts", NULL, "recieved video callback");
}

void _depth_cb(freenect_device *dev, void *depth, uint32_t timestamp) {
	//LOG_DEBUG("ghosts", NULL, "recieved depth callback");
}

void _cleanup(void* data) {
	gl_ghosts* pGlGhosts = (gl_ghosts*)data;
	if (pGlGhosts == NULL) {
		return;
	}
	/* clean up */
	freenect_stop_video(pGlGhosts->fndevice);
	freenect_close_device(pGlGhosts->fndevice);
	freenect_shutdown(pGlGhosts->fnctx);
	free(pGlGhosts->videoBuffer);
	pGlGhosts->videoBuffer = NULL;
	free(pGlGhosts->depthBuffer);
	pGlGhosts->depthBuffer = NULL;
	command_destroy(pGlGhosts->cmdr);
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
	gl_ghosts* pGlGhosts = (gl_ghosts*)data;

	if (pGlGhosts == NULL) {
		return COMMAND_CONTINUE;
	}

	value = command_find_target(cmd, "depth", '=', &isspace);
	if (value) {
		result = sscanf(value, "%f", &(pGlGhosts->depthCutoff));
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

