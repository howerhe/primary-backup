#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/message.h"

#define THREAD_NUM 100
#define CYCLE_NUM 100000

void *multithread_test(void *args);

int main(void)
{
	// Single thread test.
	assert(message_set_id(NULL) != 0);

	struct message *msg = NULL;
	for (int i = 0; i < 10; i++) {
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
	assert(msg->id == 10);
	free(msg);
	msg = NULL;

	// Multithread test.
	// Even for non-protected counter, it's rare to have race conditions.
	pthread_t threads[THREAD_NUM];
	for (int i = 0; i < THREAD_NUM; i++)
		assert(pthread_create(&threads[i], NULL, multithread_test,
				      NULL) == 0);

	for (int i = 0; i < THREAD_NUM; i++)
		assert(pthread_join(threads[i], NULL) == 0);

	msg = malloc(sizeof(struct message));
	memset(msg, 0, sizeof(struct message));
	assert(message_set_id(NULL) != 0);
	assert(message_set_id(msg) == 0);
	assert(msg->id == 11 + THREAD_NUM * CYCLE_NUM);
	free(msg);
	msg = NULL;

	printf("All tests passed.\n");

	return 0;
}

void *multithread_test(void *args)
{
	struct message *msg = NULL;
	for (int i = 0; i < CYCLE_NUM; i++) {
		msg = malloc(sizeof(struct message));
		assert(msg != NULL);
		memset(msg, 0, sizeof(struct message));
		assert(message_set_id(msg) == 0);
		free(msg);
		msg = NULL;
	}

	return NULL;
}