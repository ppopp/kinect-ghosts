#include "gtest/gtest.h"
#include "frame_store.h"
#include "libfreenect/libfreenect.h"


static const size_t _width = 4;
static const size_t _height = 4;
static const size_t _video_bytes = 3;
static const size_t _depth_bytes = 2;
static const size_t _video_size = _width * _height * _video_bytes;
static const size_t _depth_size = _width * _height * _depth_bytes;
static const size_t _meta_size = sizeof(double);

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
		_meta_size,
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
	frame_id_t frame_id = invalid_frame_id;
	frame_id_t to_remove_1 = invalid_frame_id;
	frame_id_t to_remove_2 = invalid_frame_id;
	frame_id_t to_check = invalid_frame_id;
	double meta = 1.0;

	status = frame_store_create(
		_video_size,
		_depth_size,
		_meta_size,
		1024*1024*512, /* half a gigabyte */
		&frame_store);

	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != frame_store);

	/* store three frames */
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_meta(
		frame_store, 
		(void*)&meta,
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);
	to_check = frame_id;

	timestamp += 1;
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_meta(
		frame_store, 
		(void*)&meta,
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);
	to_remove_1 = frame_id;

	timestamp += 1;
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_meta(
		frame_store, 
		(void*)&meta,
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);

	/* restart timestamp and capture more frames */
	timestamp = 0;
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_meta(
		frame_store, 
		(void*)&meta,
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);
	timestamp += 1;
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_meta(
		frame_store, 
		(void*)&meta,
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);
	to_remove_2 = frame_id;
	timestamp += 1;
	status = frame_store_capture_video(
		frame_store, 
		(void*)_video_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_depth(
		frame_store, 
		(void*)_depth_frame, 
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);
	status = frame_store_capture_meta(
		frame_store, 
		(void*)&meta,
		timestamp,
		&frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);


	/* check frame counts make sense */
	status = frame_store_frame_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(count, (size_t)6);

	/* remove 2 frames */
	status = frame_store_remove_frame(frame_store, to_remove_1);
	ASSERT_EQ(NO_ERROR, status);
	status = frame_store_remove_frame(frame_store, to_remove_2);
	ASSERT_EQ(NO_ERROR, status);
	/* check size of first clip */
	status = frame_store_frame_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(count, (size_t)4);

	/* check timestmap of first clip frame (should be last frame) */
	timestamp = 0;
	status = frame_store_video_frame(
		frame_store,
		to_check,
		(void**)&data,
		&timestamp);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(data != NULL);
	ASSERT_EQ(timestamp, (timestamp_t)0);


	/* try to remove invalid frame index */
	status = frame_store_remove_frame(frame_store, invalid_frame_id);
	ASSERT_NE(NO_ERROR, status);
	
	frame_store_release(frame_store);
}

