#ifndef _test_image_h_
#define _test_image_h_

#include <OpenGL/gl.h>

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct _test_image* test_image;

	typedef enum {
		TI_Black,
		TI_White,
		TI_4VerticalBars,
		TI_4HorizontalBars
	} TestImagePattern;
	
	test_image test_image_create(
		int width,
		int height,
		GLenum format,
		TestImagePattern pattern);

	GLenum test_image_format(const test_image image);
	int test_image_width(const test_image image);
	int test_image_height(const test_image image);
	const float* test_image_data(const test_image image);
	void test_image_destroy(test_image* pImage);

#ifdef __cplusplus
}
#endif
#endif
