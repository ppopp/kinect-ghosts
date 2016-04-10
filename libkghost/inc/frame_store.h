#ifndef _frame_store_h_
#define _frame_store_h_

#include "common.h"

#include <libfreenect/libfreenect.h>

#ifdef __cplusplus
extern "C" {
#endif
	/** \addtogroup frame_store
	 * @{
	 */

	extern const frame_id_t invalid_frame_id;
	typedef struct frame_store_s* frame_store_handle_t;

	status_t frame_store_create(
		size_t video_bytes,
		size_t depth_bytes,
		size_t meta_bytes,
		size_t max_bytes,
		frame_store_handle_t* p_handle);

	void frame_store_release(frame_store_handle_t handle);

	status_t frame_store_frame_count(frame_store_handle_t handle, size_t* p_count);

	status_t frame_store_video_frame(
		frame_store_handle_t handle,
		frame_id_t frame_id,
		void** p_data,
		timestamp_t* p_timestamp);

	status_t frame_store_depth_frame(
		frame_store_handle_t handle,
		frame_id_t frame_id,
		void** p_data,
		timestamp_t* p_timestamp);

	status_t frame_store_meta_frame(
		frame_store_handle_t handle,
		frame_id_t frame_id,
		void** p_data,
		timestamp_t* p_timestamp);

	status_t frame_store_capture_video(
		frame_store_handle_t handle,
		void* data,
		timestamp_t timestamp,
		frame_id_t* p_frame_id);

	status_t frame_store_capture_depth(
		frame_store_handle_t handle,
		void* data,
		timestamp_t timestamp,
		frame_id_t* p_frame_id);

	status_t frame_store_capture_meta(
		frame_store_handle_t handle,
		void* data,
		timestamp_t timestamp,
		frame_id_t* p_frame_id);

	status_t frame_store_remove_frame(
		frame_store_handle_t handle,
		frame_id_t frame_id);

	/** @} */

#ifdef __cplusplus
}
#endif


#endif
