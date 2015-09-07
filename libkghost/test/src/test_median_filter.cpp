#include "gtest/gtest.h"
#include "median_filter.h"

#include <iostream>

using namespace std;

template<class _type>
int compare(const void* a, const void* b) {
	const _type* first = (const _type*)a;
	const _type* second = (const _type*)b;
	return (*first) - (*second);
}

TEST(MedianFilter, Smoke) {
	median_filter_shape_t shape = {3, 3, 3};
	const size_t frames = 8;
	const size_t height = 6;
	const size_t width = 10;
	const size_t channels = 2;
	short** video = NULL;
	median_filter_input_spec_t input_spec = {
		sizeof(short), 	// element size
		channels, 				// channel count
		width, 			// width
		height 				// height
	};
	median_filter_handle_t filter = NULL;
	status_t status = NO_ERROR;

	/* create */
	status = median_filter_create(
		input_spec,
		shape,
		&compare<short>,
		&filter);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_TRUE(filter != NULL);


	/* allocate frames and assign values */
	video = new short*[frames];
	memset(video, 0, sizeof(short*) * frames);
	for (size_t f = 0; f < frames; f++) {
		video[f] = new short[width * height * channels];
		size_t pos = 0;
		for (size_t h = 0; h < height; h++) {
			for (size_t w = 0; w < width; w++) {
				for (size_t c = 0; c < channels; c++) {
					video[f][pos] = (f + w + h) * (c + 1);
					pos++;
				}
			}
		}
		status = median_filter_append(filter, (void*)video[f]);
		ASSERT_EQ(status, NO_ERROR);
	}
	/* values are (frame + row + column) * (channel + 1)
	frame: 0, channel 0:
		0 1 2 3 4 5 6
		1 2 3 4 5 6 7
		2 3 4 5 6 7 8
		3 4 5 6 7 8 9
		4 5 6 7 8 9 10
		...
	frame: 0 channel 1:
		0 2 4 6 8 10 12
		2 4 6 ...
	frame: 1 channel: 0
		1 2 3 4 5 6 7
		2 3 4 5 6 7 8
		3 4 5 6 7 8 9
		4 5 6 7 8 9 10
		5 6 7 8 9 10 11
		...
	*/

	status = median_filter_apply(filter);
	ASSERT_EQ(status, NO_ERROR);

	/* TODO: test edge values */

	for (size_t f = shape.z / 2; f < (frames - shape.z / 2); f++) {
		size_t pos = 0;
		for (size_t h = shape.y / 2; h < (height - shape.y / 2); h++) {
			for (size_t w = shape.x / 2 ; w < (width - shape.x / 2); w++) {
				for (size_t c = 0; c < channels; c++) {
					//cout << "f: " << f << " h: " << h << " w: " << w << " c: " << c << endl;
					pos = c + w * channels + h * width * channels;
					ASSERT_EQ(video[f][pos], (f + w + h) * (c + 1));
					pos++;
				}
			}
		}
	}

	status = median_filter_clear(filter);
	ASSERT_EQ(status, NO_ERROR);

	for (size_t f = 0; f < frames; f++) {
		size_t pos = 0;
		for (size_t h = 0; h < height; h++) {
			for (size_t w = 0; w < width; w++) {
				for (size_t c = 0; c < channels; c++) {
					video[f][pos] = (f + w + h) * (c + 2);
					pos++;
				}
			}
		}
		status = median_filter_append(filter, (void*)video[f]);
		ASSERT_EQ(status, NO_ERROR);
	}
	status = median_filter_apply(filter);
	ASSERT_EQ(status, NO_ERROR);

	/* TODO: test edge values */

	for (size_t f = shape.z / 2; f < (frames - shape.z / 2); f++) {
		size_t pos = 0;
		for (size_t h = shape.y / 2; h < (height - shape.y / 2); h++) {
			for (size_t w = shape.x / 2 ; w < (width - shape.x / 2); w++) {
				for (size_t c = 0; c < channels; c++) {
					//cout << "f: " << f << " h: " << h << " w: " << w << " c: " << c << endl;
					pos = c + w * channels + h * width * channels;
					ASSERT_EQ(video[f][pos], (f + w + h) * (c + 2));
					pos++;
				}
			}
		}
	}

	/* release */
	median_filter_release(filter);

	for (size_t f = 0; f < frames; f++) {
		delete[] video[f];
		video[f] = NULL;
	}
	delete[] video;
}

