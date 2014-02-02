#include "gtest/gtest.h"
#include "vector.h"

TEST(VectorTest, CreateRelease) {
	status_t err = NO_ERROR;
	vector_handle_t vector = NULL;

	err = vector_create(1, 1, &vector);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(vector != NULL);
	vector_release(vector);
}

TEST(VectorTest, AccessorsManipulators) {
	status_t err = NO_ERROR;
	vector_handle_t vector = NULL;
	size_t count = 0;
	size_t capacity = 0;
	char element = 0;
	char* p_element = NULL;

	err = vector_create(1, 1, &vector);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(vector != NULL);

	err = vector_capacity(vector, &capacity);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_EQ(capacity, (size_t)1);


	err = vector_count(vector, &count);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_EQ(count, (size_t)0);

	err = vector_append(vector, &element);
	ASSERT_EQ(err, NO_ERROR);

	err = vector_count(vector, &count);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_EQ(count, (size_t)1);

	err = vector_pop(vector);
	ASSERT_EQ(err, NO_ERROR);

	err = vector_count(vector, &count);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_EQ(count, (size_t)0);

	for (element = 0; element < 10; element++) {
		err = vector_append(vector, &element);
		ASSERT_EQ(err, NO_ERROR);
	}

	err = vector_count(vector, &count);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_EQ(count, (size_t)10);

	err = vector_capacity(vector, &capacity);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_GT(capacity, (size_t)1);

	err = vector_element(vector, 5, (void**)&p_element);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(NULL != p_element);
	ASSERT_EQ(*p_element, (char)5);

	err = vector_remove(vector, 5);
	ASSERT_EQ(err, NO_ERROR);

	err = vector_element(vector, 5, (void**)&p_element);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(NULL != p_element);
	ASSERT_EQ(*p_element, (char)6);

	err = vector_count(vector, &count);
	while ((err == NO_ERROR) && (count > 4)) {
		err = vector_pop(vector);
		ASSERT_EQ(err, NO_ERROR);
		err = vector_count(vector, &count);
	}

	ASSERT_EQ(err, NO_ERROR);
	ASSERT_EQ(count, (size_t)4);

	err = vector_remove(vector, 6);
	ASSERT_NE(err, NO_ERROR);

	err = vector_array(vector, (void**)&p_element);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_TRUE(NULL != p_element);

	element = 0;
	while (element < 4) {
		ASSERT_EQ(*p_element, element);
		p_element++;
		element++;
	}

	err = vector_clear(vector);
	ASSERT_EQ(err, NO_ERROR);

	err = vector_count(vector, &count);
	ASSERT_EQ(err, NO_ERROR);
	ASSERT_EQ(count, (size_t)0);

	err = vector_pop(vector);
	ASSERT_NE(err, NO_ERROR);

	vector_release(vector);
}
