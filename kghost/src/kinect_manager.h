#ifndef _kinect_manager_h_
#define _kinect_manager_h_

#include "common.h"
#include "timer.h"

#include <string.h>

/* Depth bits per pixel */
#define CMI_DEPTH_BPP_SIZE_T ("cmi_depth_bpp_size_t")
/* Depth width */
#define CMI_DEPTH_WIDTH_SIZE_T ("cmi_depth_width_size_t")
/* Depth height */
#define CMI_DEPTH_HEIGHT_SIZE_T ("cmi_depth_height_size_t")
/* Video bits per pixel */
#define CMI_VIDEO_BPP_SIZE_T ("cmi_video_bpp_size_t")
/* Video width */
#define CMI_VIDEO_WIDTH_SIZE_T ("cmi_video_width_size_t")
/* Video height */
#define CMI_VIDEO_HEIGHT_SIZE_T ("cmi_video_height_size_t")

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct _kinect_manager_s* kinect_manager_handle_t;

	typedef enum _kinect_manager_resolution_t {
		kinect_manager_resolution_320x240 = 0,
		kinect_manager_resolution_640x480 = 1,
		kinect_manager_resolution_1280x1024 = 2
	} kinect_manager_resolution_t;

	typedef struct _kinect_callbacks_s {
		void (*video_ready_callback)(
			kinect_manager_handle_t handle,
			void* user_data);
		void (*video_frame_callback)(
			kinect_manager_handle_t handle,
			void* video_data, 
			timestamp_t p_timestamp,
			void* user_data);
		void (*depth_ready_callback)(
			kinect_manager_handle_t handle,
			void* user_data);
		void (*depth_frame_callback)(
			kinect_manager_handle_t handle,
			void* depth_data, 
			timestamp_t p_timestamp,
			void* user_data);
	} kinect_callbacks_t;


	status_t kinect_manager_create(
		kinect_manager_handle_t* p_handle,
		kinect_manager_resolution_t resolution,
		kinect_callbacks_t* p_callbacks,
		void* user_data);

	void kinect_manager_destroy(kinect_manager_handle_t handle);

	status_t kinect_manager_info(
		kinect_manager_handle_t handle,
		const char* field,
		void* p_data,
		size_t data_size);

	status_t kinect_manager_live_video(
		kinect_manager_handle_t handle,
		void** pp_data);
	status_t kinect_manager_live_depth(
		kinect_manager_handle_t handle,
		void** pp_data);

	status_t kinect_manager_capture_frame(
		kinect_manager_handle_t handle,
		bool_t* p_captured);
	status_t kinect_manager_reset_timestamp(kinect_manager_handle_t handle);

	status_t kinect_manager_set_camera_angle(
		kinect_manager_handle_t handle,
		double angle);
	status_t kinect_manager_get_camera_angle(
		kinect_manager_handle_t handle,
		double* p_angle);

/* TODO: add control over logging */

#ifdef __cplusplus
}
#endif

#endif
