#include "gtest/gtest.h"
#include "timer.h"

#include <unistd.h>

TEST(TestTimer, CreateRelease) {
	status_t status = NO_ERROR;
	timer_handle_t handle = NULL;

	status = timer_create(&handle);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_TRUE(handle != NULL);

	timer_release(handle);
}

TEST(TestTimer, Smoke) {
	status_t status = NO_ERROR;
	timer_handle_t handle = NULL;
	double time = 0.0;
	double last = 0.0;

	status = timer_create(&handle);
	ASSERT_EQ(status, NO_ERROR);
	ASSERT_TRUE(handle != NULL);

	for (size_t i = 0; i < 10; i++) {
		usleep(1000);
		status = timer_current(handle, &time);
		ASSERT_EQ(status, NO_ERROR);
		ASSERT_GT(time - last, 0.001);
		ASSERT_LT(time - last, 0.002);
		ASSERT_GT(time, 0.0);
		last = time;
	}
	status = timer_reset(handle);
	ASSERT_EQ(status, NO_ERROR);
	status = timer_current(handle, &time);
	EXPECT_LT(time, last);
	ASSERT_GT(time, 0.0);

	last = time;
	for (size_t i = 0; i < 10; i++) {
		usleep(1000);
		status = timer_current(handle, &time);
		ASSERT_EQ(status, NO_ERROR);
		ASSERT_GT(time - last, 0.001);
		ASSERT_GT(time, 0.0);
		last = time;
	}

	timer_release(handle);
}

