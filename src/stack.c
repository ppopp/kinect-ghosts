#include <stdio.h>
#include <stdlib.h>
#include "stack.h"       

typedef struct stack_s {
  stack_value_t *contents;
  size_t max_size;
  size_t top;
} stack_container_t;

status_t stack_create(size_t maxSize, stack_handle_t* p_handle) {
	stack_value_t* contents = NULL;
	stack_container_t* stack = NULL;

	stack = (stack_container_t*)malloc(sizeof(stack_container_t));
	if (NULL == stack) {
		return ERR_FAILED_ALLOC;
	}

	// Allocate a new array to hold the contents.
	contents = (stack_value_t*)malloc(sizeof(stack_value_t) * maxSize);
	if (NULL == contents) {
		return ERR_FAILED_ALLOC;
	}

	stack->contents = contents;
	stack->max_size = maxSize;
	stack->top = 0;  

	*p_handle = stack;
	return NO_ERROR;
}

status_t stack_release(stack_handle_t handle) {
	if (NULL == handle) {
		return NO_ERROR;
	}
	free(handle->contents);
	free(handle);
	return NO_ERROR;
}

status_t stack_push(stack_handle_t handle, stack_value_t value) {
	status_t err = NO_ERROR;
	int isFull = 0;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	err = stack_full(handle, &isFull);

	if (NO_ERROR != err) {
		return err;
	}
	else if (isFull) {
		return ERR_FULL;
	}

	handle->contents[handle->top] = value;
	handle->top++;

	return NO_ERROR;
}

status_t stack_pop(stack_handle_t handle, stack_value_t* p_value) {
	status_t err = NO_ERROR;
	int isEmpty = 0;

	err = stack_empty(handle, &isEmpty);
	if (NO_ERROR != err) {
		return err;
	}
	else if (isEmpty) {
		return ERR_EMPTY;
	}

	*p_value = handle->contents[handle->top - 1];
	handle->top--;

	return NO_ERROR;
}

status_t stack_empty(stack_handle_t handle, int* flag) {
	if ((NULL == handle) || (NULL == flag)) {
		return ERR_NULL_POINTER;
	}

	*flag = (handle->top == 0);

	return NO_ERROR;
}

status_t stack_full(stack_handle_t handle, int* flag) {
	if ((NULL == handle) || (NULL == flag)) {
		return ERR_NULL_POINTER;
	}

	*flag = (handle->top >= handle->max_size);

	return NO_ERROR;
}

status_t stack_count(stack_handle_t handle, size_t* count) {
	if ((NULL == handle) || (NULL == count)) {
		return ERR_NULL_POINTER;
	}

	*count = handle->top;
	return NO_ERROR;
}

