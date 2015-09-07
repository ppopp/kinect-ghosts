#include "gl_screen.h"
#include "gl_texture.h"
#include "gl_error.h"

#include "log.h"

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _gl_screen {
	struct {
		GLuint element;
		GLuint vertex;
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

} _gl_screen;


static const GLfloat _vertex_buffer_data[] = { 
	-1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, -1.0f,
	1.0f, 1.0f
};

static const GLushort _element_buffer_data[] = {0, 1, 2, 3};

gl_screen gl_screen_create(
	size_t width, 
	size_t height, 
	GLenum target,
	GLint internalFormat,
	GLenum externalFormat,
	GLenum pixelType,
	size_t pixelByteSize) 
{
	_gl_screen* screen = NULL;

	if ((width < 1) || (height < 1)) {
		LOG_WARNING("invalid screen width/height");
		return screen;
	}

	screen = (_gl_screen*)malloc(sizeof(_gl_screen));
	screen->width = width;
	screen->height = height;
	screen->target = target;
	screen->internalFormat = internalFormat;
	screen->externalFormat = externalFormat;
	screen->pixelType = pixelType;
	screen->dataSize = width * height * pixelByteSize;
	screen->textureUnit = gl_texture_claim();
	if (screen->textureUnit == NULL) {
		free(screen);
		LOG_WARNING("failed to claim texture enum for gl_screen");
		return NULL;
	}

	// create pixel buffer 
	glGenBuffers(3, (GLuint*)&(screen->buffers));
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, screen->buffers.pixel);
	glBufferData(
		GL_PIXEL_UNPACK_BUFFER,
		screen->dataSize,
		NULL,
		GL_DYNAMIC_DRAW);
	
	// create/bind element buffer for indexing vertices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screen->buffers.element);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		sizeof(_element_buffer_data), 
		_element_buffer_data,
		GL_STATIC_DRAW);

	// create array buffer for vertices
    glBindBuffer(GL_ARRAY_BUFFER, screen->buffers.vertex);
	glBufferData(
		GL_ARRAY_BUFFER, 
		sizeof(_vertex_buffer_data), 
		_vertex_buffer_data,
		GL_STATIC_DRAW);

	// create texture
	glGenTextures(1, &(screen->texture));
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	return screen;
}


int gl_screen_set_data(gl_screen screen, const void* data) {
	void* glData = NULL;
	if (NULL == data) {
		LOG_WARNING("null pointer");
		return -1;
	}


	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, screen->buffers.pixel);
	glBufferData(
				 GL_PIXEL_UNPACK_BUFFER,
				 screen->dataSize,
				 NULL,
				 GL_DYNAMIC_DRAW);
	glData = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

	if (NULL == glData) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		LOG_WARNING("failed to map buffer");
		return -1;
	}

	memcpy(glData, data, screen->dataSize);

	if (glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER) != GL_TRUE) {
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
		LOG_WARNING("failed to unmap buffer");
		return -1;
	}
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	return 0;
}

void gl_screen_destroy(gl_screen* screen) {
	if (screen) {
		gl_texture_release((*screen)->textureUnit);
		glDeleteBuffers(3, (GLuint*)&((*screen)->buffers));
		glDeleteTextures(1, &((*screen)->texture));
		free(*screen);
		*screen = NULL;
	}
}

int gl_screen_display(
	gl_screen screen,
	GLint positionAttribute,
	GLint textureUniform)
{
	glBindTexture(GL_TEXTURE_2D, screen->texture);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, screen->buffers.pixel);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(
		screen->target,   
		0,                 
		screen->internalFormat,         // internal format
		screen->width,             
		screen->height,             
		0,                  // border
		screen->externalFormat,             
		screen->pixelType,  
		0
	);
	GL_LOG_ERROR();
	
	glActiveTexture(screen->textureUnit->textureEnum);
	glUniform1i(textureUniform, screen->textureUnit->textureIndex);
	
	glBindBuffer(GL_ARRAY_BUFFER, screen->buffers.vertex);
	
	glVertexAttribPointer(
		positionAttribute,                /* attribute */
		2,                                /* size */
		GL_FLOAT,                         /* type */
		GL_FALSE,                         /* normalized? */
		sizeof(GLfloat) * 2,              /* stride */
		(void*)0                          /* array buffer offset */
	);

	glEnableVertexAttribArray(positionAttribute);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, screen->buffers.element);
	glDrawElements(
		GL_TRIANGLE_STRIP,  /* mode */
		4,                  /* count */
		GL_UNSIGNED_SHORT,  /* type */
		(void*)0            /* element array buffer offset */
	);

	glDisableVertexAttribArray(positionAttribute);
	return 0;
}

