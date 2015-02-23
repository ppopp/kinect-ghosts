#include "gl_pixel.h"
#include "log.h"

const size_t gl_pixel_npos = -1;

static const gl_pixel_config _pixel_configs[] = {
	{GL_RED, 1, -1},
	{GL_RG, 2, -1},
	{GL_RGB, 3, -1},
	{GL_BGR, 3, -1},
	{GL_RGBA, 4, 3},
	{GL_BGRA, 4, 3}
};

static const size_t _num_pixel_configs = 
	sizeof(_pixel_configs) / sizeof(gl_pixel_config);

const gl_pixel_config* gl_pixel_get_config(GLenum format) {
	size_t i = 0;
	for (i = 0; i < _num_pixel_configs; i++) {
		if (format == _pixel_configs[i].format) {
			return &_pixel_configs[i];
		}
	}
	LOG_WARNING("failed to find pixel config");
	return NULL;
}

size_t gl_pixel_position(
	int width,
	int height,
	int xPos,
	int yPos,
	const gl_pixel_config* config)
{
	if (NULL == config) {
		LOG_WARNING("null pointer");
		return gl_pixel_npos;
	}
	return config->colorsPerPixel * (yPos * width + xPos);
}

