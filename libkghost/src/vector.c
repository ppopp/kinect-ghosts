#include "vector.h"
#include <stdlib.h>
#include <string.h>

#define DEFAULT_VECTOR_CAPACITY (32)


typedef struct vector_s {
	/* number of elements vector can hold with currently allocated memory */
	size_t capacity;
	/* size in bytes of a single vector element */
	size_t element_size;
	/* number of elements currently in vector */
	size_t count;
	/* point to contiguous data array */
	byte_t* array;
} vector_t;

static status_t _vector_adjust_capacity(
	vector_handle_t handle, 
	size_t new_capacity);

status_t vector_create(
	size_t initial_capacity,
	size_t element_size,
	vector_handle_t* p_handle)
{
	vector_t* p_vector = NULL;
	status_t err = NO_ERROR;

	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}
	if (element_size < 1) {
		return ERR_INVALID_ARGUMENT;
	}

	p_vector = (vector_t*)malloc(sizeof(vector_t));
	if (NULL == p_vector) {
		return ERR_FAILED_ALLOC;
	}

	p_vector->capacity = 0;
	p_vector->element_size = element_size;
	p_vector->count = 0;
	p_vector->array = NULL;

	if (initial_capacity > 0) {
		err = _vector_adjust_capacity(p_vector, initial_capacity);
		if (NO_ERROR != err) {
			vector_release(p_vector);
			return err;
		}
	}

	*p_handle = p_vector;

	return NO_ERROR;
}

void vector_release(vector_handle_t handle) {
	if (NULL == handle) {
		return; 
	}
	free(handle->array);
	free(handle);
}

status_t vector_count(vector_handle_t handle, size_t* p_count) {
	if ((NULL == handle) || (NULL == p_count)) {
		return ERR_NULL_POINTER;
	}
	*p_count = handle->count;
	return NO_ERROR;
}

status_t vector_capacity(vector_handle_t handle, size_t* p_capacity) {
	if ((NULL == handle) || (NULL == p_capacity)) {
		return ERR_NULL_POINTER;
	}

	*p_capacity = handle->capacity;
	return NO_ERROR;
}

status_t vector_append(vector_handle_t handle, void* p_element) {
	status_t err = NO_ERROR;

	if ((NULL == handle) || (NULL == p_element)) {
		return ERR_NULL_POINTER;
	}

	if (handle->capacity <= (handle->count + 1)) {
		if (0 == handle->capacity) {
			err = _vector_adjust_capacity(handle, DEFAULT_VECTOR_CAPACITY);
		}
		else {
			err = _vector_adjust_capacity(handle, handle->capacity * 2);
		}

		if (NO_ERROR != err) {
			return err;
		}
	}

	memcpy(
		&(handle->array[handle->count * handle->element_size]),
		p_element, 
		handle->element_size);

	handle->count++;

	return NO_ERROR;
}

status_t vector_pop(vector_handle_t handle) {
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	if (handle->count > 0) {
		handle->count--;
		return NO_ERROR;
	}
	else {
		return ERR_EMPTY;
	}
}

status_t vector_clear(vector_handle_t handle) {
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	handle->count = 0;
	return NO_ERROR;
}

status_t vector_element_copy(
	vector_handle_t handle, 
	size_t index, 
	void* p_element)
{
	if ((NULL == handle) || (NULL == p_element)) {
		return ERR_NULL_POINTER;
	}

	if (index >= handle->count) {
		return ERR_RANGE_ERROR;
	}

	memcpy(
		p_element,
		&(handle->array[index * handle->element_size]),
		handle->element_size);

	return NO_ERROR;
}

status_t vector_element_address(
	vector_handle_t handle, 
	size_t index, 
	void** pp_element)
{
	if ((NULL == handle) || (NULL == pp_element)) {
		return ERR_NULL_POINTER;
	}

	if (index >= handle->count) {
		return ERR_RANGE_ERROR;
	}

	*pp_element = (void*)&(handle->array[index * handle->element_size]);

	return NO_ERROR;
}

status_t vector_remove(vector_handle_t handle, size_t index) {
	byte_t* move_from = NULL;
	byte_t* move_to = NULL;
	size_t move_size = 0;

	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}

	if (index >= handle->count) {
		return ERR_RANGE_ERROR;
	}

	move_size = ((handle->count - index) - 1) * handle->element_size;
	if (move_size) {
		move_to = &(handle->array[index * handle->element_size]);
		move_from = move_to + handle->element_size;
		memmove(move_to, move_from, move_size);
	}

	handle->count--;
	return NO_ERROR;
}

status_t vector_array(
	vector_handle_t handle, 
	void** pp_first_element)
{
	if ((NULL == handle) || (NULL == pp_first_element)) {
		return ERR_NULL_POINTER;
	}

	*pp_first_element = handle->array;
	return NO_ERROR;
}

status_t _vector_adjust_capacity(
	vector_handle_t handle, 
	size_t new_capacity)
{
	byte_t* new_array = NULL;
	size_t copy_size = 0;
	size_t new_capacity_size = 0;

	new_capacity_size = handle->element_size * new_capacity;
	new_array = (byte_t*)malloc(new_capacity_size);
	if (NULL == new_array) {
		return ERR_FAILED_ALLOC;
	}
	if (handle->count) {
		copy_size = handle->count * handle->element_size;
		if (copy_size > new_capacity_size) {
			copy_size = new_capacity_size;
		}

		if (copy_size) {
			memcpy(new_array, handle->array, copy_size);
		}
	}

	free(handle->array);
	handle->array = new_array;
	handle->capacity = new_capacity;
	return NO_ERROR;
}

