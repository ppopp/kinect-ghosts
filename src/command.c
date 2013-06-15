#include "command.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct _command_handler {
	command_cb callback;
	struct _command_handler* next;
} command_handler;

typedef struct _commander {
	command_handler* handlers;
} _commander;

static void command_destroy_command_handlers(command_handler* head);
static command_handler* command_find_last_handler(commander cmdr);
static const char* _trim(
	const char* str, 
	size_t* len, 
	command_char_test_func testFunc);

commander command_create() {
	commander cmdr = NULL;
	cmdr = (commander)malloc(sizeof(_commander));
	if (cmdr) {
		cmdr->handlers = NULL;
	}
	return cmdr;
}

void command_destroy(commander cmdr) {
	if (cmdr) {
		command_destroy_command_handlers(cmdr->handlers);
		cmdr->handlers = NULL;
		free(cmdr);
	}
}

int command_set_callback(
	commander cmdr, 
	command_cb callback)
{
	command_handler* newHandler = NULL;
	command_handler* lastHandler = NULL;
	if ((cmdr == NULL) || (callback == NULL)) {
		return -1;
	}

	newHandler = (command_handler*)malloc(sizeof(command_handler));
	if (newHandler == NULL) {
		return -1;
	}

	newHandler->callback = callback;
	newHandler->next = NULL;

	lastHandler = command_find_last_handler(cmdr);
	if (lastHandler == NULL) {
		cmdr->handlers = newHandler;
	}
	else {
		lastHandler->next = newHandler;
	}

	return 0;
}

int command_handle(commander cmdr, const char* cmd, void* data) {
	command_handler* handler = cmdr->handlers;
	int handleCheck = COMMAND_CONTINUE;

	while ((handler != NULL) && (handleCheck >= COMMAND_CONTINUE)) {
		int result;
		result = handler->callback(cmd, data);
		handler = handler->next;
		if (result > handleCheck) {
			handleCheck = result;
		}
	}

	return handleCheck;
}

void command_destroy_command_handlers(command_handler* head) {
	command_handler* next = NULL;
	while (head != NULL) {
		next = head->next;
		free(head);
	}
}

command_handler* command_find_last_handler(commander cmdr) {
	command_handler* handler = cmdr->handlers;
	if (handler == NULL) {
		return NULL;
	}
	while (handler->next != NULL) {
		handler = handler->next;
	}
	return handler;
}

const char* command_find_target(
	const char* cmd, 
	const char* target, 
	int delim,
	command_char_test_func trimFunc)
{
	const char* value = NULL;
	size_t numTargetChars = 0;

	if ((cmd == NULL) || (target == NULL)) {
		return NULL;
	}

	value = strchr(cmd, delim);
	if (NULL == value) {
		return NULL;
	}
	numTargetChars = (size_t)(value - cmd);
	
	value++;
	value = _trim(value, NULL, trimFunc);
	if (NULL == value) {
		return NULL;
	}
	cmd = _trim(cmd, &numTargetChars, trimFunc);
	if (NULL == cmd) {
		return NULL;
	}

	if (numTargetChars != strlen(target)) {
		return NULL;
	}

	if (strncmp(target, cmd, numTargetChars) == 0) {
		return value;
	}
	return NULL;
}

const char* _trim(
	const char* str, 
	size_t* len, 
	command_char_test_func testFunc)
{
	size_t pos = 0;
	size_t end = 0;

	if ((NULL == str) || (NULL == testFunc)) {
		return str;
	}

	if (len) {
		end = *len;
	}
	else {
		end = strlen(str);
	}

	/* trim from beginning */
	while (testFunc(str[pos]) && (pos < end)) {
		pos++;
		end--;
	}
	str = &(str[pos]);

	/* if "len" was passed in, adjust "len" by trimming from end */
	if (len) {
		if (end > 0) {
			pos = end - 1;
			while (testFunc(str[pos]) && (pos >= 0)) {
				end--;
				pos--;
			}
		}
		*len = end;
	}
	return str;
}

