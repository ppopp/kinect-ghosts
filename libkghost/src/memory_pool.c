#include "memory_pool.h"
#include "common.h"
#include "rb_tree.h"
#include "stack.h"
#include "log.h"

#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

/* memory pool
 *
 * The memory_pool utilizes a thread to run memory allocation.  On the thread
 * it allocates memory and places it in the generated stack.  Once it has
 * made the enough chunks it grabs a mutex and transfers them to the available
 * stack.  
 * 
 * When a user tries to claim a chunk, it grabs the mutex, pops from
 * the available stack, and places it into the claimed tree. 
 *
 * When a user unclaims a chunk, it is removed from the claimed tree, and
 * placed back in the available stack
 */

typedef struct memory_pool_s {
	size_t chunk_size;
	size_t min_reserve_chunks;
	size_t max_reserve_chunks;
	size_t max_chunks;
	size_t generated_chunks;
	stack_handle_t generated;
	rb_tree_handle_t claimed;
	stack_handle_t available;

	pthread_t thread;
	pthread_mutex_t mutex;
	int kill_thread;
	useconds_t period_microseconds;
} memory_pool_t;

static void* _memory_pool_thread(void* pool);
static status_t _memory_pool_generate_chunk(
	memory_pool_t* pool,
	stack_handle_t stack);
static status_t _release_stack_chunks(stack_handle_t stack);
static status_t _release_tree_chunks(rb_tree_handle_t tree);
static int _rb_tree_compare(const void*, const void*);
static status_t _rb_tree_alloc_copy_key(void*, void**);
static status_t _rb_tree_alloc_copy_data(void*, void**);
static status_t _rb_tree_release_key(void*);
static status_t _rb_tree_release_data(void*);

status_t memory_pool_create(
	size_t chunkSize, 
	size_t minReserve, 
	size_t maxReserve,
	size_t maxBytes,
	useconds_t periodMicroseconds,
	memory_pool_handle_t* p_handle) 
{
	status_t err = NO_ERROR;
	int pthreadErr = 0;
	pthread_attr_t pthreadAttr;
	memory_pool_t* pool = NULL;
	size_t chunkCount = 0;

	/* check input variables */
	if (
		(chunkSize < 1) ||
		(minReserve > maxReserve) ||
		(maxReserve < 1) ||
		(maxBytes < (maxReserve * chunkSize)) ||
		(periodMicroseconds <= 0))
	{
		return ERR_INVALID_ARGUMENT;
	}

	/* alloc/init struct */
	pool = (memory_pool_t*)malloc(sizeof(memory_pool_t));
	if (NULL == pool) {
		return ERR_FAILED_ALLOC;
	}

	pool->chunk_size = chunkSize;
	pool->min_reserve_chunks = minReserve;
	pool->max_reserve_chunks = maxReserve;
	pool->max_chunks = maxBytes / chunkSize;
	pool->generated_chunks = 0;
	pool->period_microseconds = periodMicroseconds;
	pool->claimed = NULL;
	pool->generated = NULL;
	pool->available = NULL;
	pool->thread = NULL;
	pool->kill_thread = 0;

	/* create chunk containers */
	err = stack_create(maxReserve, &(pool->generated));
	if (NO_ERROR != err) {
		memory_pool_release(pool);
		return err;
	}

	err = stack_create(maxBytes / chunkSize, &(pool->available));
	if (NO_ERROR != err) {
		memory_pool_release(pool);
		return err;
	}

	err = rb_tree_create(
		&_rb_tree_compare,
		&_rb_tree_alloc_copy_key,
		&_rb_tree_release_key,
		&_rb_tree_alloc_copy_data,
		&_rb_tree_release_data,
		&(pool->claimed));
	if (NO_ERROR != err) {
		memory_pool_release(pool);
		return err;
	}

	/* alloc minimum amount of data before creating thread */
	while ((chunkCount < pool->min_reserve_chunks) && (err == NO_ERROR)) {
		err = _memory_pool_generate_chunk(pool, pool->available);
		chunkCount++;
	}

	/* create generator thread */
	pthreadErr = pthread_mutex_init(&(pool->mutex), NULL);
	if (pthreadErr) {
		memory_pool_release(pool);
		return ERR_FAILED_THREAD_CREATE;
	}
	
	pthreadErr = pthread_attr_init(&pthreadAttr);
	if (pthreadErr) {
		memory_pool_release(pool);
		return ERR_FAILED_THREAD_CREATE;
	}

	pthreadErr = pthread_attr_setdetachstate(
		&pthreadAttr, 
		PTHREAD_CREATE_JOINABLE);
	if (pthreadErr) {
		pthread_attr_destroy(&pthreadAttr);
		memory_pool_release(pool);
		return ERR_FAILED_THREAD_CREATE;
	}

	pthreadErr = pthread_create(
		&(pool->thread), 
		&pthreadAttr, 
		_memory_pool_thread, 
		(void*)pool);
	pthread_attr_destroy(&pthreadAttr);
	if (pthreadErr) {
		memory_pool_release(pool);
		return ERR_FAILED_THREAD_CREATE;
	}

	*p_handle = pool;

	return NO_ERROR;
}

