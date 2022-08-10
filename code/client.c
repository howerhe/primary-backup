// total running time unit: ms
// remote elapsed time unit: us

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "src/message.h"
#include "src/socket.h"

#include "zipf/zipf.h"

// #define DEBUG

// Codes for semantics.
#define SEMANTICS_EVENTUAL 0
#define SEMANTICS_READ_MY_WRITES 1
#define MONOTONIC_READS 2

// Constant needed for zipf distribution.
#define ZIPF_ALPHA 1

// Block store for clients. Each client thread has its own store. No concurrent
// access.
struct local_store {
	int value;
	unsigned version;
	unsigned last_read;
	unsigned last_write;
};

// Experiment record. Kept in the memory instead of standard output for
// efficiency.
struct log {
	int index;
	unsigned operation;			// 0 = read, 1 = update
	int n_or_b;					// new value or backup number
	int local_value;			// value before reade/update operation
	unsigned local_version;		// version before reade/update operation
	int remote_value; 			// value from remote server
	unsigned remote_version;	// version from remote server
	long long remote_elapsed;	// micro seconds taken to finish an operation
};

// Information for client thread.
struct info {
	struct log *log;
	struct local_store *store;
	char *port;
	int fd;
};

// Routine function for client thread.
void *client_routine(void *arg);

int semantics_code;
int thread_num;
int msg_num;
int store_size;
char *file_name_prefix;
char *self_addr;
char *self_port;
char *primary_addr;
char *primary_port;
int backup_num;
char **backup_addr;
char **backup_port;

int main(int argc, char *argv[])
{
	if (argc < 12 || argc % 2 != 0) {
		fprintf(stderr,
			"usage: ./client semantics_code thread_num msg_num store_size file_name_prefix self_addr self_port primary_addr primary_port backup_addr backup_port ...\n");
		exit(EXIT_FAILURE);
	}

	semantics_code = (int)strtol(argv[1], NULL, 10);
	thread_num = (int)strtol(argv[2], NULL, 10);
	msg_num = (int)strtol(argv[3], NULL, 10);
	store_size = (int)strtol(argv[4], NULL, 10);
	file_name_prefix = argv[5];
	self_addr = argv[6];
	self_port = argv[7];
	primary_addr = argv[8];
	primary_port = argv[9];
	backup_num = (argc - 10) / 2;
	backup_addr = malloc(sizeof(char *) * backup_num);
	backup_port = malloc(sizeof(char *) * backup_num);
	assert(backup_addr);
	assert(backup_port);
	for (int i = 0; i < backup_num; i++) {
		*(backup_addr + i) = argv[10 + i * 2];
		*(backup_port + i) = argv[10 + i * 2 + 1];
	}

	pthread_t *thread_ids = malloc(sizeof(pthread_t) * thread_num);
	struct info *infos = malloc(sizeof(struct info) * thread_num);
	assert(thread_ids);
	assert(infos);

	for (int i = 0; i < thread_num; i++) {
		struct info *info = infos + i;
		info->log = malloc(sizeof(struct log) * msg_num);
		info->store = malloc(sizeof(struct local_store) * store_size);
		info->port = malloc(sizeof(char) * MESSAGE_PORT_SIZE);
		assert(info->log);
		assert(info->store);
		assert(info->port);

		for (int j = 0; j < store_size; j++) {
			(info->store + j)->value = 0;
			(info->store + j)->version = 0;
			(info->store + j)->last_read = 0;
			(info->store + j)->last_write = 0;
		}

		// Different ports are used for different thread to avoid mixing of
		// messages from servers.
		int port = (int)strtol(self_port, NULL, 0);
		sprintf(info->port, "%d", port - i);

		info->fd = socket_init(self_addr, info->port);
		assert(info->fd != -1);
	}

	srand(time(NULL));
	init_zipf(ZIPF_ALPHA, store_size - 1);

	long long elapsed;
	struct timeval start, end;
	gettimeofday(&start, NULL);

	for (int i = 0; i < thread_num; i++)
		pthread_create(thread_ids + i, NULL, client_routine, infos + i);

	for (int i = 0; i < thread_num; i++)
		pthread_join(*(thread_ids + i), NULL);

	gettimeofday(&end, NULL);
	elapsed = (end.tv_sec - start.tv_sec) * 1000 +
		  (end.tv_usec - start.tv_usec) / 1000;

	FILE *fp_time = fopen("client_time.txt", "w");
	assert(fp_time);
	fprintf(fp_time, "%lld\n", elapsed);
	fclose(fp_time);
	fp_time = NULL;

	for (int i = 0; i < thread_num; i++) {
		char file_name[20];
		sprintf(file_name, "%s_%d.csv", file_name_prefix, i);
		FILE *fp = fopen(file_name, "w");
		assert(fp);
		fprintf(fp, "round, index, ");
		fprintf(fp, "operation, new_value/backup_no, ");
		fprintf(fp, "local_value, local_version, ");
		fprintf(fp, "remote_value, remote_version, remote_time\n");

		for (int j = 0; j < msg_num; j++) {
			struct log *cur = (infos + i)->log + j;
			fprintf(fp, "%d, %u, %d, %d, %d, %u, %d, %u, %lld\n", j,
				cur->index, cur->operation, cur->n_or_b,
				cur->local_value, cur->local_version,
				cur->remote_value, cur->remote_version,
				cur->remote_elapsed);
		}

		fclose(fp);
		fp = NULL;
	}

	destroy_zipf();

	for (int i = 0; i < thread_num; i++) {
		socket_free((infos + i)->fd);
		(infos + i)->fd = -1;
		free((infos + i)->port);
		(infos + i)->port = NULL;
		free((infos + i)->store);
		(infos + i)->store = NULL;
		free((infos + i)->log);
		(infos + i)->log = NULL;
	}
	free(infos);
	infos = NULL;
	free(thread_ids);
	thread_ids = NULL;
	free(backup_port);
	backup_port = NULL;
	free(backup_addr);
	backup_addr = NULL;

	return 0;
}

