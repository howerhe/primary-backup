#include <stdatomic.h>
#include <stdio.h>

#include "message.h"

int message_set_id(struct message *msg)
{
	static unsigned global_id = 0;

	if (msg == NULL) {
		fprintf(stderr, "message_set_id: msg is NULL\n");
		return -1;
	}

	atomic_fetch_add(&global_id, 1);
	msg->id = global_id;

	return 0;
}