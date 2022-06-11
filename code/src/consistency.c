#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "consistency.h"
#include "message.h"
#include "socket.h"
#include "store.h"

// #define DEBUG

void *consistency_eventual_backup(void *info)
{
	struct consistency_server_state *s =
		((struct consistency_handler_info *)info)->server;
	struct message *req =
		((struct consistency_handler_info *)info)->message;
	free(info);
	info = NULL;

	char *err_msg = NULL;
	struct message *res;

	if (s == NULL) {
		err_msg = "state is NULL";
		goto error;
	}
	if (req == NULL) {
		err_msg = "req is NULL";
		goto error;
	}
	res = malloc(sizeof(struct message));
	if (res == NULL)
		goto error;
	memset(res, 0, sizeof(struct message));
	assert(message_set_id(res) == 0);

	enum message_type type = req->type;
	unsigned index = req->index;
	int value = req->value;
	unsigned version = req->version;

	switch (type) {
	case MESSAGE_CLIENT_READ:
#ifdef DEBUG
		printf("MESSAGE_CLIENT_READ\n");
#endif
		type = MESSAGE_SERVER_RES;
		if (store_read(s->store, index, &value, &version) != 0) {
			err_msg = "store_read() failed\n";
			goto error;
		}
		break;
	case MESSAGE_PRIMARY_SYNC:
#ifdef DEBUG
		printf("MESSAGE_PRIMARY_SYNC\n");
#endif
		type = MESSAGE_BACKUP_ACK;
		if (store_synchronize(s->store, index, &value, &version) != 0) {
			err_msg = "store_synchronize() failed\n";
			goto error;
		}
		break;
	default:
		err_msg = "unsupported message type\n";
		goto error;
		break;
	}

	res->type = type;
	res->index = index;
	res->value = value;
	res->version = version;
	strncpy(res->addr, s->addr, MESSAGE_ADDR_SIZE);
	strncpy(res->port, s->port, MESSAGE_PORT_SIZE);

	if (req->type == MESSAGE_PRIMARY_SYNC) {
		res->client_message_id = req->client_message_id;
		res->thread_id = req->thread_id;
		res->backup_id = s->backup_id;
	}

	if (socket_send(s->fd, res, req->addr, req->port) != 0) {
		err_msg = "cannot send message";
		goto error;
	}
#ifdef DEBUG
	printf("sent\n");
#endif

clean:
	free(res);
	res = NULL;
	free(req);
	req = NULL;
	return NULL;

error:
	if (err_msg != NULL)
		fprintf(stderr, "consistency_eventual_backup: %s\n", err_msg);
	else
		perror("consistency_eventual_backup");
	goto clean;
}

