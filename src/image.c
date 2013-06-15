#include "image.h"

int image_rgb8_to_g8(
	unsigned char* dst,
	const unsigned char* src,
	size_t numPixels)
{
	size_t srcPos = 0;
	size_t dstPos = 0;
	float tmp = 0.0f;
	if ((NULL == dst) || (NULL == src)) {
		return -1;
	}

	for (srcPos = 0, dstPos = 0; dstPos < numPixels; srcPos += 3, dstPos++) {
		tmp = 0.299f * src[srcPos] 
			+ 0.587f * src[srcPos + 1] 
			+ 0.114f * src[srcPos + 2];
		if (tmp > 1.0f) {
			dst[dstPos] = 255;
		}
		else if (tmp < 0.0f) {
			dst[dstPos] = 0;
		}
		else {
			dst[dstPos] = (unsigned char)(255.0f * tmp);
		}
	}
	return 0;
}

int image_rgb8_to_gf(float* dst, const unsigned char* src, size_t numPixels) {
	size_t srcPos = 0;
	size_t dstPos = 0;
	if ((NULL == dst) || (NULL == src)) {
		return -1;
	}

	for (srcPos = 0, dstPos = 0; dstPos < numPixels; srcPos += 3, dstPos++) {
		dst[dstPos] = 0.299f * src[srcPos] 
			+ 0.587f * src[srcPos + 1] 
			+ 0.114f * src[srcPos + 2];
		if (dst[dstPos] > 1.0f) {
			dst[dstPos] = 1.0f;
		}
		else if (dst[dstPos] < 0.0f) {
			dst[dstPos] = 0.0f;
		}
	}
	return 0;
}

int image_diff_uchar(
	short* dest, 
	const unsigned char* src1, 
	const unsigned char* src2,
	size_t len)
{
	size_t i = 0;
	if ((dest == NULL) || (src1 == NULL) || (src2 == NULL)) {
		return -1;
	}

	for (i = 0; i < len; i++) {
		dest[i] = src1[i] - src2[i];
	}

	return 0;
}

