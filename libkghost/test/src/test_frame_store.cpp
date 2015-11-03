#include "gtest/gtest.h"
#include "frame_store.h"
#include "libfreenect/libfreenect.h"


static const size_t _width = 4;
static const size_t _height = 4;
static const size_t _video_bytes = 3;
static const size_t _depth_bytes = 2;
static const size_t _video_bits = 8 * _video_bytes;
static const size_t _depth_bits = 8 * _depth_bytes;
static const size_t _video_size = _width * _height * _video_bytes;
static const size_t _depth_size = _width * _height * _depth_bytes;

/*
static freenect_frame_mode _video_mode = {
	0,
	FREENECT_RESOLUTION_MEDIUM,
	{FREENECT_VIDEO_RGB},
	_video_size,
	_width,
	_height,
	_video_bits,
	0,
	30,
	1
};

static freenect_frame_mode _depth_mode = {
	0,
	FREENECT_RESOLUTION_MEDIUM,
	{FREENECT_DEPTH_REGISTERED},
	_depth_size,
	_width,
	_height,
	_depth_bits,
	0,
	30,
	1
};
*/

static unsigned char _video_frame[_video_size] = {1};
static unsigned char _depth_frame[_depth_size] = {2};

TEST(TestFrameStore, CreateRelease) {
	status_t status = NO_ERROR;
	frame_store_handle_t frame_store = NULL;

	status = frame_store_create(
		_video_size,
		_depth_size,
		1024*1024*512, /* half a gigabyte */
		&frame_store);

	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != frame_store);

	frame_store_release(frame_store);
}

