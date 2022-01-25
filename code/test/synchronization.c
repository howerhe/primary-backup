#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "../src/synchronization.h"

int main(void)
{
	int rv = -1;

	// synchronization_queue_init().
	struct synchronization_queue *q = synchronization_queue_init();
	assert(q);

	// synchronization_queue_is_empty().
	assert(synchronization_queue_is_empty(NULL) == 1);
	assert(synchronization_queue_is_empty(q) == 1);

	// Prepare for following tests.
	unsigned index1 = 1;
	int value1 = 2;
	unsigned version1 = 0;
	char *addr1 = "0.0.0.0";
	char *port1 = "10";
	struct timeval sent_time1;
	gettimeofday(&sent_time1, NULL);
	unsigned index2 = 5;
	int value2 = 6;
	unsigned version2 = 4;
	char *addr2 = "127.0.0.1";
	char *port2 = "9999";
	struct timeval sent_time2;
	gettimeofday(&sent_time2, NULL);
	unsigned index3 = 10;
	int value3 = -1;
	unsigned version3 = 100;
	char *addr3 = "255.255.255.255";
	char *port3 = "88";
	struct timeval sent_time3;
	gettimeofday(&sent_time3, NULL);

	struct synchronization_task *t1 =
		malloc(sizeof(struct synchronization_task));
	struct synchronization_task *t2 =
		malloc(sizeof(struct synchronization_task));
	struct synchronization_task *t3 =
		malloc(sizeof(struct synchronization_task));
	assert(t1);
	assert(t2);
	assert(t3);
	struct synchronization_task *tmp;

	t1->index = index1;
	t1->value = value1;
	t1->version = version1;
	strncpy(t1->addr, addr1, MESSAGE_ADDR_SIZE);
	strncpy(t1->port, port1, MESSAGE_PORT_SIZE);
	t1->sent_time.tv_sec = sent_time1.tv_sec;
	t1->sent_time.tv_usec = sent_time1.tv_usec;
	t2->index = index2;
	t2->value = value2;
	t2->version = version2;
	strncpy(t2->addr, addr2, MESSAGE_ADDR_SIZE);
	strncpy(t2->port, port2, MESSAGE_PORT_SIZE);
	t2->sent_time.tv_sec = sent_time2.tv_sec;
	t2->sent_time.tv_usec = sent_time2.tv_usec;
	t3->index = index3;
	t3->value = value3;
	t3->version = version3;
	strncpy(t3->addr, addr3, MESSAGE_ADDR_SIZE);
	strncpy(t3->port, port3, MESSAGE_PORT_SIZE);
	t3->sent_time.tv_sec = sent_time3.tv_sec;
	t3->sent_time.tv_usec = sent_time3.tv_usec;

	// synchronization_queue_enqueue(), synchronization_queue_peek(), synchronization_queue_is_empty().
	rv = synchronization_queue_enqueue(NULL, t1);
	assert(rv != 0);
	rv = synchronization_queue_enqueue(q, NULL);
	assert(rv != 0);
	assert(synchronization_queue_is_empty(q) == 1);
	tmp = synchronization_queue_peek(NULL);
	assert(tmp == NULL);
	tmp = synchronization_queue_peek(q);
	assert(tmp == NULL);
	assert(synchronization_queue_is_empty(q) == 1);

	rv = synchronization_queue_enqueue(q, t1);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	tmp = synchronization_queue_peek(q);
	assert(tmp->index == index1);
	assert(tmp->value == value1);
	assert(tmp->version == version1);
	assert(strcmp(tmp->addr, addr1) == 0);
	assert(strcmp(tmp->port, port1) == 0);
	assert(tmp->sent_time.tv_sec == sent_time1.tv_sec);
	assert(tmp->sent_time.tv_usec == sent_time1.tv_usec);
	free(tmp);
	tmp = NULL;
	assert(synchronization_queue_is_empty(q) == 0);

	rv = synchronization_queue_enqueue(q, t2);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	tmp = synchronization_queue_peek(q);
	assert(tmp->index == index1);
	assert(tmp->value == value1);
	assert(tmp->version == version1);
	assert(strcmp(tmp->addr, addr1) == 0);
	assert(strcmp(tmp->port, port1) == 0);
	assert(tmp->sent_time.tv_sec == sent_time1.tv_sec);
	assert(tmp->sent_time.tv_usec == sent_time1.tv_usec);
	free(tmp);
	tmp = NULL;
	assert(synchronization_queue_is_empty(q) == 0);

	rv = synchronization_queue_enqueue(q, t3);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	tmp = synchronization_queue_peek(q);
	assert(tmp->index == index1);
	assert(tmp->value == value1);
	assert(tmp->version == version1);
	assert(strcmp(tmp->addr, addr1) == 0);
	assert(strcmp(tmp->port, port1) == 0);
	assert(tmp->sent_time.tv_sec == sent_time1.tv_sec);
	assert(tmp->sent_time.tv_usec == sent_time1.tv_usec);
	free(tmp);
	tmp = NULL;
	assert(synchronization_queue_is_empty(q) == 0);

	// synchronization_queue_peek() should return a copy.
	tmp = synchronization_queue_peek(q);
	tmp->index = index3;
	tmp->value = value2;
	tmp->version = version2;
	strncpy(tmp->addr, addr3, MESSAGE_ADDR_SIZE);
	strncpy(tmp->port, port2, MESSAGE_PORT_SIZE);
	tmp->sent_time.tv_sec = sent_time3.tv_sec;
	tmp->sent_time.tv_usec = sent_time2.tv_usec;
	free(tmp);
	tmp = NULL;
	tmp = synchronization_queue_peek(q);
	assert(tmp->index == index1);
	assert(tmp->value == value1);
	assert(tmp->version == version1);
	assert(strcmp(tmp->addr, addr1) == 0);
	assert(strcmp(tmp->port, port1) == 0);
	assert(tmp->sent_time.tv_sec == sent_time1.tv_sec);
	assert(tmp->sent_time.tv_usec == sent_time1.tv_usec);
	free(tmp);
	tmp = NULL;

	// synchronization_queue_dequeue(), synchronization_queue_peek(), synchronization_queue_is_empty().
	t1 = NULL;
	t2 = NULL;
	t3 = NULL;
	assert(synchronization_queue_dequeue(NULL) == NULL);

	t1 = synchronization_queue_dequeue(q);
	assert(t1->index == index1);
	assert(t1->value == value1);
	assert(t1->version == version1);
	assert(strcmp(t1->addr, addr1) == 0);
	assert(strcmp(t1->port, port1) == 0);
	assert(t1->sent_time.tv_sec == sent_time1.tv_sec);
	assert(t1->sent_time.tv_usec == sent_time1.tv_usec);
	assert(synchronization_queue_is_empty(q) == 0);
	tmp = synchronization_queue_peek(q);
	assert(tmp->index == index2);
	assert(tmp->value == value2);
	assert(tmp->version == version2);
	assert(strcmp(tmp->addr, addr2) == 0);
	assert(strcmp(tmp->port, port2) == 0);
	assert(tmp->sent_time.tv_sec == sent_time2.tv_sec);
	assert(tmp->sent_time.tv_usec == sent_time2.tv_usec);
	free(tmp);
	tmp = NULL;
	assert(synchronization_queue_is_empty(q) == 0);

	t2 = synchronization_queue_dequeue(q);
	assert(t2->index == index2);
	assert(t2->value == value2);
	assert(t2->version == version2);
	assert(strcmp(t2->addr, addr2) == 0);
	assert(strcmp(t2->port, port2) == 0);
	assert(t2->sent_time.tv_sec == sent_time2.tv_sec);
	assert(t2->sent_time.tv_usec == sent_time2.tv_usec);
	assert(synchronization_queue_is_empty(q) == 0);
	tmp = synchronization_queue_peek(q);
	assert(tmp->index == index3);
	assert(tmp->value == value3);
	assert(tmp->version == version3);
	assert(strcmp(tmp->addr, addr3) == 0);
	assert(strcmp(tmp->port, port3) == 0);
	assert(tmp->sent_time.tv_sec == sent_time3.tv_sec);
	assert(tmp->sent_time.tv_usec == sent_time3.tv_usec);
	free(tmp);
	tmp = NULL;
	assert(synchronization_queue_is_empty(q) == 0);

	t3 = synchronization_queue_dequeue(q);
	assert(t3->index == index3);
	assert(t3->value == value3);
	assert(t3->version == version3);
	assert(strcmp(t3->addr, addr3) == 0);
	assert(strcmp(t3->port, port3) == 0);
	assert(t3->sent_time.tv_sec == sent_time3.tv_sec);
	assert(t3->sent_time.tv_usec == sent_time3.tv_usec);
	assert(synchronization_queue_is_empty(q) == 1);
	tmp = synchronization_queue_peek(q);
	assert(tmp == NULL);
	assert(synchronization_queue_is_empty(q) == 1);

	tmp = synchronization_queue_peek(q);
	assert(tmp == NULL);

	// synchronization_queue_enqueue(), synchronization_queue_is_empty().
	rv = synchronization_queue_enqueue(q, t1);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	rv = synchronization_queue_enqueue(q, t2);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);

	// synchronization_queue_remove_task(), synchronization_queue_is_empty().
	rv = synchronization_queue_remove_task(NULL, 10, 100);
	assert(rv != 0);
	rv = synchronization_queue_remove_task(q, index2, 100);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	rv = synchronization_queue_remove_task(q, 10, version2);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	rv = synchronization_queue_remove_task(q, index2, version2);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	rv = synchronization_queue_remove_task(q, index2, version2);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	// Should work after removing the tail task following by enqueue.
	rv = synchronization_queue_enqueue(q, t3);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	rv = synchronization_queue_remove_task(q, index3, version3);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	rv = synchronization_queue_remove_task(q, index3, version3);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 0);
	rv = synchronization_queue_remove_task(q, index1, version1);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 1);
	rv = synchronization_queue_remove_task(q, index1, version1);
	assert(rv == 0);
	assert(synchronization_queue_is_empty(q) == 1);

	// synchronization_queue_free().
	synchronization_queue_free(q);
	q = NULL;

	printf("All tests passed.\n");

	return 0;
}