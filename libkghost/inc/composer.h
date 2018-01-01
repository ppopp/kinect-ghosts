#ifndef _composer_h_
#define _composer_h_

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif
	/* TODO: finish this. probably going to have this control which loops 
	 * get played at what time
	 */

	/* TODO: if we identify loops in director.c by their location in an array,
	 * how can we remove a loop without fucking everything up?  Can we use
	 * pointers instead of indexes?
	 */

	typedef struct composer_s {
		double tatum;
		int max_repetitions;
		int min_repetitions;
		int max_density;
		int min_density;
	} composer_t;

	status_t composer_loop_added();
	status_t composer_loop_removed();
	status_t composer_set_playing_loops();




#ifdef __cplusplus
}
#endif
#endif

