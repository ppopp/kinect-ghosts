#ifndef _gl_pixel_buffer_h_
#define _gl_pixel_buffer_h_

#include <OpenGL/gl.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct _gl_pixel_buffer* gl_pixel_buffer;

	gl_pixel_buffer gl_pixel_buffer_create(
		size_t width, 
		size_t height, 
		GLenum target,
		GLint internalFormat,
		GLenum externalFormat,
		GLenum pixelType,
		size_t pixelByteSize,
		GLenum usage);

	void gl_pixel_buffer_destroy(gl_pixel_buffer* pb);

	int gl_pixel_buffer_set_data(gl_pixel_buffer pb, const void* data);
	int gl_pixel_buffer_display(
		gl_pixel_buffer pb,
		GLint textureUniform);

#ifdef __cplusplus
}
#endif
#endif

