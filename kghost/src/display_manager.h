#ifndef _display_manager_h_
#define _display_manager_h_

#include <stdlib.h>
#include <string.h>
#include <GLUT/glut.h>

#include "common.h"
#include "log.h"
#include "gl_shader.h"
#include "gl_canvas.h"
#include "gl_pixel_buffer.h"

/* TODO: can we hardcode shaders? */
/* Maximum number of textures available in shader.  This *must* match or be
 * less than the size of the texture arrays in glsl/fragment.frag
 */
#define TEXTURE_CAPACITY (8)


typedef struct _display_manager_s* display_manager_handle_t;


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
	display_manager_handle_t* p_handle);
void display_manager_destroy(display_manager_handle_t handle);

status_t display_manager_prepare_frame(display_manager_handle_t handle);
status_t display_manager_set_frame_layer(
	display_manager_handle_t handle,
	float depth_cutoff,
	void* video_data,
	void* depth_data);
status_t display_manager_display_frame(display_manager_handle_t handle);

#endif
