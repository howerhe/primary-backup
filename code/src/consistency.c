#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "consistency.h"
#include "message.h"
#include "socket.h"
#include "store.h"
#include "synchronization.h"

#define BACKUP_RETRY_TIME 2

int consistency_eventual_backup(const struct consistency_state *s,
				const struct message *req)
{
	char *err_msg = NULL;
	int rv = -1;
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

	enum message_type type = req->type;
	unsigned index = req->index;
	int value = req->value;
	unsigned version = req->version;

	switch (type) {
	case MESSAGE_CLIENT_READ:
#ifdef DEBUG
		printf("consistency_eventual_backup: MESSAGE_CLIENT_READ\n");
#endif
		type = MESSAGE_SERVER_RESPONSE;
		if (store_read(s->store, index, &value, &version) != 0) {
			err_msg = "store_read() failed\n";
			goto error;
		}
#ifdef DEBUG
		printf("- after: index %u, value %d, version %u\n", index,
		       value, version);
#endif
		break;
	case MESSAGE_PRIMARY_SYNC:
#ifdef DEBUG
		printf("consistency_eventual_backup: MESSAGE_PRIMARY_SYNC\n");
#endif
		type = MESSAGE_BACKUP_ACK;
#ifdef DEBUG
		printf("- before: index %u, value %d, version %u\n", index,
		       value, version);
#endif
		if (store_synchronize(s->store, index, &value, &version) != 0) {
			err_msg = "store_synchronize() failed\n";
			goto error;
		}
#ifdef DEBUG
		printf("- after: index %u, value %d, version %u\n", index,
		       value, version);
#endif
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

	rv = 0;

clean:
	free(res);
	res = NULL;
	return rv;

error:
	if (err_msg != NULL)
		fprintf(stderr, "consistency_eventual_backup: %s\n", err_msg);
	else
		perror("consistency_eventual_backup");
	rv = -1;
	goto clean;
}

int consistency_eventual_primary(const struct consistency_state *s,
				 const struct message *req)
{
	char *err_msg = NULL;
	int rv = -1;
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

	enum message_type type = req->type;
	unsigned index = req->index;
	int value = req->value;
	unsigned version = req->version;

	switch (type) {
	case MESSAGE_CLIENT_WRITE:
#ifdef DEBUG
		printf("consistency_eventual_primary: MESSAGE_CLIENT_WRITE\n");
#endif
		type = MESSAGE_SERVER_RESPONSE;
#ifdef DEBUG
		printf("- before: index %u, value %d, version %u\n", index,
		       value, version);
#endif
		if (store_update(s->store, index, &value, &version) != 0) {
			err_msg = "store_update() failed\n";
			goto error;
		} else {
			for (int i = 0; i < s->backup_num; i++) {
				// Assume it will not fail for now.
				struct synchronization_task *t = malloc(
					sizeof(struct synchronization_task));
				assert(t);

				t->version = version;
				t->index = index;
				t->value = value;
				strncpy(t->addr, *(s->backup_addr + i),
					MESSAGE_ADDR_SIZE);
				strncpy(t->port, *(s->backup_port + i),
					MESSAGE_PORT_SIZE);
				t->next = NULL;
				pthread_mutex_lock(&(*(s->sync_q + i))->lock);
				synchronization_queue_enqueue(*(s->sync_q + i),
							      t);
				pthread_mutex_unlock(&(*(s->sync_q + i))->lock);
				pthread_cond_signal(&(*(s->sync_q + i))->cond);
			}
		}
#ifdef DEBUG
		printf("- after: index %u, value %d, version %u\n", index,
		       value, version);
#endif
		break;
	case MESSAGE_CLIENT_READ:
#ifdef DEBUG
		printf("consistency_eventual_primary: MESSAGE_CLIENT_READ\n");
#endif
		type = MESSAGE_SERVER_RESPONSE;
#ifdef DEBUG
		printf("- before: index %u, value %d, version %u\n", index,
		       value, version);
#endif
		if (store_read(s->store, index, &value, &version) != 0) {
			err_msg = "store_read() failed\n";
			goto error;
		}
#ifdef DEBUG
		printf("- after: index %u, value %d, version %u\n", index,
		       value, version);
#endif
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

	rv = 0;

clean:
	free(res);
	res = NULL;
	return rv;

error:
	if (err_msg != NULL)
		fprintf(stderr, "consistency_eventual_primary: %s\n", err_msg);
	else
		perror("consistency_eventual_primary");
	rv = -1;
	goto clean;
}

int consistency_read_my_writes_backup(const struct consistency_state *s,
				      const struct message *req)
{
	char *err_msg = NULL;
	int rv = -1;
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

	enum message_type type = req->type;
	unsigned index = req->index;
	int value = req->value;
	unsigned version = req->version;
	unsigned last_write = req->last_read;

	switch (type) {
	case MESSAGE_CLIENT_READ:
#ifdef DEBUG
		printf("consistency_eventual_backup: MESSAGE_CLIENT_READ\n");
#endif
		type = MESSAGE_SERVER_RESPONSE;
		int value_tmp;
		unsigned version_tmp;
		int loop_counter = 0;
		do {
			if (loop_counter > 0) {
				sleep(BACKUP_RETRY_TIME);
#ifdef DEBUG
				printf("- old version, retrying %d times\n",
				       loop_counter);
#endif
			}
			if (store_read(s->store, index, &value_tmp,
				       &version_tmp) != 0) {
				err_msg = "store_read() failed\n";
				goto error;
			}
			loop_counter++;
		} while (version_tmp < last_write);
		value = value_tmp;
		version = version_tmp;
#ifdef DEBUG
		printf("- after: index %u, value %d, version %u\n", index,
		       value, version);
#endif
		break;
	case MESSAGE_PRIMARY_SYNC:
#ifdef DEBUG
		printf("consistency_eventual_backup: MESSAGE_PRIMARY_SYNC\n");
#endif
		type = MESSAGE_BACKUP_ACK;
#ifdef DEBUG
		printf("- before: index %u, value %d, version %u\n", index,
		       value, version);
#endif
		if (store_synchronize(s->store, index, &value, &version) != 0) {
			err_msg = "store_synchronize() failed\n";
			goto error;
		}
#ifdef DEBUG
		printf("- after: index %u, value %d, version %u\n", index,
		       value, version);
#endif
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

	rv = 0;

clean:
	free(res);
	res = NULL;
	return rv;

error:
	if (err_msg != NULL)
		fprintf(stderr, "consistency_monotonic_reads_backup: %s\n",
			err_msg);
	else
		perror("consistency_monotonic_reads_backup");
	rv = -1;
	goto clean;
}

int consistency_read_my_writes_primary(const struct consistency_state *s,
				       const struct message *req)
{
	// Just use eventual_primary for now.
	return consistency_eventual_primary(s, req);
}

int consistency_monotonic_reads_backup(const struct consistency_state *s,
				       const struct message *req)
{
	char *err_msg = NULL;
	int rv = -1;
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

	enum message_type type = req->type;
	unsigned index = req->index;
	int value = req->value;
	unsigned version = req->version;
	unsigned last_read = req->last_read;

	switch (type) {
	case MESSAGE_CLIENT_READ:
#ifdef DEBUG
		printf("consistency_eventual_backup: MESSAGE_CLIENT_READ\n");
#endif
		type = MESSAGE_SERVER_RESPONSE;
		int value_tmp;
		unsigned version_tmp;
		int loop_counter = 0;
		do {
			if (loop_counter > 0) {
				sleep(BACKUP_RETRY_TIME);
#ifdef DEBUG
				printf("- old version, retrying %d times\n",
				       loop_counter);
#endif
			}
			if (store_read(s->store, index, &value_tmp,
				       &version_tmp) != 0) {
				err_msg = "store_read() failed\n";
				goto error;
			}
			loop_counter++;
		} while (version_tmp < last_read);
		value = value_tmp;
		version = version_tmp;
#ifdef DEBUG
		printf("- after: index %u, value %d, version %u\n", index,
		       value, version);
#endif
		break;
	case MESSAGE_PRIMARY_SYNC:
#ifdef DEBUG
		printf("consistency_eventual_backup: MESSAGE_PRIMARY_SYNC\n");
#endif
		type = MESSAGE_BACKUP_ACK;
#ifdef DEBUG
		printf("- before: index %u, value %d, version %u\n", index,
		       value, version);
#endif
		if (store_synchronize(s->store, index, &value, &version) != 0) {
			err_msg = "store_synchronize() failed\n";
			goto error;
		}
#ifdef DEBUG
		printf("- after: index %u, value %d, version %u\n", index,
		       value, version);
#endif
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

	rv = 0;

clean:
	free(res);
	res = NULL;
	return rv;

error:
	if (err_msg != NULL)
		fprintf(stderr, "consistency_monotonic_reads_backup: %s\n",
			err_msg);
	else
		perror("consistency_monotonic_reads_backup");
	rv = -1;
	goto clean;
}

int consistency_monotonic_reads_primary(const struct consistency_state *s,
					const struct message *req)
{
	// Just use eventual_primary for now.
	return consistency_eventual_primary(s, req);
}