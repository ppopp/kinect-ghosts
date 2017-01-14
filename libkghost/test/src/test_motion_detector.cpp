#include "gtest/gtest.h"
#include "motion_detector.h"

TEST(MotionDetector, CreateRelease) {
	status_t status = NO_ERROR;
	motion_detector_handle_t handle = NULL;

	status = motion_detector_create(
		2, 1, FALSE, &handle);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_TRUE(NULL != handle);
	motion_detector_release(handle);
}

TEST(MotionDetector, Smoke) {
	/* we'll do a 4 x 4 grid of 16 pixels, 2 bytes per pixel */
	status_t status = NO_ERROR;
	motion_detector_handle_t handle = NULL;
	size_t width = 4;
	size_t height = 4;
	double motion = 0.0;
	double presence = 0.0;
	short frame_a[16] = {
		1000, 1000, 1000, 1000, 
		0, 0, 0, 0,
		20000, 20000, 20000, 20000,
		0, 0, 0, 0};
	short frame_b[16] = {
		0, 0, 0, 0,
		20000, 20000, 20000, 20000,
		20000, 20000, 0, 0,
		0, 0, 0, 0};

	status = motion_detector_create(
		sizeof(short), 
		width * height, 
		FALSE,
		&handle);
	ASSERT_TRUE(NULL != handle);
	ASSERT_EQ(status, NO_ERROR);

	motion = -1.0;
    /* TODO: logic is wrong on presence & motion.  This assumes
     * that the comparison was made for pixels > cutoff, but it's actually
     * the other way around because we're dealing with the depth buffer */
	status = motion_detector_detect(
		handle,
		(void*)frame_a,
		1001,
		&motion,
		&presence);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_DOUBLE_EQ(motion, 0.0);
	ASSERT_DOUBLE_EQ(presence, 0.75);
	status = motion_detector_detect(
		handle,
		(void*)frame_b,
		1001,
		&motion,
		&presence);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_DOUBLE_EQ(motion, 0.375);
	ASSERT_DOUBLE_EQ(presence, 0.625);
	status = motion_detector_detect(
		handle,
		(void*)frame_b,
		1001,
		&motion,
		&presence);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_DOUBLE_EQ(motion, 0.0);
	ASSERT_DOUBLE_EQ(presence, 0.625);
	status = motion_detector_detect(
		handle,
		(void*)frame_a,
		999,
		&motion,
		&presence);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_DOUBLE_EQ(motion, 0.625);
	ASSERT_DOUBLE_EQ(presence, 0.5);

	status = motion_detector_reset(handle);
	ASSERT_EQ(status, NO_ERROR);
	status = motion_detector_detect(
		handle,
		(void*)frame_b,
		1001,
		&motion,
		&presence);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_DOUBLE_EQ(motion, 0.0);
	ASSERT_DOUBLE_EQ(presence, 0.625);
	status = motion_detector_detect(
		handle,
		(void*)frame_a,
		1001,
		&motion,
		&presence);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_DOUBLE_EQ(motion, 0.375);
	ASSERT_DOUBLE_EQ(presence, 0.75);


	motion_detector_release(handle);
}

