#include <assert.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "src/consistency.h"
#include "src/message.h"
#include "src/store.h"
#include "src/socket.h"
#include "src/synchronization.h"
#include "src/pool.h"

#define REFRESH_TIME 2
#define RESENT_TIME 2000
#define POOL_SIZE 80

struct request_handler_info {
	struct consistency_state *state;
	struct message *msg;
};

struct synchronize_sender_info {
	struct consistency_state *state;
	int backup_i;
};

pool_t *pool;

void *request_handler(void *arg);
void *synchronize_sender(void *arg);
void *synchronize_refresher(void *arg);
void *synchronize_receiver(void *arg);
void *synchronize_handler(void *arg);

// The sync port for the primary is set as the original port number - 1.
int main(int argc, char *argv[])
{
	if (argc < 8 || argc % 2 != 0) {
		fprintf(stderr,
			"usage for backup: ./server 0 sematics_code store_size self_addr self_port primary_addr primary_port\n");
		fprintf(stderr,
			"usage for primary: ./server 1 sematics_code store_size self_addr self_port backup_addr backup_port ...\n");
		exit(EXIT_FAILURE);
	}

	// Assume memory allocations and socket_init() always work.
	struct consistency_state *s = malloc(sizeof(struct consistency_state));
	assert(s);
	s->role = (int)strtol(argv[1], NULL, 10);
	s->semantics = (int)strtol(argv[2], NULL, 10);
	s->store = store_init((int)strtol(argv[3], NULL, 10));
	assert(s->store);
	s->addr = argv[4];
	s->port = argv[5];
	s->fd = socket_init(s->addr, s->port);
	assert(s->fd != -1);
	pool = pool_init(POOL_SIZE);
	assert(pool);

	if (s->role == CONSISTENCY_ROLE_BACKUP) {
		s->sync_addr = NULL;
		s->sync_port = NULL;
		s->sync_fd = -1;
		s->backup_num = 0;
		s->backup_addr = NULL;
		s->backup_port = NULL;
		s->sync_q = NULL;
		s->sent_q = NULL;
		s->primary_addr = argv[6];
		s->primary_port = argv[7];
	} else {
		// Quick fix for sync_fd.
		int client_port = (int)strtol(s->port, NULL, 0) - 1;
		char client_port_str[MESSAGE_PORT_SIZE];
		sprintf(client_port_str, "%d", client_port);
		s->sync_addr = s->addr;
		s->sync_port = client_port_str;
		s->sync_fd = socket_init(s->sync_addr, s->sync_port);
		assert(s->sync_fd);

		s->backup_num = (argc - 6) / 2;
		s->backup_addr = malloc(sizeof(char *) * s->backup_num);
		s->backup_port = malloc(sizeof(char *) * s->backup_num);
		s->sync_q = malloc(sizeof(struct synchronization_queue) *
				   s->backup_num);
		s->sent_q = malloc(sizeof(struct synchronization_queue) *
				   s->backup_num);
		assert(s->backup_addr);
		assert(s->backup_port);
		assert(s->sync_q);
		assert(s->sent_q);
		for (int i = 0; i < s->backup_num; i++) {
			*(s->backup_addr + i) = argv[6 + i * 2];
			*(s->backup_port + i) = argv[6 + i * 2 + 1];
			*(s->sync_q + i) = synchronization_queue_init();
			*(s->sent_q + i) = synchronization_queue_init();
			assert(*(s->sync_q + i));
			assert(*(s->sent_q + i));

			pthread_t ignored;
			struct synchronize_sender_info *info =
				malloc(sizeof(struct synchronize_sender_info));
			assert(info);
			info->state = s;
			info->backup_i = i;
			pthread_create(&ignored, NULL, synchronize_sender,
				       info);
			pthread_create(&ignored, NULL, synchronize_refresher,
				       info);
		}
		s->primary_addr = NULL;
		s->primary_port = NULL;

		pthread_t ignored;
		pthread_create(&ignored, NULL, synchronize_receiver, s);
	}

	while (1) {
		struct request_handler_info *info =
			malloc(sizeof(struct request_handler_info));
		assert(info);
		info->state = s;
		info->msg = malloc(sizeof(struct message));
		assert(info->msg);

		if (socket_receive(s->fd, info->msg) == -1) {
			fprintf(stderr, "server: socket_receive() failed\n");
			free(info->msg);
			info->msg = NULL;
			free(info);
			info = NULL;
		} else {
			pool_add(pool, request_handler, info);
		}
	}

	// Ignore the step to release resources for now.

	return 0;
}

