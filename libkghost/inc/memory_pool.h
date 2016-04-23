#ifndef _memory_pool_h_
#define _memory_pool_h_

#include "common.h"
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif
	/** \addtogroup memory_pool
	 * @{
	 */

	typedef struct memory_pool_s* memory_pool_handle_t;

	status_t memory_pool_create(
		size_t chunkSize, 
		size_t minReserveChunks, 
		size_t maxReserveChunks,
		size_t maxBytes,
		useconds_t periodMicroseconds,
		memory_pool_handle_t* p_handle);
	status_t memory_pool_release(memory_pool_handle_t handle);
	status_t memory_pool_claim(memory_pool_handle_t handle, void** data);
	status_t memory_pool_unclaim(memory_pool_handle_t handle, void* data);

	/** @} */

#ifdef __cplusplus
}
#endif

#endif
