#ifndef _director_h_
#define _director_h_


/* TODO: logic for automatically recording.  We have presence and motion.
   some metrics might be:
   	- maximum length
	- minimum presence
	- minimum motion
	- minimum length

	There really could be any logic for determining start and end and what plays.

	The "director" controls the recording and playback.  probably want to keep things decoupled and have 
	the director's communications with the display_manager and kinect_manager in main, but lets think
	about that.

	

	[communication between director and display manager]
	[main] show the next frame (timestamp)
		[director] tells [display_manager] to display these frames (where does the director get the frames)
		or 
		[main] asks [director] for next frames and hands them to [display_manager]
		[director] asks [display_manager] how many layers it can display.

		? this sounds more like an editor

	[communication between director and kinect_manager]
		none.  

	[communication between director and frame_store]
	[director] determines wether [frame_store] should hold clip or not
	[director] tells [frame_store] when to makr clip boundary 
		- is that necessary though? can we store the sequence of frames elsewhere and just use the frame store
		  as a data housing.  But then isn't that the memory pool? 
	[director] cleans up clips by removing frames.  

	So the main functionality is
		- Cleanup clips
		- Provide flexibility in how to playback clips (lots of cool stuff could happen here)
		- Somehow manage recording.  Could just constantly record and then discard if the clip sucks.

*/

#include "common.h"

#define DIRECTOR_MAX_LAYERS (64)

#ifdef __cplusplus
extern "C" {
#endif
	/** \addtogroup director
	 * @{
	 */

	typedef struct director_s* director_handle_t;

	typedef struct director_frame_layers_s {
		void* video_layers[DIRECTOR_MAX_LAYERS];
		void* depth_layers[DIRECTOR_MAX_LAYERS];
		float depth_cutoffs[DIRECTOR_MAX_LAYERS];
		size_t layer_count;
	} director_frame_layers_t;

	status_t director_create(
		size_t max_layers,
		size_t max_bytes, 
		size_t bytes_per_video_frame,
		size_t bytes_per_depth_frame,
		size_t bytes_per_depth_pixel,
		float depth_scale,
		director_handle_t* p_handle);

	void director_release(director_handle_t handle);

	//TODO: get/set status_t director_set_*

	status_t director_playback_layers(
		director_handle_t handle, 
		timestamp_t delta, 
		director_frame_layers_t* p_layers);

	status_t director_capture_video(
		director_handle_t handle,
		void* data,
		timestamp_t timestamp);

	status_t director_capture_depth(
		director_handle_t handle,
		void* data,
		float cutoff,
		timestamp_t timestamp);

	/* TODO: settings
	   - contraints on marking start & end of clip
	   - constraints on quantizing loops
	   - ect.
	   */
	/** @} */

#ifdef __cplusplus
}
#endif
#endif
