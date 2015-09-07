//#include <GL/glew.h>
#include "gl_shader.h"

#include "log.h"

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>

#include <stdio.h>
#include <stdlib.h>

/*
static void gl_shader_info_log(
	GLuint object, 
	PFNGLGETSHADERIVPROC glGet__iv, 
	PFNGLGETSHADERINFOLOGPROC glGet__InfoLog);
*/

GLuint gl_shader_load_file(GLenum type, const char* path) {
	GLint length = 0;
	size_t readSize = 0;
	GLuint shader = 0;
	GLchar *source = NULL;
	FILE *pFile = 0;

//	glewInit();
	pFile = fopen(path, "rb");
	if (!pFile) {
		LOG_ERROR("failed to open shader file");
		return 0;
	}

	if (fseek(pFile, 0, SEEK_END)) {
		LOG_ERROR("error seeking within shader file");
		fclose(pFile);
		return 0;
	}

	length = ftell(pFile);
	if (length < 1) {
		LOG_ERROR("invalid shader file");
		fclose(pFile);
		return 0;
	}

	source = (GLchar*)malloc(length);
	if (!source) {
		LOG_ERROR("malloc error");
		fclose(pFile);
		return 0;
	}

	rewind(pFile);
	readSize = fread(source, 1, length, pFile);
	fclose(pFile);
	if (readSize != (size_t)length) {
		LOG_ERROR("error reading shader file");
		free(source);
		return 0;
	}

	shader = gl_shader_load_source(type, source, length);
	free(source);
	return shader;
}

GLuint gl_shader_load_source(GLenum type, const GLchar* source, GLint length) {
	GLuint shader = 0;
	GLint shader_ok = 0;

	if (NULL == source) {
		LOG_ERROR("null shader source");
		return 0;
	}

	shader = glCreateShader(type);
	glShaderSource(shader, 1, (const GLchar**)&source, &length);
	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
	if (!shader_ok) {
		LOG_ERROR("failed to compile shader");
		//gl_shader_info_log(shader, glGetShaderiv, glGetShaderInfoLog);
		glDeleteShader(shader);
		return 0;
	}
	return shader;
}

/*
void gl_shader_info_log(
	GLuint object, 
	PFNGLGETSHADERIVPROC glGet__iv, 
	PFNGLGETSHADERINFOLOGPROC glGet__InfoLog)
{
	GLint log_length = 0;
	char *log = NULL; 

	glGet__iv(object, GL_INFO_LOG_LENGTH, &log_length);
	log = (char*)malloc(log_length);
	glGet__InfoLog(object, log_length, NULL, log);
	fprintf(stderr, "%s", log);
	free(log);
}
*/

GLuint gl_shader_program(GLuint vertexShader, GLuint fragmentShader) {
	GLint program_ok;
	GLuint program = 0;

	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
	if (!program_ok) {
		LOG_ERROR("failed to linke shader program");
		//gl_shader_info_log(program, glGetProgramiv, glGetProgramInfoLog);
		glDeleteProgram(program);
		return 0;
	}
	return program;
}

