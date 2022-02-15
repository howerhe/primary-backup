#include <stdio.h>

#include "message.h"

int message_set_id(struct message *msg)
{
	static unsigned global_id = 0;

	if (msg == NULL) {
		fprintf(stderr, "message_set_id: msg is NULL\n");
		return -1;
	}

	global_id++;
	msg->id = global_id;

	return 0;
}