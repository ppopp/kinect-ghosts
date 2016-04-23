#ifndef _gl_canvas_h_
#define _gl_canvas_h_

#include <OpenGL/gl.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	/** \addtogroup gl
	 * @{
	 */
	typedef struct _gl_canvas* gl_canvas;

	gl_canvas gl_canvas_create();
	void gl_canvas_destroy(gl_canvas* canvas);
	int gl_canvas_display(
		gl_canvas canvas,
		GLint positionAttribute);

	/** @} */
#ifdef __cplusplus
}
#endif
#endif

