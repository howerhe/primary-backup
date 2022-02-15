#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/message.h"

int main(void)
{
	assert(message_set_id(NULL) != 0);

	struct message *msg = NULL;
	for (int i = 1; i <= 10; i++) {
		msg = malloc(sizeof(struct message));
		memset(msg, 0, sizeof(struct message));

		assert(message_set_id(msg) == 0);
		assert(msg->id == i);

		free(msg);
		msg = NULL;
	}

	msg = malloc(sizeof(struct message));
	memset(msg, 0, sizeof(struct message));
    assert(message_set_id(NULL) != 0);
	assert(message_set_id(msg) == 0);
	assert(msg->id == 11);
	free(msg);
	msg = NULL;

    printf("All tests passed.\n");

	return 0;
}