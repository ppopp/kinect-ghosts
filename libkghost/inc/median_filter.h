#ifndef _median_filter_h_
#define _median_filter_h_

#include "common.h"

/*
   How is this going to go down?

   Everything is in RAM.  The slow way to do this is to 
   copy memory in for eacy pixel, sort it, then move onto next
   pixel.  We could do it all at once, or we could do each dimension
   separately in 3 passes.   

   1st just do it in the time domain, see if it improves things.

*/

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct zmedian_s* zmedian_handle_t;
	
	status_t zmedian_create(
		size_t element_size, 
		comparison_function compare
		zmedian_handle_t* p_handle);

	void zmedian_release(zmedian_handle_t handle);

	status_t zmedian_append(
		zmedian_handle_t handle,
		void* data, 
		size_t data_size);

	status_t zmedian_filter(
		zmedian_handle_t handle,
		size_t filter_len);

#ifdef __cplusplus
}
#endif

#endif
