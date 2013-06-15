#ifndef _gl_pixel_h_
#define _gl_pixel_h_

#include <OpenGL/gl.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	extern const size_t gl_pixel_npos;

	typedef struct _gl_pixel_config {
		GLenum format;
		int colorsPerPixel;
		int alphaPosition;
	} gl_pixel_config;

	const gl_pixel_config* gl_pixel_get_config(GLenum format);

	size_t gl_pixel_position(
		int width,
		int height,
		int xPos,
		int yPos,
		const gl_pixel_config* config);

#ifdef __cplusplus
}
#endif

#endif
