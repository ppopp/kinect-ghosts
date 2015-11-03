#ifndef _gl_error_h_
#define _gl_error_h_

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include "log.h"

#define GL_LOG_ERROR() {\
	GLenum _gl_error_error = glGetError();\
	if (_gl_error_error != GL_NO_ERROR) {\
		LOG_ERROR("gl error: %s", (const char*)gluErrorString(_gl_error_error));\
	}\
}

#endif
