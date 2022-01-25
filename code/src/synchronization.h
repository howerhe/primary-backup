#ifndef SYNCHRONIZATION_H
#define SYNCHRONIZATION_H

#include <pthread.h>
#include <sys/time.h>

#include "message.h"

// Each message queue is for a dedicated backup server.
// The best data structure should be a MapTree: ordered, efficient to search.
// For now I just use standard queues instead.

/**
 * @brief The task for synchronization.
 * 
 */
struct synchronization_task {
	unsigned index;
	int value;
	unsigned version;
	char addr[MESSAGE_ADDR_SIZE];
	char port[MESSAGE_PORT_SIZE];
	struct timeval sent_time;
	struct synchronization_task *next;
};

/**
 * @brief The queue for synchronization tasks.
 * 
 */
struct synchronization_queue {
	unsigned length;
	struct synchronization_task *cap;
	struct synchronization_task *tail;
	pthread_mutex_t lock;
	pthread_cond_t cond;
};

/**
 * @brief Initiate a queue for synchronization tasks.
 * 
 * @return struct synchronization_queue* representing the queue, NULL when errors occur
 */
struct synchronization_queue *synchronization_queue_init();

/**
 * @brief Clean up a synchronization task queue.
 * 
 * @param q the queue
 */
void synchronization_queue_free(struct synchronization_queue *q);

/**
 * @brief Determine whether a synchronization task queue is empty or not.
 * 
 * @param q 
 * @return int 1 when the queue is empty, 0 otherwise
 */
int synchronization_queue_is_empty(struct synchronization_queue *q);

/**
 * @brief Peek a synchronization task queue but not remove the peeked task.
 * 
 * @param q the queue
 * @return struct synchronization_task* representing the task, NULL when the queue is empty or when errors occur
 */
struct synchronization_task *
synchronization_queue_peek(struct synchronization_queue *q);

/**
 * @brief Enqueue a synchronization task into a synchronization task queue. The returned synchronization task should be a copy.
 * 
 * @param q the queue
 * @param t the pointer to the task
 * @return int 0 when no errors occur
 */
int synchronization_queue_enqueue(struct synchronization_queue *q,
				  struct synchronization_task *t);

/**
 * @brief Dequeue from a synchronization task queue. Caller should check whether the queue is empty or not.
 * 
 * @param q the queue
 * @return struct synchronization_task* representing the task, NULL when the queue is empty or when errors occur
 */
struct synchronization_task *
synchronization_queue_dequeue(struct synchronization_queue *q);

/**
 * @brief Remove a task from a synchronization task queue. The task is specified with the index and version. If the specified task is not found, do nothing.
 * 
 * @param q the queue
 * @param index the index
 * @param version the version
 * @return int 0 when no errors occur
 */
int synchronization_queue_remove_task(struct synchronization_queue *q,
				      unsigned index, unsigned version);

#endif