#ifndef _command_h_
#define _command_h_

#define COMMAND_ERROR (-2)
#define COMMAND_HANDLE_CLAIM (-1)

#define COMMAND_CONTINUE (1)
#define COMMAND_HANDLE_CONTINUE (2)

#ifdef __cplusplus
extern "C" {
#endif
	typedef int (*command_cb)(const char* cmd, void* data);
	typedef int (*command_char_test_func)(int val);

	typedef struct _commander* commander;

	commander command_create();
	void command_destroy(commander cmdr);
	int command_set_callback(commander cmdr, command_cb callback);
	int command_handle(commander cmdr, const char* cmd, void* data);

	const char* command_find_target(
		const char* cmd, 
		const char* target, 
		int delim,
		command_char_test_func trimFunc);
		

#ifdef __cplusplus
}
#endif

#endif