void *consistency_eventual_primary(void *info)
{
	unsigned tid = ((struct consistency_handler_info *)info)->id;
	struct consistency_server_state *s =
		((struct consistency_handler_info *)info)->server;
	struct message *req =
		((struct consistency_handler_info *)info)->message;
	free(info);
	info = NULL;

	char *err_msg = NULL;
	struct message *res;

	if (s == NULL) {
		err_msg = "state is NULL";
		goto error;
	}
	if (req == NULL) {
		err_msg = "req is NULL";
		goto error;
	}
	res = malloc(sizeof(struct message));
	if (res == NULL)
		goto error;
	memset(res, 0, sizeof(struct message));
	assert(message_set_id(res) == 0);

	enum message_type type = req->type;
	unsigned index = req->index;
	int value = req->value;
	unsigned version = req->version;

	switch (type) {
	case MESSAGE_CLIENT_READ:
#ifdef DEBUG
		printf("MESSAGE_CLIENT_READ\n");
#endif
		type = MESSAGE_SERVER_RES;
		if (store_read(s->store, index, &value, &version) != 0) {
			err_msg = "store_read() failed\n";
			goto error;
		}
		break;
	case MESSAGE_CLIENT_WRITE:
#ifdef DEBUG
		printf("MESSAGE_CLIENT_WRITE\n");
#endif
		type = MESSAGE_SERVER_RES;
		if (store_update(s->store, index, &value, &version) != 0) {
			err_msg = "store_update() failed\n";
			goto error;
		}
		break;
	default:
		err_msg = "unsupported message type\n";
		goto error;
		break;
	}

	res->type = type;
	res->index = index;
	res->value = value;
	res->version = version;
	strncpy(res->addr, s->addr, MESSAGE_ADDR_SIZE);
	strncpy(res->port, s->port, MESSAGE_PORT_SIZE);

	if (socket_send(s->fd, res, req->addr, req->port) != 0) {
		err_msg = "cannot send message";
		goto error;
	}
#ifdef DEBUG
	printf("client response sent\n");
#endif

	if (req->type == MESSAGE_CLIENT_WRITE) {
		struct consistency_thread *shelf = &(s->shelf[tid]);
		shelf->latest_client_msg_id = req->client_message_id;

		struct message *sync = malloc(sizeof(struct message));
		assert(sync);

		// Send out first round synchronization messages.
		memset(sync, 0, sizeof(struct message));
		sync->type = MESSAGE_PRIMARY_SYNC;
		sync->index = index;
		sync->value = value;
		sync->version = version;
		strncpy(sync->addr, s->addr, MESSAGE_ADDR_SIZE);
		strncpy(sync->port, s->port, MESSAGE_PORT_SIZE);
		sync->client_message_id = req->client_message_id;
		sync->thread_id = tid;
		for (int i = 0; i < s->backup_num; i++) {
			assert(message_set_id(sync) == 0);
			if (socket_send(s->fd, sync, s->backup_addr[i],
					s->backup_port[i]) != 0) {
				err_msg = "cannot send message";
				goto error;
			}
		}

		int synced_num = 0;
		int *synced_flag = malloc(sizeof(int) * s->backup_num);
		assert(synced_flag);
		for (int i = 0; i < s->backup_num; i++)
			synced_flag[i] = 0;
		struct timespec t;
		clock_gettime(CLOCK_REALTIME, &t);
		t.tv_sec += CONSISTENCY_PRIMARY_WAIT_TIME;

		while (1) {
			pthread_mutex_lock(&(shelf->mutex));
			int rv = 0;
			while (shelf->block_flag == 1) {
				rv = pthread_cond_timedwait(
					&(shelf->cond), &(shelf->mutex), &t);
				if (rv == ETIMEDOUT)
					break;
			}
			shelf->block_flag = 1;

#ifdef DEBUG
			printf("SYNC waked up: ");
			if (rv == ETIMEDOUT)
				printf("timed out\n");
			else
				printf("by backup\n");
#endif

			for (int i = 0; i < s->backup_num; i++) {
				if (rv == ETIMEDOUT && synced_flag[i] == 0) {
					assert(message_set_id(sync) == 0);
					assert(socket_send(s->fd, sync,
							   s->backup_addr[i],
							   s->backup_port[i]) ==
					       0);
				} else if (rv == 0 && shelf->msgs[i] != NULL &&
					   synced_flag[i] == 0) {
					synced_flag[i] = 1;
					synced_num++;
					free(shelf->msgs[i]);
					shelf->msgs[i] = NULL;
				}
			}

			pthread_mutex_unlock(&(s->shelf[tid].mutex));
			if (synced_num == s->backup_num)
				break;
		}

		free(synced_flag);
		synced_flag = NULL;
		free(sync);
		sync = NULL;
		shelf = NULL;
	}
#ifdef DEBUG
	printf("sync finished\n");
#endif

clean:
	free(res);
	res = NULL;
	free(req);
	req = NULL;
	return NULL;

error:
	if (err_msg != NULL)
		fprintf(stderr, "consistency_eventual_primary: %s\n", err_msg);
	else
		perror("consistency_eventual_primary");
	goto clean;
}

