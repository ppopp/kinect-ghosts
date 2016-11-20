
#include "display_manager.h"

#include <stdlib.h>
#include <string.h>
#include <GLUT/glut.h>

#include "common.h"
#include "log.h"
#include "gl_shader.h"
#include "gl_canvas.h"
#include "gl_pixel_buffer.h"
#include "gl_error.h"


typedef struct _display_manager_s {
	/* Shader variables */
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint shader_program;

	gl_pixel_buffer gl_video_buffers[TEXTURE_CAPACITY];
	gl_pixel_buffer gl_depth_buffers[TEXTURE_CAPACITY];
	gl_canvas canvas;

	struct {
		GLint video_textures[TEXTURE_CAPACITY];
		GLint depth_textures[TEXTURE_CAPACITY];
		GLint depth_cutoffs[TEXTURE_CAPACITY];
		GLint count;
		GLint depth_horizontal_pixel_stride;
		GLint depth_vertical_pixel_stride;
	} uniforms;

	struct {
		GLint position;
	} attributes;

	int layer_count;
	float depth_scale;
	float depth_horizontal_pixel_stride;
	float depth_vertical_pixel_stride;

} display_manager_t;


static status_t _init_opengl(
	display_manager_t* p_dspmgr,
	const char* vertex_shader_path,
	const char* fragment_shader_path,
	size_t video_bits_per_pixel,
	size_t video_width,
	size_t video_height,
	size_t depth_bits_per_pixel,
	size_t depth_width,
	size_t depth_height);

status_t display_manager_create(
	const char* vertex_shader_path,
	const char* fragment_shader_path,
	size_t video_bits_per_pixel,
	size_t video_width,
	size_t video_height,
	size_t depth_bits_per_pixel,
	size_t depth_width,
	size_t depth_height,
	float depth_scale,
	display_manager_handle_t* p_handle) 
{
	status_t error = NO_ERROR;
	display_manager_t* p_dspmgr = NULL;

	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}
	
	if ((video_width < 1) || 
		(video_height < 1) ||
		(depth_width < 1) ||
		(depth_height < 1)) 
	{
		return ERR_INVALID_ARGUMENT;
	}

	p_dspmgr = malloc(sizeof(display_manager_t));
	if (NULL == p_dspmgr) {
		return ERR_FAILED_ALLOC;
	}
	memset(p_dspmgr, 0, sizeof(display_manager_t));

	p_dspmgr->layer_count = 0;
	p_dspmgr->depth_scale = depth_scale;
	p_dspmgr->depth_vertical_pixel_stride = 1.0f / (float)depth_height;
	p_dspmgr->depth_horizontal_pixel_stride = 1.0f / (float)depth_width;
	error = _init_opengl(
		p_dspmgr,
		vertex_shader_path,
		fragment_shader_path,
		video_bits_per_pixel, video_width, video_height,
		depth_bits_per_pixel, depth_width, depth_height);
	if (NO_ERROR != error) {
		display_manager_destroy(p_dspmgr);
		return error;
	}


	*p_handle = p_dspmgr;
	return NO_ERROR;
}

void display_manager_destroy(display_manager_handle_t handle) {
	size_t index = 0;
	if (NULL == handle) {
		return;
	}

	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		gl_pixel_buffer_destroy(&handle->gl_video_buffers[index]);
		gl_pixel_buffer_destroy(&handle->gl_depth_buffers[index]);
	}
	gl_canvas_destroy(&handle->canvas);

	free(handle);
}

