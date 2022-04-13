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
#include "src/pool.h"

// #define DEBUG

int main(int argc, char *argv[])
{
	if (argc < 8) {
		fprintf(stderr,
			"usage for backup: ./server 0 sematics_code thread_num store_size"
			" self_addr self_port backup_id\n");
		fprintf(stderr,
			"usage for primary: ./server 1 sematics_code thread_num"
			" store_size self_addr self_port backup_addr backup_port ...\n");
		exit(EXIT_FAILURE);
	}

	int role = (int)strtol(argv[1], NULL, 10);

	if (role == 0 && argc != 8) {
		fprintf(stderr,
			"usage for backup: ./server 0 sematics_code thread_num store_size"
			" self_addr self_port backup_id\n");
		exit(EXIT_FAILURE);
	}
	if (role == 1 && (argc < 9 || argc % 2 == 0)) {
		fprintf(stderr,
			"usage for primary: ./server 1 sematics_code thread_num"
			" store_size self_addr self_port backup_addr backup_port ...\n");
		exit(EXIT_FAILURE);
	}

	struct consistency_server_state *s =
		malloc(sizeof(struct consistency_server_state));
	assert(s);
	int semantics = (int)strtol(argv[2], NULL, 10);
	int thread_num = (int)strtol(argv[3], NULL, 10);
	pool_t pool = pool_init(thread_num);
	assert(pool);
	s->store = store_init((int)strtol(argv[4], NULL, 10));
	assert(s->store);
	s->addr = argv[5];
	s->port = argv[6];
	s->fd = socket_init(s->addr, s->port);
	assert(s->fd != -1);

	if (role == CONSISTENCY_ROLE_BACKUP) {
		s->backup_id = (int)strtol(argv[7], NULL, 10);
		s->backup_num = 0;
		s->backup_addr = NULL;
		s->backup_port = NULL;
		s->shelf = NULL;
	} else {
		s->backup_id = 0;
		s->backup_num = (argc - 7) / 2;
		s->backup_addr = malloc(sizeof(char *) * s->backup_num);
		s->backup_port = malloc(sizeof(char *) * s->backup_num);
		assert(s->backup_addr);
		assert(s->backup_port);
		for (int i = 0; i < s->backup_num; i++) {
			s->backup_addr[i] = argv[7 + i * 2];
			s->backup_port[i] = argv[7 + i * 2 + 1];
		}

		s->shelf =
			malloc(sizeof(struct consistency_thread) * thread_num);
		assert(s->shelf);
		for (int i = 0; i < thread_num; i++) {
			s->shelf[i].block_flag = 1;
			assert(pthread_mutex_init(&(s->shelf[i].mutex), NULL) ==
			       0);
			assert(pthread_cond_init(&(s->shelf[i].cond), NULL) ==
			       0);
			s->shelf[i].msgs = malloc(sizeof(struct message *) *
						  s->backup_num);
			assert(s->shelf[i].msgs);
			s->shelf[i].latest_client_msg_id = 0;
			s->shelf[i].latest_msg_ids =
				malloc(sizeof(unsigned) * s->backup_num);
			assert(s->shelf[i].latest_msg_ids);
			for (int j = 0; j < s->backup_num; j++) {
				s->shelf[i].msgs[j] = NULL;
				s->shelf[i].latest_msg_ids[j] = 0;
			}
			s->shelf[i].block_flag = 1;
		}
	}

	while (1) {
		struct consistency_handler_info *info =
			malloc(sizeof(struct consistency_handler_info));
		assert(info);
		memset(info, 0, sizeof(struct consistency_handler_info));
		info->server = s;
		info->message = malloc(sizeof(struct message));
		assert(info->message);

		if (socket_receive(s->fd, info->message) == -1) {
			fprintf(stderr, "server: socket_receive() failed\n");
			free(info->message);
			info->message = NULL;
			free(info);
			info = NULL;
			continue;
		}

#ifdef DEBUG
		printf("new msg id %u, type %d, index %u, value %d, version %u, from %s:%s, \n",
		       info->message->id, info->message->type,
		       info->message->index, info->message->value,
		       info->message->version, info->message->addr,
		       info->message->port);
#endif

		if (role == CONSISTENCY_ROLE_BACKUP) {
			pool_add(pool, consistency_eventual_backup, info);
		} else if (info->message->type == MESSAGE_BACKUP_ACK) {
			struct message *msg = info->message;
			struct consistency_thread *shelf =
				&(s->shelf[msg->thread_id]);

			pthread_mutex_lock(&(shelf->mutex));
			if (msg->client_message_id !=
				    shelf->latest_client_msg_id ||
			    msg->id < shelf->latest_msg_ids[msg->backup_id]) {
				pthread_mutex_unlock(&(shelf->mutex));
#ifdef DEBUG
				printf("id %d ignored.\n", info->message->id);
#endif
				shelf = NULL;
				msg = NULL;
				free(info->message);
				info->message = NULL;
				free(info);
				info = NULL;
				continue;
			}

			shelf->latest_msg_ids[info->message->backup_id] =
				msg->id;
			shelf->block_flag = 0;

			// Need to free old message on the shelf.
			free(shelf->msgs[msg->backup_id]);
			shelf->msgs[msg->backup_id] = NULL;

			shelf->msgs[msg->backup_id] = info->message;
			pthread_cond_signal(&(shelf->cond));
			pthread_mutex_unlock(&(shelf->mutex));

			// Need to free info.
			shelf = NULL;
			msg = NULL;
			free(info);
			info = NULL;
		} else {
			// need to have some jump table to choose the correct semantics
			pool_add(pool, consistency_eventual_primary, info);
		}
	}
	// Ignore steps to release resources for now.

	return 0;
}