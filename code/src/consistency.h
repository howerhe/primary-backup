#ifndef CONSISTENCY_H
#define CONSISTENCY_H

#include "message.h"
#include "store.h"

// Roles for servers.
#define CONSISTENCY_ROLE_BACKUP 0
#define CONSISTENCY_ROLE_PRIMARY 1

// Codes for semantics.
#define CONSISTENCY_SEMANTICS_EVENTUAL 0
#define CONSISTENCY_SEMANTICS_READ_MY_WRITES 1
#define CONSISTENCY_SEMANTICS_MONOTONIC_READS 2

// Time constants for waiting time.
#define CONSISTENCY_PRIMARY_WAIT_TIME 5 // seconds
#define CONSISTENCY_BACKUP_WAIT_TIME 50 // micro seconds

/**
 * @brief Synchronization information of worker threads in primary.
 *
 */
struct consistency_thread {
	pthread_mutex_t mutex; // For cond_timed_wait.
	pthread_cond_t cond; // For cond_timed_wait.
	struct message **msgs; // Message queue for synchronization
						   // acknowledgement from backups.
	unsigned latest_client_msg_id; // To filter out outdated messages.
	unsigned *latest_msg_ids; // To filter out outdated messages.
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
	struct consistency_thread *shelf; // Each worker thread has its space to 
									  // store and pass synchronization
									  // information.
};

/**
 * @brief Bundled information of servers and message for worker threads.
 *
 */
struct consistency_handler_info {
	unsigned id; // Required by pool.h.
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