TEST(TestFrameStore, RemoveFrame) {
	status_t status = NO_ERROR;
	frame_store_handle_t frame_store = NULL;
	timestamp_t timestamp = 0;
	void* data = NULL;
	size_t count = 0;

	status = frame_store_create(
		_video_size,
		_depth_size,
		1024*1024*512, /* half a gigabyte */
		&frame_store);

	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != frame_store);

	/* create clip with three frames */
	status = frame_store_mark_clip_boundary(frame_store);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	timestamp += 1;
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	timestamp += 1;
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);

	/* create 2nd clip with 3 frames */
	status = frame_store_mark_clip_boundary(frame_store);
	ASSERT_EQ(NO_ERROR, status);
	timestamp = 0;

	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	timestamp += 1;
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	timestamp += 1;
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp);
	ASSERT_EQ(NO_ERROR, status);

	/* mark end of second clip */
	status = frame_store_mark_clip_boundary(frame_store);
	ASSERT_EQ(NO_ERROR, status);

	/* check clip and frame counts make sense */
	status = frame_store_clip_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(count, (size_t)2);
	status = frame_store_frame_count(frame_store, 0, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(count, (size_t)3);
	status = frame_store_frame_count(frame_store, 1, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(count, (size_t)3);

	/* remove 2 frames from first clip */
	status = frame_store_remove_frame(frame_store, 0, 1);
	ASSERT_EQ(NO_ERROR, status);
	status = frame_store_remove_frame(frame_store, 0, 0);
	ASSERT_EQ(NO_ERROR, status);
	/* check size of first clip */
	status = frame_store_frame_count(frame_store, 0, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(count, (size_t)1);

	/* check timestmap of first clip frame (should be last frame) */
	timestamp = 0;
	status = frame_store_video_frame(
		frame_store,
		0,
		0,
		(void**)&data,
		&timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(data != NULL);
	ASSERT_EQ(timestamp, (timestamp_t)2);


	/* check size of 2nd clip unaltered */
	status = frame_store_frame_count(frame_store, 1, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(count, (size_t)3);

	/* try to remove invalid frame index */
	status = frame_store_remove_frame(frame_store, 0, 1);
	ASSERT_NE(NO_ERROR, status);
	/* try to remove invalid clip index */
	status = frame_store_remove_frame(frame_store, 2, 0);
	ASSERT_NE(NO_ERROR, status);
	
	frame_store_release(frame_store);
}

TEST(TestFrameStore, Capture) {
	status_t status = NO_ERROR;
	frame_store_handle_t frame_store = NULL;
	size_t count = -1;
	void* data = NULL;
	timestamp_t timestamp = 0;
	timestamp_t expected_timestamp = 0;

	status = frame_store_create(
		_video_size,
		_depth_size,
		1024*1024*512, /* half a gigabyte */
		&frame_store);

	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != frame_store);


	status = frame_store_clip_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)0, count);

	status = frame_store_frame_count(frame_store, 0, &count);
	ASSERT_NE(NO_ERROR, status);

	status = frame_store_mark_clip_boundary(frame_store);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_clip_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)0, count);


	/* matching timestamps */
	status = frame_store_capture_video(frame_store, (void*)_video_frame, 1);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 1);
	ASSERT_EQ(NO_ERROR, status);


	/* mismatching timestamps */
	status = frame_store_capture_video(frame_store, (void*)_video_frame, 2);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 3);
	ASSERT_EQ(NO_ERROR, status);


	/* matching timestamps */
	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 4);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_capture_video(frame_store, (void*)_video_frame, 4);
	ASSERT_EQ(NO_ERROR, status);


	/* new clip */
	status = frame_store_mark_clip_boundary(frame_store);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_frame_count(frame_store, 0, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	status = frame_store_clip_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)1, count);

	/* matching timestamps */
	status = frame_store_capture_video(frame_store, (void*)_video_frame, 5);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 5);
	ASSERT_EQ(NO_ERROR, status);


	/* mismatching timestamps */
	status = frame_store_capture_video(frame_store, (void*)_video_frame, 6);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 7);
	ASSERT_EQ(NO_ERROR, status);


	/* matching timestamps */
	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 8);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_capture_video(frame_store, (void*)_video_frame, 8);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_mark_clip_boundary(frame_store);
	ASSERT_EQ(NO_ERROR, status);

	status = frame_store_frame_count(frame_store, 1, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	/* TODO: test getting data back */
	status = frame_store_clip_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	status = frame_store_frame_count(
		frame_store,
		0,
		&count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	status = frame_store_video_frame(frame_store, 0, 0, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)1);
	ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

	status = frame_store_depth_frame(frame_store, 0, 0, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)1);
	ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

	status = frame_store_video_frame(frame_store, 0, 1, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)4);
	ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

	status = frame_store_depth_frame(frame_store, 0, 1, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)4);
	ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));


	status = frame_store_frame_count(
		frame_store,
		1,
		&count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	status = frame_store_video_frame(frame_store, 1, 0, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)5);
	ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

	status = frame_store_depth_frame(frame_store, 1, 0, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)5);
	ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

	status = frame_store_video_frame(frame_store, 1, 1, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)8);
	ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

	status = frame_store_depth_frame(frame_store, 1, 1, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)8);
	ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

	/* capture a bunch */
	timestamp = 1;
	for (size_t i = 0; i < 200; i++) {
		/* matching timestamps */
		status = frame_store_capture_depth(frame_store, (void*)_depth_frame, timestamp);
		ASSERT_EQ(NO_ERROR, status);
		
		status = frame_store_capture_video(frame_store, (void*)_video_frame, timestamp);
		ASSERT_EQ(NO_ERROR, status);
		
		/* 100 Hz frame rate */
		usleep(10000);

		timestamp++;
	}

	status = frame_store_mark_clip_boundary(frame_store);
	ASSERT_EQ(NO_ERROR, status);
	
	timestamp = 0;
	expected_timestamp = 1;
	for (size_t i = 0; i < 200; i++) {
		status = frame_store_video_frame(frame_store, 2, i, &data, &timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, expected_timestamp);
		ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));
		
		status = frame_store_depth_frame(frame_store, 2, i, &data, &timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, expected_timestamp);
		ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

		expected_timestamp++;
	}
	frame_store_release(frame_store);
}

