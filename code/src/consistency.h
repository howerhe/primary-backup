#ifndef CONSISTENCY_H
#define CONSISTENCY_H

#include "message.h"
#include "store.h"
#include "synchronization.h"

#define CONSISTENCY_ROLE_BACKUP 0
#define CONSISTENCY_ROLE_PRIMARY 1

#define CONSISTENCY_SEMANTICS_EVENTUAL 0
#define CONSISTENCY_SEMANTICS_READ_MY_WRITES 1
#define CONSISTENCY_SEMANTICS_MONOTONIC_READS 2

/**
 * @brief The consistency state for servers.
 * 
 */
struct consistency_state {
	int role;
	int semantics;
	store_t store;
	char *addr;
	char *port;
	int fd;

	// For the primary server.
	char *sync_addr;
	char *sync_port;
	int sync_fd;
	int backup_num;
	char **backup_addr;
	char **backup_port;
	struct synchronization_queue **sync_q;
	struct synchronization_queue **sent_q;

	// For backup servers.
	char *primary_addr;
	char *primary_port;
};

/**
 * @brief Handle requests with eventual consistency semantics by a backup server.
 * There are only two valid requests to handle: client's read and primary's sync.
 * Other requests will be ignored.
 * If any of the internal operations fail, the servers won't respond back and jump to a new cycle.
 * 
 * @param s the consistency state
 * @param req the requst
 * @return int 0 when no errors occur
 */
int consistency_eventual_backup(const struct consistency_state *s,
				const struct message *req);

/**
 * @brief Handle requests with eventual consistency semantics by a primay server.
 * There are only two valid requests to handle: client's read and client's write.
 * Other requests will be ignored.
 * If the request is a client's write. New sync tasks will be added into the sync queues.
 * If any of the internal operations fail, the servers won't respond back and jump to a new cycle.
 * 
 * @param s the consistency state
 * @param req the requst
 * @return int 0 when no errors occur
 */
int consistency_eventual_primary(const struct consistency_state *s,
				 const struct message *req);

/**
 * @brief Handle requests with read-my-writes consistency semantics by a backup server.
 * There are only two valid requests to handle: client's read and primary's sync.
 * Other requests will be ignored.
 * If the version from the client is higher, the server will wait until it sees a newer version to return.
 * If any of the internal operations fail, the servers won't respond back and jump to a new cycle.
 * 
 * @param s the consistency state
 * @param req the requst
 * @return int 0 when no errors occur
 */
int consistency_read_my_writes_backup(const struct consistency_state *s,
				      const struct message *req);

/**
 * @brief Handle requests with read-my-writes consistency semantics by a primay server.
 * There are only two valid requests to handle: client's read and client's write.
 * Other requests will be ignored.
 * If the request is a client's write. New sync tasks will be added into the sync queues.
 * If any of the internal operations fail, the servers won't respond back and jump to a new cycle.
 * 
 * @param s the consistency state
 * @param req the requst
 * @return int 0 when no errors occur
 */
int consistency_read_my_writes_primary(const struct consistency_state *s,
				       const struct message *req);

/**
 * @brief Handle requests with monotonic reads consistency semantics by a backup server.
 * There are only two valid requests to handle: client's read and primary's sync.
 * Other requests will be ignored.
 * If the version from the client is higher, the server will wait until it sees a newer version to return.
 * If any of the internal operations fail, the servers won't respond back and jump to a new cycle.
 * 
 * @param s the consistency state
 * @param req the requst
 * @return int 0 when no errors occur
 */
int consistency_monotonic_reads_backup(const struct consistency_state *s,
				       const struct message *req);

/**
 * @brief Handle requests with monotonic reads consistency semantics by a primay server.
 * There are only two valid requests to handle: client's read and client's write.
 * Other requests will be ignored.
 * If the request is a client's write. New sync tasks will be added into the sync queues.
 * If any of the internal operations fail, the servers won't respond back and jump to a new cycle.
 * 
 * @param s the consistency state
 * @param req the requst
 * @return int 0 when no errors occur
 */
int consistency_monotonic_reads_primary(const struct consistency_state *s,
					const struct message *req);

#endif