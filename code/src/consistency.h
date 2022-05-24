#ifndef CONSISTENCY_H
#define CONSISTENCY_H

#include "message.h"
#include "store.h"

#define CONSISTENCY_ROLE_BACKUP 0
#define CONSISTENCY_ROLE_PRIMARY 1

#define CONSISTENCY_SEMANTICS_EVENTUAL 0
#define CONSISTENCY_SEMANTICS_READ_MY_WRITES 1
#define CONSISTENCY_SEMANTICS_MONOTONIC_READS 2

#define CONSISTENCY_PRIMARY_WAIT_TIME 5 // seconds
#define CONSISTENCY_BACKUP_WAIT_TIME 50 // micro seconds

/**
 * @brief Synchronization information of worker threads.
 *
 */
struct consistency_thread {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	struct message **msgs;
	unsigned latest_client_msg_id;
	unsigned *latest_msg_ids;
	int block_flag;
};

/**
 * @brief Information of servers for worker threads.
 *
 */
struct consistency_server_state {
	store_t store;
	char *addr;
	char *port;
	int fd;

	// For backup server.
	int backup_id;

	// For the primary server.
	int backup_num;
	char **backup_addr;
	char **backup_port;
	struct consistency_thread *shelf;
};

/**
 * @brief Bundled information of servers and message for worker threads.
 *
 */
struct consistency_handler_info {
	unsigned id;
	struct consistency_server_state *server;
	struct message *message;
};

/**
 * @brief Handle requests with eventual consistency semantics by a backup
 * server.
 *
 * @param info bundled info struct
 * @return void* 
 */
void *consistency_eventual_backup(void *info);

/**
 * @brief Handle requests with eventual consistency semantics by the primary
 * server.
 *
 * @param info bundled info struct
 * @return void* 
 */
void *consistency_eventual_primary(void *info);

/**
 * @brief Handle requests with read-my-writes consistency semantics by a backup
 * server.
 *
 * @param info bundled info struct
 * @return void* 
 */
void *consistency_read_my_writes_backup(void *info);

/**
 * @brief Handle requests with read-my-writes consistency semantics by the primary
 * server.
 *
 * @param info bundled info struct
 * @return void* 
 */
void *consistency_read_my_writes_primary(void *info);

/**
 * @brief Handle requests with monotonic reads consistency semantics by a backup
 * server.
 *
 * @param info bundled info struct
 * @return void* 
 */
void *consistency_monotonic_reads_backup(void *info);

/**
 * @brief Handle requests with monotonic reads consistency semantics by the primary
 * server.
 *
 * @param info bundled info struct
 * @return void* 
 */
void *consistency_monotonic_reads_primary(void *info);

#endif