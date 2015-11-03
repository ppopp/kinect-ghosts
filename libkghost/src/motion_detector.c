#include "motion_detector.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>

#define POP_COUNT __builtin_popcount
#define MODULO_32 (0x0000001F)
#define REMAINDER_32 (0xFFFFFFE0)

typedef unsigned int (*mask_function)(
	void* data,
	unsigned int pixel_count,
	int cutoff,
	unsigned int* mask,
	unsigned int mask_length);

typedef struct motion_detector_s {
	unsigned int* mask_a;
	unsigned int* mask_b;
	bool_t existing_frame;
	bool_t mask_flag;
	size_t pixel_size;
	size_t pixel_count;
	size_t mask_length;
	size_t mask_bytes;
	bool_t skip_invalid;
	mask_function make_mask;
} motion_detector_t;


static unsigned int _create_mask_uint16(
	void* data,
	unsigned int pixel_count,
	int cutoff,
	unsigned int* mask,
	unsigned int mask_length);
static unsigned int _create_mask_uint16_si(
	void* data,
	unsigned int pixel_count,
	int cutoff,
	unsigned int* mask,
	unsigned int mask_length);


status_t motion_detector_create(
	size_t pixel_size,
	size_t pixel_count,
	bool_t skip_invalid,
	motion_detector_handle_t* p_handle)
{
	motion_detector_t* p_md = NULL;

	if (32 != (sizeof(unsigned int) * CHAR_BIT)) {
		return ERR_UNSUPPORTED_ARCHITECTURE;
	}

	if ((pixel_size < 1) || (pixel_count < 1)) {
		return ERR_INVALID_ARGUMENT;
	}

	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}

	p_md = malloc(sizeof(motion_detector_t));
	if (NULL == p_md) {
		return ERR_FAILED_ALLOC;
	}
	memset(p_md, 0, sizeof(motion_detector_t));

	p_md->pixel_size = pixel_size;
	p_md->pixel_count = pixel_count;
	p_md->existing_frame = FALSE;
	p_md->skip_invalid = skip_invalid;
	if (skip_invalid == TRUE) {
		switch (pixel_size) {
			case 2:
				/* only handling 16 bit depth pixels for now */
				p_md->make_mask = &_create_mask_uint16_si;
				break;
			default:
				motion_detector_release(p_md);
				return ERR_INVALID_ARGUMENT;
		}
	}
	else {
		switch (pixel_size) {
			case 2:
				/* only handling 16 bit depth pixels for now */
				p_md->make_mask = &_create_mask_uint16;
				break;
			default:
				motion_detector_release(p_md);
				return ERR_INVALID_ARGUMENT;
		}
	}
	p_md->mask_flag = TRUE;
	p_md->mask_length = pixel_count / (sizeof(unsigned int) * CHAR_BIT) + 1;
	p_md->mask_bytes = p_md->mask_length * sizeof(unsigned int);
	p_md->mask_a = malloc(p_md->mask_bytes);
	if (NULL == p_md->mask_a) {
		motion_detector_release(p_md);
		return ERR_FAILED_ALLOC;
	}
	memset(p_md->mask_a, 0, p_md->mask_bytes);

	p_md->mask_b = malloc(p_md->mask_bytes);
	if (NULL == p_md->mask_b) {
		motion_detector_release(p_md);
		return ERR_FAILED_ALLOC;
	}
	memset(p_md->mask_b, 0, p_md->mask_bytes);
	
	*p_handle = p_md;

	return NO_ERROR;
}

void motion_detector_release(motion_detector_handle_t handle) {
	if (NULL == handle) {
		return;
	}
	free(handle->mask_a);
	handle->mask_a = NULL;
	free(handle->mask_b);
	handle->mask_b = NULL;	
	free(handle);
}

status_t motion_detector_reset(motion_detector_handle_t handle) {
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	memset(handle->mask_a, 0, handle->mask_bytes);
	memset(handle->mask_b, 0, handle->mask_bytes);
	handle->existing_frame = FALSE;
	handle->mask_flag = TRUE;

	return NO_ERROR;
}

status_t motion_detector_detect(
	motion_detector_handle_t handle,
	void* depth, 
	int cutoff, 
	double* p_motion, 
	double* p_presence) 
{
	unsigned int* mask = NULL;
	size_t bit_count = 0;
	size_t index = 0;
	unsigned int valid_pixel_count = 0;

	if ((NULL == handle) ||
		(NULL == depth) || 
		(NULL == p_motion) || 
		(NULL == p_presence)) 
	{
		return ERR_NULL_POINTER;
	}

	/* toggle between "mask a" and "mask b" */
	if (TRUE == handle->mask_flag) {
		mask = handle->mask_a;
		handle->mask_flag = FALSE;
	}
	else {
		mask = handle->mask_b;
		handle->mask_flag = TRUE;
	}

	/* create mask from data */
	valid_pixel_count = handle->make_mask(
		depth,
		handle->pixel_count,
		cutoff,
		mask,
		handle->mask_length);

	/* presence is number of 1's in mask */
	for (index = 0; index < handle->mask_length; index++) {
		bit_count += POP_COUNT(mask[index]);
	}
	*p_presence = (double)bit_count / (double)valid_pixel_count;

	if (handle->existing_frame) {
		bit_count = 0;
		for (index = 0; index < handle->mask_length; index++) {
			/* sum of bitwise difference between masks */
			bit_count += POP_COUNT(handle->mask_a[index] ^ handle->mask_b[index]);
		}
		*p_motion = (double)bit_count / (double)valid_pixel_count;
	}
	else {
		*p_motion = 0.0;
	}

	handle->existing_frame = TRUE;
	return NO_ERROR;
}

unsigned int _create_mask_uint16(
	void* data,
	unsigned int pixel_count,
	int cutoff,
	unsigned int* mask,
	unsigned int mask_length)
{
	unsigned short* pixels = (unsigned short*)data;
	unsigned short cut = (unsigned short)cutoff;
	unsigned int index = 0;
    unsigned int i = 0;

	memset(mask, 0, mask_length * sizeof(unsigned int));

	/* TODO: this can probably be made faster by unrolling */
	for (index = 0; index < pixel_count; index++) {
		if (pixels[index] <= cut) {
            i = (REMAINDER_32 & index) >> 5;
			mask[i] |= (1 << (MODULO_32 & index));
		}
	}
	return pixel_count;
}

unsigned int _create_mask_uint16_si(
	void* data,
	unsigned int pixel_count,
	int cutoff,
	unsigned int* mask,
	unsigned int mask_length)
{
	unsigned short* pixels = (unsigned short*)data;
	unsigned short cut = (unsigned short)cutoff;
	unsigned int index = 0;
    unsigned int i = 0;
	unsigned int valid_count = pixel_count;

	memset(mask, 0, mask_length * sizeof(unsigned int));

	/* TODO: this can probably be made faster by unrolling */
	for (index = 0; index < pixel_count; index++) {
		if (pixels[index] == 0) {
			valid_count--;
		}
		else if (pixels[index] <= cut) {
            i = (REMAINDER_32 & index) >> 5;
			mask[i] |= (1 << (MODULO_32 & index));
		}
	}
	return valid_count;
}

