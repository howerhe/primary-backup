#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "synchronization.h"

struct synchronization_queue *synchronization_queue_init()
{
	struct synchronization_queue *q =
		malloc(sizeof(struct synchronization_queue));
	if (q == NULL)
		goto error;
	if (pthread_mutex_init(&q->lock, NULL) != 0)
		goto error;
	if (pthread_cond_init(&q->cond, NULL) != 0)
		goto error;
	q->cap = malloc(sizeof(struct synchronization_task));
	if (q->cap == NULL)
		goto error;

	q->cap->index = __INT_MAX__;
	q->cap->version = __INT_MAX__;
	q->cap->next = NULL;
	q->length = 0;
	q->tail = q->cap;

	return q;

error:
	perror("synchronization_queue_init");
	synchronization_queue_free(q);
	q = NULL;
	return NULL;
}

void synchronization_queue_free(struct synchronization_queue *q)
{
	if (q == NULL)
		return;

	while (synchronization_queue_is_empty(q) == 0) {
		struct synchronization_task *t =
			synchronization_queue_dequeue(q);
		free(t);
		t = NULL;
	}
	free(q->cap);
	q->cap = NULL;
	pthread_cond_destroy(&q->cond);
	pthread_mutex_destroy(&q->lock);
	free(q);
	q = NULL;
}

int synchronization_queue_is_empty(struct synchronization_queue *q)
{
	if (q == NULL)
		return 1;

	return q->length == 0;
}

struct synchronization_task *
synchronization_queue_peek(struct synchronization_queue *q)
{
	if (synchronization_queue_is_empty(q) == 1)
		return NULL;

	struct synchronization_task *t =
		malloc(sizeof(struct synchronization_task));
	if (t == NULL) {
		perror("synchronization_queue_peek");
		return NULL;
	}

	struct synchronization_task *head = q->cap->next;
	t->index = head->index;
	t->value = head->value;
	t->version = head->version;
	strncpy(t->addr, head->addr, MESSAGE_ADDR_SIZE);
	strncpy(t->port, head->port, MESSAGE_PORT_SIZE);
	t->sent_time.tv_sec = head->sent_time.tv_sec;
	t->sent_time.tv_usec = head->sent_time.tv_usec;
	t->next = NULL;

	return t;
}

int synchronization_queue_enqueue(struct synchronization_queue *q,
				  struct synchronization_task *t)
{
	char *err_msg = NULL;

	if (q == NULL) {
		err_msg = "q is NULL";
		goto error;
	}
	if (t == NULL) {
		err_msg = "t is NULL";
		goto error;
	}

	q->length++;
	q->tail->next = t;
	q->tail = t;
	t->next = NULL;

	return 0;

error:
	fprintf(stderr, "synchronization_queue_enqueue: %s\n", err_msg);
	return -1;
}

struct synchronization_task *
synchronization_queue_dequeue(struct synchronization_queue *q)
{
	char *err_msg = NULL;

	if (q == NULL) {
		err_msg = "q is NULL";
		goto error;
	}
	if (synchronization_queue_is_empty(q) == 1) {
		err_msg = "q is empty";
		goto error;
	}

	struct synchronization_task *t;

	q->length--;
	t = q->cap->next;
	q->cap->next = t->next;
	if (q->length == 0)
		q->tail = q->cap;
	t->next = NULL;

	return t;

error:
	fprintf(stderr, "synchronization_queue_dequeue: %s\n", err_msg);
	return NULL;
}

int synchronization_queue_remove_task(struct synchronization_queue *q,
				      unsigned index, unsigned version)
{
	char *err_msg = NULL;

	if (q == NULL) {
		err_msg = "q is NULL";
		goto error;
	}

	struct synchronization_task *pre = q->cap;
	struct synchronization_task *cur = pre->next;

	// Should remove the tasks whose version is older?
	while (cur != NULL) {
		if (cur->index == index && cur->version <= version) {
			q->length--;
			pre->next = cur->next;
			if (cur->next == NULL)
				q->tail = pre;
			free(cur);
			cur = NULL;
			break;
		} else {
			pre = cur;
			cur = cur->next;
		}
	}

	return 0;

error:
	fprintf(stderr, "synchronization_queue_remove_task: %s\n", err_msg);
	return -1;
}