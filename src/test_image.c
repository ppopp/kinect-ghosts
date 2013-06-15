#include "test_image.h"
#include "gl_pixel.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef struct _test_image {
	int width;
	int height;
	GLenum format;
	float* data;
} _test_image;


static void white_pixel(
	_test_image* image,
	int xPos, 
	int yPos, 
	const gl_pixel_config* config);

static void black_pixel(
	_test_image* image,
	int xPos, 
	int yPos, 
	const gl_pixel_config* config);

static size_t _pixel_position(
	_test_image* image,
	int xPos,
	int yPos,
	const gl_pixel_config* config);


typedef struct _pattern_config {
	TestImagePattern pattern;
	void (*draw)(_test_image* image, const gl_pixel_config* config);
} _pattern_config;

typedef void (*color_func)(
	_test_image* image,
	int xPos, 
	int yPos, 
	const gl_pixel_config* config);

static void draw_color(
	_test_image* image,
	const gl_pixel_config* config,
	color_func colorFunc);

static void draw_white(
	_test_image* image, 
	const gl_pixel_config* config);
static void draw_black(
	_test_image* image, 
	const gl_pixel_config* config);
static void draw_4_vertical_bars(
	_test_image* image, 
	const gl_pixel_config* config);
static void draw_4_horizontal_bars(
	_test_image* image, 
	const gl_pixel_config* config);

static const _pattern_config _pattern_configs[] = {
	{
		TI_Black,
		&draw_black
	},
	{
		TI_White,
		&draw_white
	},
	{
		TI_4VerticalBars,
		&draw_4_vertical_bars
	},
	{
		TI_4HorizontalBars,
		&draw_4_horizontal_bars
	}
};

static const size_t _num_pattern_configs = 
	sizeof(_pattern_configs) / sizeof(_pattern_config);

test_image test_image_create(
	int width,
	int height,
	GLenum format,
	TestImagePattern pattern) 
{
	const gl_pixel_config* pixelConfig = NULL;
	const _pattern_config* patternConfig = NULL;
	test_image image = NULL;
	int dataSize = 0;
	size_t i = 0;

	pixelConfig = gl_pixel_get_config(format);
	if (NULL == pixelConfig) {
		return image;
	}

	for (i = 0; i < _num_pattern_configs; i++) {
		if (_pattern_configs[i].pattern == pattern) {
			patternConfig = &_pattern_configs[i];
			break;
		}
	}
	if (NULL == patternConfig) {
		return image;
	}
	
	image = (_test_image*)malloc(sizeof(_test_image));
	if (image == NULL) {
		return image;
	}

	image->width = width;
	image->height = height;
	image->format = format;
	image->data = NULL;

	dataSize = width * height * sizeof(float) * pixelConfig->colorsPerPixel;
	if (dataSize > 0) {
		image->data = (float*)malloc(dataSize);
		memset(image->data, 0, dataSize);
	}

	patternConfig->draw(image, pixelConfig);
	return image;
}

GLenum test_image_format(const test_image image) {
	if (image) {
		return image->format;
	}
	return -1;
}

int test_image_width(const test_image image) {
	if (image) {
		return image->width;
	}
	return -1;
}

int test_image_height(const test_image image) {
	if (image) {
		return image->height;
	}
	return -1;
}

const float* test_image_data(const test_image image) {
	if (image) {
		return image->data;
	}
	return NULL;
}

void test_image_destroy(test_image* pImage) {
	if (pImage) {
		if (*pImage) {
			if ((*pImage)->data) {
				free((*pImage)->data);
			}
			free(*pImage);
		}
		*pImage = NULL;
	}
}

void draw_color(
	_test_image* image,
	const gl_pixel_config* config,
	color_func colorFunc)
{
	int xPos = 0;
	int yPos = 0;
	for (xPos = 0; xPos < image->width; xPos++) {
		for (yPos = 0; yPos < image->height; yPos++) {
			colorFunc(image, xPos, yPos, config);
		}
	}
}
void draw_white(_test_image* image, const gl_pixel_config* config) {
	draw_color(image, config, &white_pixel);
}

void draw_black(_test_image* image, const gl_pixel_config* config) {
	draw_color(image, config, &black_pixel);
}

void draw_4_vertical_bars(_test_image* image, const gl_pixel_config* config) {
	int delta = 0;
	int cntr = 0;
	int drawWhite = 1;
	int yPos = 0;
	int xPos = 0;

	if (image == NULL) {
		return;
	}
	if (image->data == NULL) {
		return;
	}

	delta = (int)ceil((float)image->width / 7.0f);
	for (yPos = 0; yPos < image->height; yPos++) {
		cntr = 0;
		drawWhite = 1;
		for (xPos = 0; xPos < image->width; xPos++) {
			if (cntr >= delta) {
				cntr = 0;
				drawWhite = !drawWhite;
			}
			
			if (drawWhite) {
				white_pixel(image, xPos, yPos, config);
			}
			else {
				black_pixel(image, xPos, yPos, config);
			}
			cntr += 1;
		}
	}
}
void draw_4_horizontal_bars(_test_image* image, const gl_pixel_config* config) {
	int delta = 0;
	int cntr = 0;
	int drawWhite = 1;
	int xPos = 0;
	int yPos = 0;

	if (image == NULL) {
		return;
	}
	if (image->data == NULL) {
		return;
	}

	delta = (int)ceil((float)image->height / 7.0f);
	for (xPos = 0; xPos < image->width; xPos++) {
		cntr = 0;
		drawWhite = 1;
		for (yPos = 0; yPos < image->height; yPos++) {
			if (cntr >= delta) {
				cntr = 0;
				drawWhite = !drawWhite;
			}
			
			if (drawWhite) {
				white_pixel(image, xPos, yPos, config);
			}
			else {
				black_pixel(image, xPos, yPos, config);
			}
			cntr += 1;
		}
	}
}

void white_pixel(
	_test_image* image,
	int xPos, 
	int yPos, 
	const gl_pixel_config* config)
{
	size_t pos = 0;
	int i = 0;

	if (NULL == image) {
		return;
	}
	if (NULL == image->data) {
		return;
	}
	if (NULL == config) {
		return;
	}

	pos = _pixel_position(image, xPos, yPos, config);
	for (i = 0; i < config->colorsPerPixel; i++) {
		image->data[pos + i] = 1.0f;
	}
}

void black_pixel(
	_test_image* image,
	int xPos, 
	int yPos, 
	const gl_pixel_config* config)
{
	size_t pos = 0;
	int i = 0;

	if (NULL == image) {
		return;
	}
	if (NULL == image->data) {
		return;
	}
	if (NULL == config) {
		return;
	}

	pos = _pixel_position(image, xPos, yPos, config);
	for (i = 0; i < config->colorsPerPixel; i++) {
		if (i == config->alphaPosition) {
			image->data[pos + i] = 1.0f;
		}
		else {
			image->data[pos + i] = 0.0f;
		}
	}
}

size_t _pixel_position(
	_test_image* image,
	int xPos,
	int yPos,
	const gl_pixel_config* config)
{
	if (config == NULL) {
		return 0;
	}
	if (image == NULL) {
		return 0;
	}
	return gl_pixel_position(image->width, image->height, xPos, yPos, config);
}