status_t memory_pool_release(memory_pool_handle_t pool) {
	void* threadStatus = NULL;
	int err = 0;
	if (NULL == pool) {
		return NO_ERROR;
	}
	/* set kill thread flag */
	err = pthread_mutex_lock(&(pool->mutex));
	pool->kill_thread = 1;
	err = pthread_mutex_unlock(&(pool->mutex));

	/* join thread */
	if (NULL != pool->thread) {
		pthread_join(pool->thread, &threadStatus);
	}

	/* release allocated data */
	_release_stack_chunks(pool->generated);
	_release_stack_chunks(pool->available);
	_release_tree_chunks(pool->claimed);

	rb_tree_release(pool->claimed);
	stack_release(pool->generated);
	stack_release(pool->available);
	free(pool);

	return NO_ERROR;
}

status_t memory_pool_claim(memory_pool_handle_t handle, void** data) {
	status_t status = NO_ERROR;
	int err = 0;
	void* chunk = NULL;
	rb_tree_node_handle_t node = NULL;

	if ((NULL == handle) || (NULL == data)) {
		return ERR_NULL_POINTER;
	}

	err = pthread_mutex_lock(&(handle->mutex));
	if (err) {
		return ERR_MUTEX_ERROR;
	}

	status = stack_pop(handle->available, &chunk);

	err = pthread_mutex_unlock(&(handle->mutex));
	if (NO_ERROR != status) {
		return status;
	}
	else if (err) {
		return ERR_MUTEX_ERROR;
	}

	status = rb_tree_insert(handle->claimed, chunk, NULL, &node);
	if (NO_ERROR != status) {
		return status;
	}

	*data = chunk;

	return NO_ERROR;
}

status_t memory_pool_unclaim(memory_pool_handle_t handle, void* data) {
	status_t status = NO_ERROR;
	rb_tree_node_handle_t node = NULL;
	int err = 0;

	if ((NULL == handle) || (NULL == data)) {
		return ERR_NULL_POINTER;
	}

	status = rb_tree_find(handle->claimed, data, &node);
	if (NO_ERROR != status) {
		return status;
	}
	if (NULL == node) {
		return ERR_INVALID_ARGUMENT;
	}

	err = pthread_mutex_lock(&(handle->mutex));
	if (err) {
		return ERR_MUTEX_ERROR;
	}
	status = stack_push(handle->available, data);
	err = pthread_mutex_unlock(&(handle->mutex));
	if (NO_ERROR != status) {
		return status;
	}

	status = rb_tree_remove(handle->claimed, node);
	if (NO_ERROR != status) {
		return status;
	}

	return NO_ERROR;
}

int _rb_tree_compare(const void* key1, const void* key2) {
	return memcmp(&key1, &key2, sizeof(void*));
}

status_t _rb_tree_alloc_copy_key(void* in, void** copy) {
	*copy = in;
	return NO_ERROR;
}

status_t _rb_tree_alloc_copy_data(void* in, void** copy) {
	*copy = in;
	return NO_ERROR;
}

status_t _rb_tree_release_key(void* key) {
	return NO_ERROR;
}

status_t _rb_tree_release_data(void* data) {
	return NO_ERROR;
}

