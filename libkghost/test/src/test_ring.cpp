#include "gtest/gtest.h"
#include "common.h"
#include "ring.h"

typedef int element_t;

TEST(RingTest, CreateRelease) {
	status_t status = NO_ERROR;
	ring_handle_t ring = NULL;

	status = ring_create(1, 1, &ring);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(ring != NULL);
	ring_release(ring);
}

TEST(RingTest, RingBasic) {
	status_t      status      = NO_ERROR;
	ring_handle_t ring        = NULL;
	element_t     elements[4] = {0};
	element_t*    p_element   = NULL;
	size_t        value       = 0;
	size_t        index       = 0;

	status = ring_create(4, sizeof(element_t), &ring);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_TRUE(ring != NULL);

	status = ring_count(ring, &value);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)0, value);

	status = ring_capacity(ring, &value);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)4, value);

	/* test pushing values till full */
	for (index = 4; index > 0; index--) {
		elements[index - 1] = rand();
		status = ring_push(ring, (void*)&(elements[index - 1]));
		ASSERT_EQ(NO_ERROR, status);
	}

	for (index = 0; index < 4; index++) {
		status = ring_element(ring, index, (void**)&p_element);
		ASSERT_EQ(NO_ERROR, status);
		ASSERT_EQ(*p_element, elements[index]);
	}

	status = ring_count(ring, &value);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)4, value);

	/* test adding one extra */
	elements[3] = rand();
	status = ring_push(ring, (void*)&(elements[3]));
	ASSERT_EQ(NO_ERROR, status);

	status = ring_element(ring, 0, (void**)&p_element);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(*p_element, elements[3]);
	status = ring_element(ring, 1, (void**)&p_element);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(*p_element, elements[0]);
	status = ring_element(ring, 2, (void**)&p_element);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(*p_element, elements[1]);
	status = ring_element(ring, 3, (void**)&p_element);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ(*p_element, elements[2]);

	status = ring_count(ring, &value);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)4, value);

	/* test clearing */
	status = ring_clear(ring);
	ASSERT_EQ(NO_ERROR, status);

	status = ring_count(ring, &value);
	ASSERT_EQ(NO_ERROR, status);
	ASSERT_EQ((size_t)0, value);

	ring_release(ring);
}

