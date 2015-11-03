#ifndef _editor_h_
#define _editor_h_

#include "common.h"

typedef struct editor_s* editor_handle_t;

/* TODO: add something about the format here */
status_t editor_create(editor_handle_t* p_handle);
status_t editor_release(editor_handle_t handle);

status_t editor_append_frame(
	editor_handle_t handle,
	const void* video, 
	const void* depth);
status_t editor_reset(editor_handle_t handle);


	

/* so editors... 
   We need some control mechanisms, in the following ways
   - because we don't have endless memory, we should probably
     have something say "start recording".  Or something that will
	 atleast tell the memory pool that we don't need certain frames.
   - then we also need somethign to say we're done and there's no more
     action in this clip
   - third, and this is a bit more optional, we need something to 
     setup the loop points and such in case we want to regularize
	 the looping.

   TODO: I think this requires a slight reworking of freerec to make it
         more like a frame-store.  We'll need some straight forward algorithms
		 which take in a series of frames and do detection ABC.  And we might
		 need a third mechanism to contain and define the loops.
*/


#endif