void* _memory_pool_thread(void* data) {
	memory_pool_t* pool = (memory_pool_t*)data;
	int doKill = 0;
	int err = 0;
	status_t status = NO_ERROR;
	size_t chunkCount = 0;
	size_t availableChunks = 0;
	void* chunk = NULL;

	if (NULL == pool) {
		pthread_exit(NULL);
	}

	while (0 == doKill) {
		/* update whether to kill thread or not */
		err = pthread_mutex_lock(&(pool->mutex));
		if (err) {
			pthread_exit(NULL);
		}

		doKill = pool->kill_thread;

		err = pthread_mutex_unlock(&(pool->mutex));
		if (err) {
			pthread_exit(NULL);
		}

		if (doKill) {
			/* skip rest of loop and exit while loop */
			continue;
		}

		/* get number of chunks available */
		err = pthread_mutex_lock(&(pool->mutex));
		if (err) {
			pthread_exit(NULL);
		}
		status = stack_count(pool->available, &availableChunks);
		if (NO_ERROR != status) {
			pthread_exit(NULL);
		}
		err = pthread_mutex_unlock(&(pool->mutex));
		if (err) {
			pthread_exit(NULL);
		}

		/* determine if more chunks are needed */
		if (pool->min_reserve_chunks > availableChunks) {
			chunkCount = pool->max_reserve_chunks - availableChunks;
			while (
				(chunkCount > 0) &&
				((chunkCount + pool->generated_chunks) > pool->max_chunks))
			{
				chunkCount--;
				LOG_DEBUG("chunk count %u", chunkCount);
			}

			while ((chunkCount != 0) && (status == NO_ERROR)) {
				LOG_DEBUG("chunks to generate %u", chunkCount);
				status = _memory_pool_generate_chunk(pool, pool->generated);
				chunkCount--;
			}
			if (NO_ERROR != status) {
				pthread_exit(NULL);
			}

			LOG_DEBUG("generate %u of %u bytes", pool->generated_chunks, pool->max_chunks);
		}

		/* if chunks exist in generated stack, place them in available stack */
		status = stack_count(pool->generated, &chunkCount);
		err = pthread_mutex_lock(&(pool->mutex));
		if (err) {
			pthread_exit(NULL);
		}
		while ((NO_ERROR == status) && (chunkCount > 0)) {
			status = stack_pop(pool->generated, &chunk);
			if (NO_ERROR != status) {
				continue;
			}
			status = stack_push(pool->available, chunk);
			if (NO_ERROR != status) {
				continue;
			}
			chunkCount--;
		}
		err = pthread_mutex_unlock(&(pool->mutex));
		if (err) {
			pthread_exit(NULL);
		}
		if (NO_ERROR != status) {
			pthread_exit(NULL);
		}

		/* sleep */
		err = usleep(pool->period_microseconds);
		if (err) {
			pthread_exit(NULL);
		}
	}

	pthread_exit(NULL);
}

status_t _memory_pool_generate_chunk(
	memory_pool_t* pool, 
	stack_handle_t stack) {
	status_t err = NO_ERROR;
	void* newChunk = NULL;

	newChunk = malloc(pool->chunk_size);
	if (NULL == newChunk) {
		return ERR_FAILED_ALLOC;
	}

	err = stack_push(stack, newChunk);
	if (NO_ERROR != err) {
		free(newChunk);
		return err;
	}
	pool->generated_chunks += 1;

	return NO_ERROR;
}

status_t _release_stack_chunks(stack_handle_t stack) {
	status_t status = NO_ERROR;
	size_t count = 0;
	void* chunk = NULL;

	if (NULL == stack) {
		return ERR_NULL_POINTER;
	}

	status = stack_count(stack, &count);
	while ((status == NO_ERROR) && (count > 0)) {
		status = stack_pop(stack, &chunk);
		if (NO_ERROR != status) {
			continue;
		}
		free(chunk);
		status = stack_count(stack, &count);
	}
	return status;
}

status_t _release_tree_chunks(rb_tree_handle_t tree) {
	status_t status = NO_ERROR;
	rb_tree_node_handle_t last = NULL;
	rb_tree_node_handle_t next = NULL;
	void* chunk = NULL;

	if (NULL == tree) {
		return ERR_NULL_POINTER;
	}

	status = rb_tree_enumerate(
		tree,
		NULL,
		NULL,
		last,
		&next);
	while ((NO_ERROR == status) && (NULL != next)) {
		last = next;
		status = rb_tree_node_key(next, &chunk);
		if (NO_ERROR != status) {
			continue;
		}
		free(chunk);
		status = rb_tree_enumerate(
			tree,
			NULL,
			NULL,
			last,
			&next);
	}
	return status;
}

