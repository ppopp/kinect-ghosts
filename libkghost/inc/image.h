#ifndef _image_h_
#define _image_h_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif
	int image_rgb8_to_g8(
		unsigned char* dst,
		const unsigned char* src,
		size_t numPixels);

	int image_rgb8_to_gf(
		float* dst,
		const unsigned char* src,
		size_t numPixels);

	int image_diff_uchar(
		short* dest, 
		const unsigned char* src1, 
		const unsigned char* src2,
		size_t len);

#ifdef __cplusplus
}
#endif

#endif
