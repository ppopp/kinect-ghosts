#ifndef _common_h_
#define _common_h_

// Error codes
// everything is cool
#define NO_ERROR (0)
// a null pointer was passed into a function
#define ERR_NULL_POINTER (-1)
// a call to malloc returned with NULL
#define ERR_FAILED_ALLOC (-2)
// something failed in it's "create" call
#define ERR_FAILED_CREATE (-3)
// the index is beyond the bounds of the array
#define ERR_EMPTY (-4)
#define ERR_FAILED_THREAD_CREATE (-5)
#define ERR_RANGE_ERROR (-6)
// passed in a bad argument value
#define ERR_INVALID_ARGUMENT (-7)
// something is too big
#define ERR_EXCEED_ERROR (-8)
// failed to export some object to different format
#define ERR_FAILED_EXPORT (-9)
#define ERR_MUTEX_ERROR (-10)
#define ERR_FULL (-11)
#define ERR_INVALID_TIMESTAMP (-12)

#define ERR_FAILED_TIMER (-13)
#define ERR_DEVICE_ERROR (-14)
#define ERR_UNSUPPORTED_ARCHITECTURE (-15)
#define ERR_UNSUPPORTED_FORMAT (-16)


#include <stddef.h>


/** \addtogroup common
 * @{
 */


typedef int status_t;
typedef unsigned char byte_t;
typedef unsigned char bool_t;
typedef double timestamp_t;
typedef size_t frame_id_t;

/*
typedef unsigned timestamp_t;
*/

typedef int (*comparison_function)(const void*, const void*);

const char* error_string(status_t code);

#define TRUE (1)
#define FALSE (0)

/** @} */
#endif
