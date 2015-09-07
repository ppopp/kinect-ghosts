#include "median_filter.h"
#include "common.h"
#include "log.h"
#include "ring.h"
#include "vector.h"

#include <stdlib.h>
#include <string.h>

/* enough to hold 1 hour of frames at 30 frames per second */
//static const size_t _max_frames = 60 * 60 * 30;

/*
static void print_matrix_int64(const char* data, size_t element_size, size_t rows, size_t columns, size_t channels) {
	size_t row = 0;
	size_t column = 0;
    size_t channel = 0;
	size_t element = 0;
	size_t pos = 0;
	size_t copy_offset = sizeof(size_t) - element_size;

	if (element_size > sizeof(size_t)) {
		printf("cannot print");
		return;
	}

    for (channel = 0; channel < channels; channel++) {
        pos = channel * element_size;
        for (row = 0; row < rows; row++) {
            for (column = 0; column < columns; column++) {
                char* p_element = (char*)&element;
                memcpy((void*)p_element, (const void*)&data[pos], element_size);
                printf("%zu\t", element);
                pos += element_size * channels;
			
            }
            printf("\n");
        }
        printf("\n");
    }
}
*/

typedef struct median_filter_s {
	median_filter_shape_t filter_shape;
	size_t filter_size;
	size_t sort_buffer_median_pos;
	median_filter_input_spec_t input_spec;
	size_t input_size;
	comparison_function compare;
	char* sort_buffer;
	size_t sort_buffer_size;
	ring_handle_t ring;
	vector_handle_t frames;
} median_filter_t;

static status_t _load_buffer(
	char* buffer,
	median_filter_input_spec_t* p_input_spec,
	median_filter_shape_t* p_shape,
	ring_handle_t ring, 
	size_t x, 
	size_t y, 
	size_t channel);
	
status_t median_filter_create(
	median_filter_input_spec_t input_spec,
	median_filter_shape_t filter_shape,
	comparison_function compare,
	median_filter_handle_t* p_handle)
{
	status_t status = NO_ERROR;
	median_filter_t* p_median_filter = NULL;

	/* check inputs */
	if ((NULL == p_handle) || (NULL == compare)) {
		LOG_ERROR("null pointer");
		return ERR_NULL_POINTER;
	}

	if ((filter_shape.x < 1) || 
		(filter_shape.y < 1) ||
		(filter_shape.z < 1) ||
		((filter_shape.x % 2) == 0) ||
		((filter_shape.y % 2) == 0) ||
		((filter_shape.z % 2) == 0))
	{
		LOG_ERROR("invalid filter shape");
		return ERR_INVALID_ARGUMENT;
	}

	if ((input_spec.element_size < 1) ||
		(input_spec.channel_count < 1) ||
		(input_spec.width < 1) ||
		(input_spec.height < 1)) 
	{
		LOG_ERROR("invalid input spec");
		return ERR_INVALID_ARGUMENT;
	}

	/* allocate structure */
	p_median_filter = malloc(sizeof(median_filter_t));
	if (NULL == p_median_filter) {
		LOG_ERROR("failed allocation");
		return ERR_FAILED_ALLOC;
	}
	memset(p_median_filter, 0, sizeof(median_filter_t));

	/* initialize structure */
	p_median_filter->filter_shape = filter_shape;
	p_median_filter->filter_size = filter_shape.x * filter_shape.y * filter_shape.z;
	p_median_filter->input_spec = input_spec;
	p_median_filter->input_size = 
		input_spec.width * 
		input_spec.height * 
		input_spec.element_size * 
		input_spec.channel_count;
	p_median_filter->compare = compare;
	p_median_filter->sort_buffer_size = 
		filter_shape.x * 
		filter_shape.y * 
		filter_shape.z * 
		input_spec.element_size;
	p_median_filter->sort_buffer_median_pos = 
		(p_median_filter->filter_size / 2) * input_spec.element_size;

	p_median_filter->sort_buffer = (char*)malloc(p_median_filter->sort_buffer_size);
	if (NULL == p_median_filter->sort_buffer) {
		median_filter_release(p_median_filter);
		LOG_ERROR("failed alloc");
		return ERR_FAILED_ALLOC;
	}

	status = ring_create(
		filter_shape.z, 
		p_median_filter->input_size,
		&(p_median_filter->ring));
	if (status != NO_ERROR) {
		median_filter_release(p_median_filter);
		LOG_ERROR("failed create");
		return ERR_FAILED_CREATE;
	}

	status = vector_create(1024, sizeof(void*), &(p_median_filter->frames));
	if (status != NO_ERROR) {
		median_filter_release(p_median_filter);
		LOG_ERROR("failed create");
		return ERR_FAILED_CREATE;
	}

	*p_handle = p_median_filter;

	return NO_ERROR;
}