TEST(TestFrameStore, Capture) {
	status_t status = NO_ERROR;
	frame_store_handle_t frame_store = NULL;
	size_t count = -1;
	void* data = NULL;
	timestamp_t timestamp = 0;
	frame_id_t frame_id = invalid_frame_id;
	double meta = 1.0;
	timestamp_t valid_timestamps[300];
	frame_id_t valid_frames[300];
	size_t valid_frame_count = 0;
	size_t i = 0;

	status = frame_store_create(
		_video_size,
		_depth_size,
		_meta_size,
		1024*1024*512, /* half a gigabyte */
		&frame_store);

	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(NULL != frame_store);

	status = frame_store_frame_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);

	/* matching timestamps */
	status = frame_store_capture_video(frame_store, (void*)_video_frame, 1, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 1, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_meta(frame_store, (void*)&meta, 1, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);
	valid_frames[valid_frame_count] = frame_id;
	valid_timestamps[valid_frame_count] = 1;
	valid_frame_count++;
	

	/* mismatching timestamps */
	status = frame_store_capture_video(frame_store, (void*)_video_frame, 2, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 3, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_meta(frame_store, (void*)&meta, 1, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);


	/* matching timestamps */
	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 4, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_video(frame_store, (void*)_video_frame, 4, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_meta(frame_store, (void*)&meta, 4, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);
	valid_frames[valid_frame_count] = frame_id;
	valid_timestamps[valid_frame_count] = 4;
	valid_frame_count++;

	/* check count */
	status = frame_store_frame_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)2, count);

	/* matching timestamps */
	status = frame_store_capture_video(frame_store, (void*)_video_frame, 5, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 5, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_meta(frame_store, (void*)&meta, 5, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);
	valid_frames[valid_frame_count] = frame_id;
	valid_timestamps[valid_frame_count] = 5;
	valid_frame_count++;

	/* mismatching timestamps */
	status = frame_store_capture_video(frame_store, (void*)_video_frame, 6, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 7, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_meta(frame_store, (void*)&meta, 7, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	/* matching timestamps */
	status = frame_store_capture_depth(frame_store, (void*)_depth_frame, 8, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_video(frame_store, (void*)_video_frame, 8, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(frame_id, invalid_frame_id);

	status = frame_store_capture_meta(frame_store, (void*)&meta, 8, &frame_id);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_NE(frame_id, invalid_frame_id);
	valid_frames[valid_frame_count] = frame_id;
	valid_timestamps[valid_frame_count] = 8;
	valid_frame_count++;

	status = frame_store_frame_count(frame_store, &count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)4, count);

	/* test getting data back */
	status = frame_store_frame_count(
		frame_store,
		&count);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)4, count);

	for (i = 0; i < valid_frame_count; i++) {
		status = frame_store_video_frame(
			frame_store, 
			valid_frames[i], 
			&data, 
			&timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, valid_timestamps[i]);
		ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

		status = frame_store_depth_frame(
			frame_store, 
			valid_frames[i], 
			&data, 
			&timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, valid_timestamps[i]);
		ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

		status = frame_store_meta_frame(
			frame_store, 
			valid_frames[i], 
			&data, 
			&timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, valid_timestamps[i]);
		ASSERT_EQ(0, memcmp(data, &meta, _meta_size));
	}


	/* capture a bunch */
	timestamp = 1;
	valid_frame_count = 0;
	for (i = 0; i < 200; i++) {
		/* matching timestamps */
		status = frame_store_capture_depth(
			frame_store, 
			(void*)_depth_frame, 
			timestamp, 
			&frame_id);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_EQ(frame_id, invalid_frame_id);
		
		status = frame_store_capture_video(
			frame_store, 
			(void*)_video_frame, 
			timestamp,
			&frame_id);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_EQ(frame_id, invalid_frame_id);
		
		status = frame_store_capture_meta(
			frame_store, 
			(void*)&meta, 
			timestamp, 
			&frame_id);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_NE(frame_id, invalid_frame_id);
		valid_frames[valid_frame_count] = frame_id;
		valid_timestamps[valid_frame_count] = timestamp;
        valid_frame_count++;

		/* 100 Hz frame rate */
		usleep(10000);

		timestamp++;
	}

	
	timestamp = 0;
	for (i = 0; i < 200; i++) {
		status = frame_store_video_frame(
			frame_store, 
			valid_frames[i], 
			&data, 
			&timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, valid_timestamps[i]);
		ASSERT_EQ(0, memcmp(data, _video_frame, _video_size));

		status = frame_store_depth_frame(
			frame_store, 
			valid_frames[i], 
			&data, 
			&timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, valid_timestamps[i]);
		ASSERT_EQ(0, memcmp(data, _depth_frame, _depth_size));

		status = frame_store_meta_frame(
			frame_store, 
			valid_frames[i], 
			&data, 
			&timestamp);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_TRUE(NULL != data);
		ASSERT_EQ(timestamp, valid_timestamps[i]);
		ASSERT_EQ(0, memcmp(data, &meta, _meta_size));
	}
	frame_store_release(frame_store);
}

