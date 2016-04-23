#ifndef _gl_screen_h_
#define _gl_screen_h_

#include <OpenGL/gl.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	/** \addtogroup gl
	 * @{
	 */

	typedef struct _gl_screen* gl_screen;

	gl_screen gl_screen_create(
		size_t width, 
		size_t height, 
		GLenum target,
		GLint internalFormat,
		GLenum externalFormat,
		GLenum pixelType,
		size_t pixelByteSize);

	void gl_screen_destroy(gl_screen* screen);

	int gl_screen_set_data(gl_screen screen, const void* data);
	int gl_screen_display(
		gl_screen screen,
		GLint positionAttribute,
		GLint textureUniform);
	/** @} */

#ifdef __cplusplus
}
#endif
#endif