void median_filter_release(median_filter_handle_t handle) {
	if (NULL == handle) {
		return;
	}

	ring_release(handle->ring);
	vector_release(handle->frames);
	free(handle->sort_buffer);
	free(handle);
}

status_t median_filter_append(median_filter_handle_t handle, void* data) {
	status_t status = NO_ERROR;

	if ((NULL == handle) || (NULL == data)) {
		LOG_ERROR("null pointer");
		return ERR_NULL_POINTER;
	}

	/* copies data pointer; doesn't copy data */
	status = vector_append(handle->frames, &data);
	if (status != NO_ERROR) {
		LOG_ERROR("failed to add frame");
	}
	return status;
}


status_t median_filter_apply(median_filter_handle_t handle) {
	status_t status = NO_ERROR;
	size_t frame_count = 0;
	size_t i = 0;
	size_t push_pos = 0;
	size_t out_channel = 0;
	size_t out_x = 0;
	size_t out_y = 0;
	size_t out_z = 0;
	size_t start = 0;
	size_t end = 0;
	size_t half_z_medfilt = 0;
	size_t output_pixel_stride = 0;
	void** frames = NULL;
    char** data = NULL;
	char* out = NULL;

	if (NULL == handle) {
		LOG_ERROR("null handle");
		return ERR_NULL_POINTER;
	}

	/* get pushed frames */
	status = vector_count(handle->frames, &frame_count);
	if (NO_ERROR != status) {
		return status;
	}
	if (frame_count < handle->filter_shape.z) {
		return NO_ERROR;
	}
	status = vector_array(handle->frames, (void**)&frames);
	if (NO_ERROR != status) {
		return status;
	}

	half_z_medfilt = handle->filter_shape.z / 2;
	start = half_z_medfilt;
	end = frame_count - half_z_medfilt;
    /*
	output_pixel_stride = 
		handle->input_spec.element_size * 
		handle->input_spec.channel_count;
     */
    output_pixel_stride = handle->input_spec.element_size;

	/* TODO: some weird pointer stuff going on.  should probably check
	 * what is actually going on */

	/* initialize ring buffer by pushing in bunch of copies of the first frame */
	status = ring_clear(handle->ring);
	if (NO_ERROR != status) {
		return status;
	}
	for (i = 0; i < half_z_medfilt; i++) {
		status = ring_push(handle->ring, frames[0]);
		if (NO_ERROR != status) {
			return status;
		}
	}
	/* then add some more frames until we have enough to calculate our first output */
	for (i = 0; i < (half_z_medfilt + 1); i++) {
		status = ring_push(handle->ring, frames[i]);
		if (NO_ERROR != status) {
			return status;
		}
	}

	push_pos = half_z_medfilt + 1;
	while (out_z < frame_count) {
		status = vector_element(handle->frames, out_z, (void**)&data);
        out = *data;
		if (NO_ERROR != status) {
			return status;
		}
		/*
		printf("input frame\n");
		print_matrix_int64(
		   *data,
		   handle->input_spec.element_size,
		   handle->input_spec.height,
		   handle->input_spec.width,
		   handle->input_spec.channel_count);
		   */
		/* TODO: assume row major memory layout */
		for (out_y = 0; out_y < handle->input_spec.height; out_y++) {
			for (out_x = 0; out_x < handle->input_spec.width; out_x++) {
				for (out_channel = 0; 
					out_channel < handle->input_spec.channel_count; 
					out_channel++) 
				{
                    
					/* copy data into sort buffer */
					status = _load_buffer(
						handle->sort_buffer,
						&(handle->input_spec),
						&(handle->filter_shape),
						handle->ring, 
						out_x, 
						out_y, 
						out_channel);


						
					/* sort data in buffer */
					qsort(
						handle->sort_buffer,
						handle->filter_size,
						handle->input_spec.element_size,
						handle->compare);
                    
					/*
					printf("z: %u, y: %u, x: %u, c: %u\n", out_z, out_y, out_x, out_channel);
                    print_matrix_int64(
                                       handle->sort_buffer,
                                       handle->input_spec.element_size,
                                       1,
                                       handle->filter_size,
                                       1);
									   */


					/* copy data to output */
					memcpy(
						out,
						&handle->sort_buffer[handle->sort_buffer_median_pos],
						handle->input_spec.element_size);

					/* TODO: assume row major memory layout */
                    out += output_pixel_stride;

				}
			}
		}
		/*
		printf("new output\n");
		print_matrix_int64(
			*data, 
			handle->input_spec.element_size, 
			handle->input_spec.height,
			handle->input_spec.width,
			handle->input_spec.channel_count);
			*/

		status = ring_push(handle->ring, frames[push_pos]);
		if (NO_ERROR != status) {
			return status;
		}

		out_z++;
		push_pos++;
		if (push_pos >= frame_count) {
			push_pos = frame_count - 1;
		}
	}

	return NO_ERROR;
}

