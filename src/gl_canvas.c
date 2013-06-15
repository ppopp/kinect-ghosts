#include "gl_canvas.h"
#include "gl_texture.h"
#include "gl_error.h"

#include "log.h"

#include <OpenGL/gl.h>
#include <OpenGL/glu.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _gl_canvas {
	struct {
		GLuint element;
		GLuint vertex;
	} buffers;
} _gl_canvas;


static const GLfloat _vertex_buffer_data[] = { 
	-1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, -1.0f,
	1.0f, 1.0f
};

static const GLushort _element_buffer_data[] = {0, 1, 2, 3};

gl_canvas gl_canvas_create() {
	_gl_canvas* canvas = NULL;

	canvas = (_gl_canvas*)malloc(sizeof(_gl_canvas));

	glGenBuffers(2, (GLuint*)&(canvas->buffers));
	// create/bind element buffer for indexing vertices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, canvas->buffers.element);
	glBufferData(
		GL_ELEMENT_ARRAY_BUFFER, 
		sizeof(_element_buffer_data), 
		_element_buffer_data,
		GL_STATIC_DRAW);

	// create array buffer for vertices
    glBindBuffer(GL_ARRAY_BUFFER, canvas->buffers.vertex);
	glBufferData(
		GL_ARRAY_BUFFER, 
		sizeof(_vertex_buffer_data), 
		_vertex_buffer_data,
		GL_STATIC_DRAW);

	return canvas;
}


void gl_canvas_destroy(gl_canvas* canvas) {
	if (canvas) {
		glDeleteBuffers(2, (GLuint*)&((*canvas)->buffers));
		free(*canvas);
		*canvas = NULL;
	}
}

int gl_canvas_display(gl_canvas canvas, GLint positionAttribute) {
	glBindBuffer(GL_ARRAY_BUFFER, canvas->buffers.vertex);
	
	glVertexAttribPointer(
		positionAttribute,                /* attribute */
		2,                                /* size */
		GL_FLOAT,                         /* type */
		GL_FALSE,                         /* normalized? */
		sizeof(GLfloat) * 2,              /* stride */
		(void*)0                          /* array buffer offset */
	);

	glEnableVertexAttribArray(positionAttribute);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, canvas->buffers.element);
	glDrawElements(
		GL_TRIANGLE_STRIP,  /* mode */
		4,                  /* count */
		GL_UNSIGNED_SHORT,  /* type */
		(void*)0            /* element array buffer offset */
	);

	glDisableVertexAttribArray(positionAttribute);
	return 0;
}

