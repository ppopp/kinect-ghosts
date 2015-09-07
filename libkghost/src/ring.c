#include "ring.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>

typedef struct ring_s {
	size_t capacity;
	size_t count;
	size_t element_size;
	size_t data_size;
	byte_t* data;
	/* head is a byte counter (not element counter) that points to the position
	 * in 'data' where the next insertion should be made */
	size_t head;
} ring_t;

status_t ring_create(
	size_t capacity,
	size_t element_size,
	ring_handle_t* p_handle)
{
	ring_t* p_ring = NULL;

	if ((capacity < 1) ||
		(element_size < 1))
	{
		return ERR_INVALID_ARGUMENT;
	}

	if (NULL == p_handle) {
		return ERR_NULL_POINTER;
	}

	p_ring = (ring_t*)malloc(sizeof(ring_t));
	if (NULL == p_ring) {
		return ERR_FAILED_ALLOC;
	}

	memset(p_ring, 0, sizeof(ring_t));
	p_ring->data = (byte_t*)malloc(capacity * element_size);
	if (NULL == p_ring->data) {
		ring_release(p_ring);
		return ERR_FAILED_ALLOC;
	}

	/* point head to last set of bytes where data can be inserted */
	p_ring->capacity = capacity;
	p_ring->element_size = element_size;
	p_ring->data_size = element_size * capacity;
	p_ring->head = p_ring->data_size - element_size;
	p_ring->count = 0;

	*p_handle = p_ring;
	return NO_ERROR;
}

void ring_release(ring_handle_t handle) {
	if (NULL == handle) {
		return;
	}

	free(handle->data);
	free(handle);
}
	
status_t ring_count(ring_handle_t handle, size_t* p_count) {
	if ((NULL == handle) || (NULL == p_count)) {
		return ERR_NULL_POINTER;
	}
	*p_count = handle->count;
	return NO_ERROR;
}

status_t ring_capacity(ring_handle_t handle, size_t* p_capacity) {
	if ((NULL == handle) || (NULL == p_capacity)) {
		return ERR_NULL_POINTER;
	}
	*p_capacity = handle->capacity;
	return NO_ERROR;
}

status_t ring_push(ring_handle_t handle, const void* p_element) {
	if ((NULL == handle) ||
		(NULL == p_element))
	{
		return ERR_NULL_POINTER;
	}

	/* make internal copy of element */
	memcpy(&(handle->data[handle->head]), p_element, handle->element_size);

	/* Move head to new position, wrap if necessary. Note that the pointer is
	 * moved to the left in the dat array so that the most recent entries are
	 * farthest left. */
	if (0 == handle->head) {
		handle->head = handle->data_size - handle->element_size;
	}
	else {
		handle->head -= handle->element_size;
	}

	/* increment count if we're not writing over something */
	if (handle->count < handle->capacity) {
		handle->count += 1;
	}

	return NO_ERROR;
}

status_t ring_clear(ring_handle_t handle) {
	if (NULL == handle) {
		return ERR_NULL_POINTER;
	}
	handle->count = 0;
	handle->head = 0;
	return NO_ERROR;
}

status_t ring_element(
	ring_handle_t handle, 
	size_t index, 
	void** pp_element)
{
	size_t position = 0;

	if ((NULL == handle) || (NULL == pp_element)) {
		return ERR_NULL_POINTER;
	}

	if (index >= handle->count) {
		return ERR_RANGE_ERROR;
	}

    /* TODO: i think indexing is wrong if we're at capacity, or something ...
     * why isn't the count used to determine the position?, why is index=0
     * not the same as position = 0?
     */
	position = ((index + 1) * handle->element_size + handle->head);
	position %= handle->data_size;

	*pp_element = (void*)&(handle->data[position]);

	return NO_ERROR;
}

