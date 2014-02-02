#include "gtest/gtest.h"
#include "memory_pool.h"
#include "unistd.h"

TEST(MemoryPoolTest, CreateRelease) {
	status_t err = NO_ERROR;
	memory_pool_handle_t pool = NULL;

	err = memory_pool_create(
		32,
		16,
		32,
		1024 * 1024,
		1000,
		&pool);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_TRUE(NULL != pool);

	err = memory_pool_release(pool);
	ASSERT_EQ(NO_ERROR, err);
}

TEST(MemoryPoolTest, ClaimUnclaim) {
	status_t err = NO_ERROR;
	memory_pool_handle_t pool = NULL;
	size_t counter = 0;
	void* data = NULL;
	void* first_data = NULL;

	err = memory_pool_create(
		1024,
		24,
		32,
		256 * 1024,
		10,
		&pool);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_TRUE(NULL != pool);

	while (NO_ERROR == err) {
		err = memory_pool_claim(pool, &data);
		ASSERT_TRUE(NULL != data);
		if (NULL == first_data) {
			first_data = data;
		}
		counter++;

		usleep(100);
	}

	ASSERT_NE(NO_ERROR, err);
	ASSERT_EQ(counter, (size_t)257);

	err = memory_pool_unclaim(pool, first_data);
	ASSERT_EQ(NO_ERROR, err);

	data = NULL;
	err = memory_pool_claim(pool, &data);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_TRUE(NULL != data);
	EXPECT_TRUE(data == first_data);

	err = memory_pool_release(pool);
	ASSERT_EQ(NO_ERROR, err);
}