void *client_routine(void *arg)
{
	struct log *log = ((struct info *)arg)->log;
	struct local_store *store = ((struct info *)arg)->store;
	char *port = ((struct info *)arg)->port;
	int fd = ((struct info *)arg)->fd;

	// Note that the thread would hang if there's no response from servers.
	for (int i = 0; i < msg_num; i++) {
#ifdef DEBUG
		printf("round %d\n", i);
#endif

		struct timeval start, end;
		struct message req, res;
		memset(&req, 0, sizeof(req));
		memset(&res, 0, sizeof(req));

		// Index is from zipf distribution.
		// unsigned index = rand() % store_size;
		unsigned index = (unsigned)zipf_blk();
		// Operation (read, write) is from normal distribution.
		int operation = rand() % 2;
		// New value or backup number is from normal distribution (will be seen
		// later).
		int n_or_b;

		int local_value, remote_value;
		unsigned local_version, remote_version;
		long long remote_elapsed;

		local_value = (store + index)->value;
		local_version = (store + index)->version;
		req.index = index;
		req.version = local_version;
		strncpy(req.addr, self_addr, MESSAGE_ADDR_SIZE);
		strncpy(req.port, port, MESSAGE_PORT_SIZE);
		req.last_read = (store + index)->last_read;
		req.last_write = (store + index)->last_write;

		if (operation == 0) { // Read.
			int backup_no = rand() % backup_num;
			n_or_b = backup_no;
			req.type = MESSAGE_CLIENT_READ;
			req.value = 0;

			gettimeofday(&start, NULL);

			assert(message_set_id(&req) == 0);
			req.client_message_id = req.id;
			if (socket_send(fd, &req, *(backup_addr + backup_no),
					*(backup_port + backup_no)) != 0) {
				fprintf(stderr,
					"- cannot send message to the backup (%d)\n",
					backup_no);
				continue;
			}

			if (socket_receive(fd, &res) != 0) {
				fprintf(stderr,
					"- cannot receive message from the backup (%d)\n",
					backup_no);
				continue;
			}

			gettimeofday(&end, NULL);
			remote_elapsed = (end.tv_sec - start.tv_sec) * 1000000 +
					 (end.tv_usec - start.tv_usec);
			remote_value = res.value;
			remote_version = res.version;
			// if (remote_version > local_version) {
			(store + index)->value = remote_value;
			(store + index)->version = remote_version;
			(store + index)->last_read = remote_version;
			// }
		} else { // Update.
			// Set a boundary to the new value.
			int new_value = rand() % 10 + 1;
			n_or_b = new_value;
			req.type = MESSAGE_CLIENT_WRITE;
			req.value = new_value;

			gettimeofday(&start, NULL);

			assert(message_set_id(&req) == 0);
			req.client_message_id = req.id;
			if (socket_send(fd, &req, primary_addr, primary_port) !=
			    0) {
				fprintf(stderr,
					"- cannot send message to the primary\n");
				continue;
			}

			if (socket_receive(fd, &res) != 0) {
				fprintf(stderr,
					"- cannot receive message from the primary\n");
				continue;
			}

			gettimeofday(&end, NULL);
			remote_elapsed = (end.tv_sec - start.tv_sec) * 1000000 +
					 (end.tv_usec - start.tv_usec);
			remote_value = res.value;
			remote_version = res.version;

			// TODO: some bugs to fix.
			// if (semantics_code == SEMANTICS_READ_MY_WRITES) {
			// if (remote_version > local_version) {
			(store + index)->value = remote_value;
			(store + index)->version = remote_version;
			(store + index)->last_write = remote_version;
			// 	}
			// }
		}

		// Store the result.
		struct log *cur = log + i;
		cur->index = index;
		cur->operation = operation;
		cur->n_or_b = n_or_b;
		cur->local_value = local_value;
		cur->local_version = local_version;
		cur->remote_value = remote_value;
		cur->remote_version = remote_version;
		cur->remote_elapsed = remote_elapsed;
	}

	return NULL;
}