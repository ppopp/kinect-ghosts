#include "gl_pixel_buffer.h"
#include "gl_texture.h"
#include "gl_error.h"

#include "log.h"

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _gl_pixel_buffer {
	struct {
		GLuint pixel;
	} buffers;
	const texture_unit* textureUnit;
	GLuint texture;
	size_t width;
	size_t height;
	size_t dataSize;
	GLenum target;
	GLenum externalFormat;
	GLenum internalFormat;
	GLenum pixelType;
	GLenum usage;
} _gl_pixel_buffer;



gl_pixel_buffer gl_pixel_buffer_create(
	size_t width, 
	size_t height, 
	GLenum target,
	GLint internalFormat,
	GLenum externalFormat,
	GLenum pixelType,
	size_t pixelByteSize,
	GLenum usage) 
{
	_gl_pixel_buffer* pb = NULL;

	if ((width < 1) || (height < 1)) {
		LOG_WARNING("gl", NULL, "invalid pixel buffer width/height");
		return pb;
	}

	pb = (_gl_pixel_buffer*)malloc(sizeof(_gl_pixel_buffer));
	pb->width = width;
	pb->height = height;
	pb->target = target;
	pb->internalFormat = internalFormat;
	pb->externalFormat = externalFormat;
	pb->pixelType = pixelType;
	pb->usage = usage;

	pb->dataSize = width * height * pixelByteSize;
	pb->textureUnit = gl_texture_claim();
	if (pb->textureUnit == NULL) {
		free(pb);
		LOG_WARNING("gl", NULL, "failed to claim texture enum for gl_pixel_buffer");
		return NULL;
	}

	// create pixel buffer 
	glGenBuffers(1, (GLuint*)&(pb->buffers));
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb->buffers.pixel);
	glBufferData(
		GL_PIXEL_UNPACK_BUFFER,
		pb->dataSize,
		NULL,
		pb->usage);
	
	// create texture
	glGenTextures(1, &(pb->texture));
	glBindTexture(GL_TEXTURE_2D, pb->texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	return pb;
}


int gl_pixel_buffer_set_data(gl_pixel_buffer pb, const void* data) {
	void* glData = NULL;
	if (NULL == data) {
		LOG_WARNING("gl", NULL, "null pointer");
		return -1;
	}

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb->buffers.pixel);
	glBufferData(
		 GL_PIXEL_UNPACK_BUFFER,
		 pb->dataSize,
		 NULL,
		 pb->usage);
	glData = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

	if (NULL == glData) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		LOG_WARNING("gl", NULL, "failed to map buffer");
		return -1;
	}

	memcpy(glData, data, pb->dataSize);

	if (glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER) != GL_TRUE) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		LOG_WARNING("gl", NULL, "failed to unmap buffer");
		return -1;
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	return 0;
}

void gl_pixel_buffer_destroy(gl_pixel_buffer* pixel_buffer) {
	if (pixel_buffer) {
		gl_texture_release((*pixel_buffer)->textureUnit);
		glDeleteBuffers(1, (GLuint*)&((*pixel_buffer)->buffers));
		glDeleteTextures(1, &((*pixel_buffer)->texture));
		free(*pixel_buffer);
		*pixel_buffer = NULL;
	}
}

int gl_pixel_buffer_display(
	gl_pixel_buffer pb,
	GLint textureUniform)
{
	glActiveTexture(pb->textureUnit->textureEnum);
	glBindTexture(GL_TEXTURE_2D, pb->texture);
	glUniform1i(textureUniform, pb->textureUnit->textureIndex);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pb->buffers.pixel);
	
	glTexImage2D(
		pb->target,   
		0,                 
		pb->internalFormat,         // internal format
		pb->width,             
		pb->height,             
		0,                  // border
		pb->externalFormat,             
		pb->pixelType,  
		0
	);
	GL_LOG_ERROR();
	
	
	return 0;
}