void *consistency_read_my_writes_backup(void *info)
{
	struct consistency_server_state *s =
		((struct consistency_handler_info *)info)->server;
	struct message *req =
		((struct consistency_handler_info *)info)->message;
	free(info);
	info = NULL;

	char *err_msg = NULL;
	struct message *res;

	if (s == NULL) {
		err_msg = "state is NULL";
		goto error;
	}
	if (req == NULL) {
		err_msg = "req is NULL";
		goto error;
	}
	res = malloc(sizeof(struct message));
	if (res == NULL)
		goto error;
	memset(res, 0, sizeof(struct message));
	assert(message_set_id(res) == 0);

	enum message_type type = req->type;
	unsigned index = req->index;
	int value = req->value;
	unsigned version = req->version;
	unsigned last_write = req->last_write;

	switch (type) {
	case MESSAGE_CLIENT_READ:
#ifdef DEBUG
		printf("MESSAGE_CLIENT_READ\n");
#endif
		type = MESSAGE_SERVER_RES;
		int value_tmp;
		unsigned version_tmp;
		int loop_counter = 0;
		do {
			if (loop_counter > 0)
				usleep(CONSISTENCY_BACKUP_WAIT_TIME);
			if (store_read(s->store, index, &value_tmp,
				       &version_tmp) != 0) {
				err_msg = "store_read() failed\n";
				goto error;
			}
			loop_counter++;
		} while (version_tmp < last_write);
		value = value_tmp;
		version = version_tmp;
		break;
	case MESSAGE_PRIMARY_SYNC:
#ifdef DEBUG
		printf("MESSAGE_PRIMARY_SYNC\n");
#endif
		type = MESSAGE_BACKUP_ACK;
		if (store_synchronize(s->store, index, &value, &version) != 0) {
			err_msg = "store_synchronize() failed\n";
			goto error;
		}
		break;
	default:
		err_msg = "unsupported message type\n";
		goto error;
		break;
	}

	res->type = type;
	res->index = index;
	res->value = value;
	res->version = version;
	strncpy(res->addr, s->addr, MESSAGE_ADDR_SIZE);
	strncpy(res->port, s->port, MESSAGE_PORT_SIZE);

	if (req->type == MESSAGE_PRIMARY_SYNC) {
		res->client_message_id = req->client_message_id;
		res->thread_id = req->thread_id;
		res->backup_id = s->backup_id;
	}

	if (socket_send(s->fd, res, req->addr, req->port) != 0) {
		err_msg = "cannot send message";
		goto error;
	}
#ifdef DEBUG
	printf("sent\n");
#endif

clean:
	free(res);
	res = NULL;
	free(req);
	req = NULL;
	return NULL;

error:
	if (err_msg != NULL)
		fprintf(stderr, "consistency_read_my_writes_backup: %s\n",
			err_msg);
	else
		perror("consistency_read_my_writes_backup");
	goto clean;
}

void *consistency_read_my_writes_primary(void *info)
{
	return consistency_eventual_primary(info);
}

void *consistency_monotonic_reads_backup(void *info)
{
	struct consistency_server_state *s =
		((struct consistency_handler_info *)info)->server;
	struct message *req =
		((struct consistency_handler_info *)info)->message;
	free(info);
	info = NULL;

	char *err_msg = NULL;
	struct message *res;

	if (s == NULL) {
		err_msg = "state is NULL";
		goto error;
	}
	if (req == NULL) {
		err_msg = "req is NULL";
		goto error;
	}
	res = malloc(sizeof(struct message));
	if (res == NULL)
		goto error;
	memset(res, 0, sizeof(struct message));
	assert(message_set_id(res) == 0);

	enum message_type type = req->type;
	unsigned index = req->index;
	int value = req->value;
	unsigned version = req->version;
	unsigned last_read = req->last_read;

	switch (type) {
	case MESSAGE_CLIENT_READ:
#ifdef DEBUG
		printf("MESSAGE_CLIENT_READ\n");
#endif
		type = MESSAGE_SERVER_RES;
		int value_tmp;
		unsigned version_tmp;
		int loop_counter = 0;
		do {
			if (loop_counter > 0)
				usleep(CONSISTENCY_BACKUP_WAIT_TIME);
			if (store_read(s->store, index, &value_tmp,
				       &version_tmp) != 0) {
				err_msg = "store_read() failed\n";
				goto error;
			}
			loop_counter++;
		} while (version_tmp < last_read);
		value = value_tmp;
		version = version_tmp;
		break;
	case MESSAGE_PRIMARY_SYNC:
#ifdef DEBUG
		printf("MESSAGE_PRIMARY_SYNC\n");
#endif
		type = MESSAGE_BACKUP_ACK;
		if (store_synchronize(s->store, index, &value, &version) != 0) {
			err_msg = "store_synchronize() failed\n";
			goto error;
		}
		break;
	default:
		err_msg = "unsupported message type\n";
		goto error;
		break;
	}

	res->type = type;
	res->index = index;
	res->value = value;
	res->version = version;
	strncpy(res->addr, s->addr, MESSAGE_ADDR_SIZE);
	strncpy(res->port, s->port, MESSAGE_PORT_SIZE);

	if (req->type == MESSAGE_PRIMARY_SYNC) {
		res->client_message_id = req->client_message_id;
		res->thread_id = req->thread_id;
		res->backup_id = s->backup_id;
	}

	if (socket_send(s->fd, res, req->addr, req->port) != 0) {
		err_msg = "cannot send message";
		goto error;
	}
#ifdef DEBUG
	printf("sent\n");
#endif

clean:
	free(res);
	res = NULL;
	free(req);
	req = NULL;
	return NULL;

error:
	if (err_msg != NULL)
		fprintf(stderr, "consistency_monotonic_reads_backup: %s\n",
			err_msg);
	else
		perror("consistency_monotonic_reads_backup");
	goto clean;
}

void *consistency_monotonic_reads_primary(void *info)
{
	return consistency_eventual_primary(info);
}