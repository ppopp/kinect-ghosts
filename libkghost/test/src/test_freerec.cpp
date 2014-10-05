#include "gtest/gtest.h"
#include "freerec.h"
#include "libfreenect/libfreenect.h"


static const size_t _width = 4;
static const size_t _height = 4;
static const size_t _video_bytes = 3;
static const size_t _depth_bytes = 2;
static const size_t _video_bits = 8 * _video_bytes;
static const size_t _depth_bits = 8 * _depth_bytes;
static const size_t _video_size = _width * _height * _video_bytes;
static const size_t _depth_size = _width * _height * _depth_bytes;

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

static unsigned char _video_frame[_video_size] = {1};
static unsigned char _depth_frame[_depth_size] = {2};

TEST(TestFreerec, CreateRelease) {
	status_t status = NO_ERROR;
	freerec_handle_t freerec = NULL;

	status = freerec_create(
		&_video_mode,
		&_depth_mode,
		1024*1024*512, /* half a gigabyte */
		&freerec);

	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != freerec);

	freerec_release(freerec);
}

TEST(TestFreerec, Capture) {
	status_t status = NO_ERROR;
	freerec_handle_t freerec = NULL;
	size_t count = -1;
	void* data = NULL;
	timestamp_t timestamp = 0;
	timestamp_t expected_timestamp = 0;

	status = freerec_create(
		&_video_mode,
		&_depth_mode,
		1024*1024*512, /* half a gigabyte */
		&freerec);

	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != freerec);


	status = freerec_clip_count(freerec, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)0, count);

	status = freerec_clip_frame_count(freerec, 0, &count);
	ASSERT_NE(NO_ERROR, status);

	status = freerec_action(freerec);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_clip_count(freerec, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)0, count);


	/* matching timestamps */
	status = freerec_capture_video(freerec, (void*)_video_frame, 1);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_capture_depth(freerec, (void*)_depth_frame, 1);
	ASSERT_EQ(NO_ERROR, status);


	/* mismatching timestamps */
	status = freerec_capture_video(freerec, (void*)_video_frame, 2);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_capture_depth(freerec, (void*)_depth_frame, 3);
	ASSERT_EQ(NO_ERROR, status);


	/* matching timestamps */
	status = freerec_capture_depth(freerec, (void*)_depth_frame, 4);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_capture_video(freerec, (void*)_video_frame, 4);
	ASSERT_EQ(NO_ERROR, status);


	/* new clip */
	status = freerec_action(freerec);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_clip_frame_count(freerec, 0, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	status = freerec_clip_count(freerec, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)1, count);

	/* matching timestamps */
	status = freerec_capture_video(freerec, (void*)_video_frame, 5);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_capture_depth(freerec, (void*)_depth_frame, 5);
	ASSERT_EQ(NO_ERROR, status);


	/* mismatching timestamps */
	status = freerec_capture_video(freerec, (void*)_video_frame, 6);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_capture_depth(freerec, (void*)_depth_frame, 7);
	ASSERT_EQ(NO_ERROR, status);


	/* matching timestamps */
	status = freerec_capture_depth(freerec, (void*)_depth_frame, 8);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_capture_video(freerec, (void*)_video_frame, 8);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_action(freerec);
	ASSERT_EQ(NO_ERROR, status);

	status = freerec_clip_frame_count(freerec, 1, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	/* TODO: test getting data back */
	status = freerec_clip_count(freerec, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	status = freerec_clip_frame_count(
		freerec,
		0,
		&count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	status = freerec_clip_video_frame(freerec, 0, 0, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)1);
	ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

	status = freerec_clip_depth_frame(freerec, 0, 0, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)1);
	ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

	status = freerec_clip_video_frame(freerec, 0, 1, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)4);
	ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

	status = freerec_clip_depth_frame(freerec, 0, 1, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)4);
	ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));


	status = freerec_clip_frame_count(
		freerec,
		1,
		&count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	status = freerec_clip_video_frame(freerec, 1, 0, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)5);
	ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

	status = freerec_clip_depth_frame(freerec, 1, 0, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)5);
	ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

	status = freerec_clip_video_frame(freerec, 1, 1, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)8);
	ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

	status = freerec_clip_depth_frame(freerec, 1, 1, &data, &timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != data);
	ASSERT_EQ(timestamp, (timestamp_t)8);
	ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

	/* capture a bunch */
	timestamp = 1;
	for (size_t i = 0; i < 200; i++) {
		/* matching timestamps */
		status = freerec_capture_depth(freerec, (void*)_depth_frame, timestamp);
		ASSERT_EQ(NO_ERROR, status);
		
		status = freerec_capture_video(freerec, (void*)_video_frame, timestamp);
		ASSERT_EQ(NO_ERROR, status);
		
		/* 100 Hz frame rate */
		usleep(10000);

		timestamp++;
	}

	status = freerec_action(freerec);
	ASSERT_EQ(NO_ERROR, status);
	
	timestamp = 0;
	expected_timestamp = 1;
	for (size_t i = 0; i < 200; i++) {
		status = freerec_clip_video_frame(freerec, 2, i, &data, &timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, expected_timestamp);
		ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));
		
		status = freerec_clip_depth_frame(freerec, 2, i, &data, &timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, expected_timestamp);
		ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

		expected_timestamp++;
	}
	freerec_release(freerec);
}