void *request_handler(void *arg)
{
	assert(arg);

	char *err_msg = NULL;
	struct consistency_state *s =
		((struct request_handler_info *)arg)->state;
	struct message *req = ((struct request_handler_info *)arg)->msg;

	if (s->role == CONSISTENCY_ROLE_BACKUP) {
		switch (s->semantics) {
		case CONSISTENCY_SEMANTICS_EVENTUAL:
			if (consistency_eventual_backup(s, req) != 0) {
				err_msg =
					"consistency_eventual_backup() failed";
				goto error;
			}
			break;
		case CONSISTENCY_SEMANTICS_READ_MY_WRITES:
			if (consistency_read_my_writes_backup(s, req) != 0) {
				err_msg =
					"consistency_read_my_writes_backup() failed";
				goto error;
			}
			break;
		case CONSISTENCY_SEMANTICS_MONOTONIC_READS:
			if (consistency_monotonic_reads_backup(s, req) != 0) {
				err_msg =
					"consistency_monotonic_reads_backup() failed";
				goto error;
			}
			break;
		default:
			break;
		}
	} else {
		switch (s->semantics) {
		case CONSISTENCY_SEMANTICS_EVENTUAL:
			if (consistency_eventual_primary(s, req) != 0) {
				err_msg =
					"consistency_eventual_primary() failed";
				goto error;
			}
			break;
		case CONSISTENCY_SEMANTICS_READ_MY_WRITES:
			if (consistency_read_my_writes_primary(s, req) != 0) {
				err_msg =
					"consistency_read_my_writes_primary() failed";
				goto error;
			}
			break;
		case CONSISTENCY_SEMANTICS_MONOTONIC_READS:
			if (consistency_monotonic_reads_primary(s, req) != 0) {
				err_msg =
					"consistency_monotonic_reads_primary() failed";
				goto error;
			}
			break;
		default:
			break;
		}
	}

clean:
	free(req);
	req = NULL;
	free(arg);
	arg = NULL;

	return NULL;

error:
	fprintf(stderr, "request_handler: %s\n", err_msg);
	goto clean;
}

void *synchronize_sender(void *arg)
{
	assert(arg);
	struct consistency_state *s =
		((struct synchronize_sender_info *)arg)->state;
	int backup_i = ((struct synchronize_sender_info *)arg)->backup_i;
	struct synchronization_queue *sync_q = *(s->sync_q + backup_i);
	struct synchronization_queue *sent_q = *(s->sent_q + backup_i);

	pthread_mutex_t sync_q_mutex;
	pthread_mutex_init(&sync_q_mutex, NULL);

	while (1) {
		pthread_mutex_lock(&sync_q_mutex);
		while (synchronization_queue_is_empty(sync_q))
			pthread_cond_wait(&sync_q->cond, &sync_q_mutex);
		pthread_mutex_unlock(&sync_q_mutex);

		pthread_mutex_lock(&sync_q->lock);
		pthread_mutex_lock(&sent_q->lock);

		struct synchronization_task *t =
			synchronization_queue_dequeue(sync_q);

		struct message *msg = malloc(sizeof(struct message));
		assert(msg);
		memset(msg, 0, sizeof(struct message));
		msg->type = MESSAGE_PRIMARY_SYNC;
		msg->index = t->index;
		msg->value = t->value;
		msg->version = t->version;
		strncpy(msg->addr, s->sync_addr, MESSAGE_ADDR_SIZE);
		strncpy(msg->port, s->sync_port, MESSAGE_PORT_SIZE);

		synchronization_queue_enqueue(sent_q, t);
		if (socket_send(s->sync_fd, msg, t->addr, t->port) != 0) {
			fprintf(stderr,
				"synchronize_sender: socket_send() failed.");
		} else {
			gettimeofday(&(t->sent_time), NULL);
#ifdef DEBUG
			printf("synchronize_sender: sync: backup %d, version %u, index %u, value %d\n",
			       backup_i, t->version, t->index, t->value);
#endif
		}

		pthread_mutex_unlock(&sent_q->lock);
		pthread_mutex_unlock(&sync_q->lock);

		free(msg);
		msg = NULL;
	}

	// Ignore the step to release resources for now.

	return NULL;
}

