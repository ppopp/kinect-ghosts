#ifndef _gl_shader_h_
#define _gl_shader_h_

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>

#ifdef __cplusplus
extern "C" {
#endif

	GLuint gl_shader_load_file(GLenum type, const char* path);
	GLuint gl_shader_load_source(GLenum type, const char* source, int length);
	GLuint gl_shader_program(GLuint vertexShader, GLuint fragmentShader);


#ifdef __cplusplus
}
#endif

#endif
