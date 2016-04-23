#ifndef _gl_texture_h_
#define _gl_texture_h_

#include <OpenGL/gl.h>
#ifdef __cplusplus
extern "C" {
#endif
	/** \addtogroup gl
	 * @{
	 */

	typedef struct _texture_unit {
		int textureIndex;
		GLenum textureEnum;
	} texture_unit;

	const texture_unit* gl_texture_claim();
	void gl_texture_release(const texture_unit* tunit);

	/** @} */

#ifdef __cplusplus
}
#endif
#endif