// Assume no errors for now.
void *synchronize_refresher(void *arg)
{
	assert(arg);
	struct consistency_state *s =
		((struct synchronize_sender_info *)arg)->state;
	int backup_i = ((struct synchronize_sender_info *)arg)->backup_i;
	struct synchronization_queue *sync_q = *(s->sync_q + backup_i);
	struct synchronization_queue *sent_q = *(s->sent_q + backup_i);

	struct synchronization_task *t = NULL;
	struct timeval now;
	long elapsed = 0;

	while (1) {
		sleep(REFRESH_TIME);

		pthread_mutex_lock(&sync_q->lock);
		pthread_mutex_lock(&sent_q->lock);

		while (synchronization_queue_is_empty(sent_q) == 0) {
			t = synchronization_queue_peek(sent_q);
			gettimeofday(&now, NULL);
			elapsed = (now.tv_sec - t->sent_time.tv_sec) * 1000 +
				  (now.tv_usec - t->sent_time.tv_usec) / 1000;
			if (elapsed > RESENT_TIME) {
				free(t);
				t = synchronization_queue_dequeue(sent_q);
#ifdef DEBUG
				printf("synchronize_refresher: move back: backup %d, version %u, index %u\n",
				       backup_i, t->version, t->index);
#endif
				synchronization_queue_enqueue(sync_q, t);
				pthread_cond_signal(&(sync_q->cond));
				t = NULL;
			} else {
				free(t);
				t = NULL;
				break;
			}
		}
		pthread_mutex_unlock(&sent_q->lock);
		pthread_mutex_unlock(&sync_q->lock);
	}

	return NULL;
}

void *synchronize_receiver(void *arg)
{
	assert(arg);
	struct consistency_state *s = (struct consistency_state *)arg;

	while (1) {
		struct request_handler_info *info =
			malloc(sizeof(struct request_handler_info));
		assert(info);
		info->state = s;
		info->msg = malloc(sizeof(struct message));
		assert(info->msg);

		if (socket_receive(s->sync_fd, info->msg) == -1) {
			fprintf(stderr,
				"synchronize_receiver: socket_receive() failed\n");
			free(info->msg);
			info->msg = NULL;
			free(info);
			info = NULL;
		} else {
			pool_add(pool, synchronize_handler, info);
		}
	}

	return NULL;
}

void *synchronize_handler(void *arg)
{
	assert(arg);

	char *err_msg = NULL;
	struct consistency_state *s =
		((struct request_handler_info *)arg)->state;
	struct message *req = ((struct request_handler_info *)arg)->msg;

	for (int i = 0; i < s->backup_num; i++) {
		if ((strcmp(*(s->backup_addr + i), req->addr) == 0) &&
		    (strcmp(*(s->backup_port + i), req->port) == 0)) {
			struct synchronization_queue *sent_q = *(s->sent_q + i);
			pthread_mutex_lock(&sent_q->lock);
			if (synchronization_queue_remove_task(
				    sent_q, req->index, req->version) != 0) {
				err_msg =
					"synchronization_queue_remove_task failed";
				goto error;
			} else {
#ifdef DEBUG
				printf("synchronize_handler: remove: backup %d, version %u, index %u\n",
				       i, req->version, req->index);
#endif
			}
			pthread_mutex_unlock(&sent_q->lock);

			break;
		}
	}

clean:
	free(req);
	req = NULL;
	free(arg);
	arg = NULL;

	return NULL;

error:
	fprintf(stderr, "synchronize_handler: %s\n", err_msg);
	goto clean;
}