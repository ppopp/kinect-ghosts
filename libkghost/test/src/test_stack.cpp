#include "gtest/gtest.h"
#include "stack.h"

TEST(StackTest, CreateRelease) {
	status_t err = NO_ERROR;
	stack_handle_t stack = NULL;

	err = stack_create(8, &stack);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_TRUE(NULL != stack);

	err = stack_release(stack);
	ASSERT_EQ(NO_ERROR, err);
}

TEST(StackTest, AccessorsManipulators) {
	status_t err = NO_ERROR;
	stack_handle_t stack = NULL;
	double values[8] = {1, 2, 3, 4, 5, 6, 7, 8};
	int flag = 0;
	size_t count = 0;
	double* value = NULL;

	err = stack_create(8, &stack);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_TRUE(NULL != stack);

	err = stack_empty(stack, &flag);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_EQ((int)1, flag);

	err = stack_full(stack, &flag);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_EQ((int)0, flag);

	err = stack_count(stack, &count);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_EQ((size_t)0, count);

	for (size_t i = 0; i < 8; i++) {
		err = stack_push(stack, (void*)&(values[i]));
		ASSERT_EQ(NO_ERROR, err);

		err = stack_count(stack, &count);
		ASSERT_EQ(NO_ERROR, err);
		ASSERT_EQ((size_t)i + 1, count);
	}

	err = stack_push(stack, (void*)&values[0]);
	ASSERT_NE(NO_ERROR, err);

	err = stack_empty(stack, &flag);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_EQ((int)0, flag);

	err = stack_full(stack, &flag);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_EQ((int)1, flag);

	for (size_t i = 8; i > 0; i--) {
		err = stack_pop(stack, (void**)&value);
		ASSERT_EQ(NO_ERROR, err);
		ASSERT_EQ(values[i - 1], *value);

		err = stack_count(stack, &count);
		ASSERT_EQ(NO_ERROR, err);
		ASSERT_EQ((size_t)i - 1, count);
	}

	err = stack_empty(stack, &flag);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_EQ((int)1, flag);

	err = stack_full(stack, &flag);
	ASSERT_EQ(NO_ERROR, err);
	ASSERT_EQ((int)0, flag);

	err = stack_pop(stack, (void**)&value);
	ASSERT_NE(NO_ERROR, err);

	err = stack_release(stack);
	ASSERT_EQ(NO_ERROR, err);
}