status_t _init_opengl(
	display_manager_t* p_dspmgr,
	const char* vertex_shader_path,
	const char* fragment_shader_path,
	size_t video_bits_per_pixel,
	size_t video_width,
	size_t video_height,
	size_t depth_bits_per_pixel,
	size_t depth_width,
	size_t depth_height)
{
	size_t   index               = 0;
	char     uniform_string[256] = {'\0'};

	/* Initialize opengl to have two pixel buffers, one for video data from
	 * the kinect, and another for the depth data.  Data types / formats here
	 * have to match those used when initializing the kinect.  The pixel 
	 * buffers offer fast streaming of data from the kinect to the graphics
	 * card */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_COLOR); 
	//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
	glDisable(GL_DEPTH_TEST);

	/* create video pixel streaming buffer */
	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		p_dspmgr->gl_video_buffers[index] = gl_pixel_buffer_create(
			video_width,
			video_height,
			GL_TEXTURE_2D,
            /*
			GL_RGBA, 
			GL_RGB,
            */
            GL_RGB,
            GL_RGB,
			GL_UNSIGNED_BYTE,
			video_bits_per_pixel / 8,
			GL_DYNAMIC_DRAW);
		if (NULL == p_dspmgr->gl_video_buffers[index]) {
			LOG_ERROR("failed to create screen");
			return -1;
		}
	}

	/* create depth pixel streaming buffer */
	for (index = 0; index < TEXTURE_CAPACITY; index++) {
		p_dspmgr->gl_depth_buffers[index] = gl_pixel_buffer_create(
			depth_width,
			depth_height,
			GL_TEXTURE_2D,
/*			GL_RGBA,
			GL_RED,
            */
            GL_ALPHA,
            GL_ALPHA,
			GL_UNSIGNED_SHORT,
			depth_bits_per_pixel / 8,
			GL_DYNAMIC_DRAW);
		if (NULL == p_dspmgr->gl_depth_buffers[index]) {
			LOG_ERROR("failed to create screen");
			return ERR_FAILED_CREATE;
		}
	}

	/* create the canvas, which is simply a square in 3D space upon which the
	 * pixel buffers are drawn */
	p_dspmgr->canvas = gl_canvas_create();
	if (NULL == p_dspmgr->canvas) {
		LOG_ERROR("failed to create canvas");
		return ERR_FAILED_CREATE;
	}
	
	/* load customized shader programs which handle depth cutoff and masking
	 * of video data by depth data */
	p_dspmgr->vertex_shader = gl_shader_load_file(
		GL_VERTEX_SHADER,
		vertex_shader_path);
	if (p_dspmgr->vertex_shader == 0) {
		LOG_ERROR("failed to load vertex shader");
		return -1;
	}
	p_dspmgr->fragment_shader = gl_shader_load_file(
		GL_FRAGMENT_SHADER,
		fragment_shader_path);
	if (p_dspmgr->fragment_shader == 0) {
		LOG_ERROR("failed to load fragment shader");
		return -1;
	}
	p_dspmgr->shader_program = gl_shader_program(
		p_dspmgr->vertex_shader,
		p_dspmgr->fragment_shader);
	if (p_dspmgr->shader_program == 0) {
		LOG_ERROR("failed to link shader program");
		return -1;
	}

	/* get handles into shader program fro variables */
	p_dspmgr->uniforms.count
		= glGetUniformLocation(p_dspmgr->shader_program, "count");
	p_dspmgr->uniforms.depth_horizontal_pixel_stride = glGetUniformLocation(
		p_dspmgr->shader_program, 
		"depth_horizontal_pixel_stride");
	p_dspmgr->uniforms.depth_vertical_pixel_stride = glGetUniformLocation(
		p_dspmgr->shader_program, 
		"depth_vertical_pixel_stride");
	p_dspmgr->attributes.position
		= glGetAttribLocation(p_dspmgr->shader_program, "position");
	/* check that these uniforms retrieved successfully */
	if ((p_dspmgr->uniforms.count < 0) || 
		(p_dspmgr->attributes.position < 0) ||
		(p_dspmgr->uniforms.depth_vertical_pixel_stride < 0) ||
		(p_dspmgr->uniforms.depth_horizontal_pixel_stride < 0)) 
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
		p_dspmgr->uniforms.video_textures[index]
			= glGetUniformLocation(p_dspmgr->shader_program, uniform_string);
		if (sprintf(uniform_string, "depth_texture_%02i", (int)index) < 0) {
			LOG_ERROR("failed to create string for uniform");
			return ERR_FAILED_CREATE;
		}
		p_dspmgr->uniforms.depth_textures[index]
			= glGetUniformLocation(p_dspmgr->shader_program, uniform_string);
		if (sprintf(uniform_string, "depth_cutoff_%02i", (int)index) < 0) {
			LOG_ERROR("failed to create string for uniform");
			return ERR_FAILED_CREATE;
		}
		p_dspmgr->uniforms.depth_cutoffs[index]
			= glGetUniformLocation(p_dspmgr->shader_program, uniform_string);
		if ((p_dspmgr->uniforms.video_textures[index] < 0) || 
			(p_dspmgr->uniforms.depth_textures[index] < 0) ||
			(p_dspmgr->uniforms.depth_cutoffs[index] < 0))
		{
			LOG_ERROR("failed to get uniform/attribute locations from shaders");
			return ERR_FAILED_CREATE;
		}	
	}

	return NO_ERROR;
}

status_t display_manager_prepare_frame(display_manager_handle_t handle) {
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	/* prepare graphics card to accept new image data */
	glUseProgram(handle->shader_program);
	/* TODO: mucking around with alpha to see what happens :) */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
	/* TODO: if things don't work.  it might be because of this */
	//glUniform1i(handle->uniforms.count, (int)layer_count);
	handle->layer_count = 0;

	return NO_ERROR;
}

status_t display_manager_set_frame_layer(
	display_manager_handle_t handle,
	float depth_cutoff,
	void* video_data,
	void* depth_data)
{
	size_t index = 0;
	if ((NULL == handle) || (NULL == video_data) || (NULL == depth_data)) {
		return ERR_NULL_POINTER;
	}

	if (handle->layer_count >= TEXTURE_CAPACITY) {
		return ERR_EXCEED_ERROR;
	}
	index = handle->layer_count;
	handle->layer_count++;

	glUniform1f(handle->uniforms.depth_cutoffs[index], depth_cutoff);
		
	gl_pixel_buffer_set_data(
		handle->gl_video_buffers[index],
		video_data);
	gl_pixel_buffer_display(
		handle->gl_video_buffers[index],
		handle->uniforms.video_textures[index]);

	gl_pixel_buffer_set_data(
		handle->gl_depth_buffers[index],
		depth_data);
	/* use GL_RED_SCALE because the depth data only has 1 number 
	 * per pixel */
     /* TODO */
	glPixelTransferf(GL_RED_SCALE, handle->depth_scale);
    glPixelTransferf(GL_ALPHA_SCALE, handle->depth_scale);
	gl_pixel_buffer_display(
		handle->gl_depth_buffers[index],
		handle->uniforms.depth_textures[index]);
	glPixelTransferf(GL_RED_SCALE, 1.0f);
    glPixelTransferf(GL_ALPHA_SCALE, 1.0f);

	return NO_ERROR;
}

status_t display_manager_display_frame(display_manager_handle_t handle)
{
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	glUniform1i(handle->uniforms.count, handle->layer_count);
	/* set pixel strides */
	glUniform1f(
		handle->uniforms.depth_vertical_pixel_stride,
		handle->depth_vertical_pixel_stride);
	GL_LOG_ERROR();

	glUniform1f(
		handle->uniforms.depth_horizontal_pixel_stride,
		handle->depth_horizontal_pixel_stride);
	GL_LOG_ERROR();
	/* set canvas to draw */
	gl_canvas_display(
		handle->canvas,
		handle->attributes.position);
	
	/* apply new buffer */
	glutSwapBuffers();

	return NO_ERROR;
}