status_t median_filter_clear(median_filter_handle_t handle) {
	status_t status = NO_ERROR;

	if (NULL == handle) {
		LOG_ERROR("null handle");
		return ERR_NULL_POINTER;
	}

	status = vector_clear(handle->frames);
	if (NO_ERROR != status) {
		return status;
	}
	status = ring_clear(handle->ring);
	return status;
}

status_t _load_buffer(
	char* buffer,
	median_filter_input_spec_t* p_input_spec,
	median_filter_shape_t* p_shape,
	ring_handle_t ring, 
	size_t x, 
	size_t y, 
	size_t channel)
{
	status_t status = NO_ERROR;
	char* data = NULL;
	char* p_row = NULL;
	char* p_buffer = buffer;
	int i = 0;
	int j = 0;
	int k = 0;
	int row_size = 
		p_input_spec->width * 
		p_input_spec->element_size * 
		p_input_spec->channel_count;
	int input_pixel_stride = 
		p_input_spec->element_size * 
		p_input_spec->channel_count;
	int input_channel_offset = p_input_spec->element_size * channel;
	int output_pixel_stride = p_input_spec->element_size;
	int y_start = y - p_shape->y / 2;
	int x_start = x - p_shape->x / 2;

	for (i = 0; i < p_shape->z; i++) {
		status = ring_element(ring, i, (void**)&data);
		if (NO_ERROR != status) {
			return status;
		}
		/* TODO: assuming data is row major */
		for (k = 0; k < p_shape->y; k++) {
			int row = y_start + k;
			if (row < 0) {
				/* if we're before the first row , 
				 * use the first row repeatedly */
				row = 0;
			}
			else if (row >= p_input_spec->height) {
				/* if we're before the first row , 
				 * use the first row repeatedly */
				row = p_input_spec->height - 1;
			}
			p_row = &data[row * row_size];

			for (j = 0; j < p_shape->x; j++) {
				int pos = (x_start + j) * input_pixel_stride + input_channel_offset;
				if (pos < 0) {
					/* if we're before the first column, 
					 * copy first pixel repeatedly */
					pos = input_channel_offset;
				}
				else if (pos >= row_size) {
					/* if we're past the last column
					 * copy last pixel repeatedly */
					pos = row_size - input_pixel_stride + input_channel_offset;
				}

				memcpy(
					p_buffer, 
					&p_row[pos],
					p_input_spec->element_size);
				/* move pointers */
				p_buffer += output_pixel_stride;
			}
		}
	}
	return NO_ERROR;
}

