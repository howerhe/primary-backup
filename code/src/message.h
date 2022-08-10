#ifndef MESSAGE_H
#define MESSAGE_H

// Message addr size and port size.
#define MESSAGE_ADDR_SIZE 16
#define MESSAGE_PORT_SIZE 6

/**
 * @brief Types for messages.
 *
 */
enum message_type {
	MESSAGE_CLIENT_READ,
	MESSAGE_CLIENT_WRITE,
	MESSAGE_SERVER_RES,
	MESSAGE_PRIMARY_SYNC,
	MESSAGE_BACKUP_ACK
};

// We can have functions like
//  int message_parse_addr(char *s, unsigned *parsed);
//  int message_parse_port(char *s, unsigned short *parsed);
//  int message_build_addr(unsigned parsed, char *s);
//  int message_build_port(unsigned short parsed, char *s);
// to optimize the space used by struct message. But they add more complexity
// including checking whether it's a valid IP address. So we can just ignore
// them for now.

/**
 * @brief Message for network communications through sockets.
 */
struct message {
	// Currently id is global to a process, and only used for synchronization.
	unsigned id;
	enum message_type type;
	unsigned index;
	int value;
	unsigned version;
	char addr[MESSAGE_ADDR_SIZE];
	char port[MESSAGE_PORT_SIZE];

	// For backup synchronization.
	unsigned client_message_id;
	unsigned thread_id;
	unsigned backup_id;

	// For other semantics...
	unsigned last_read;
	unsigned last_write;
};

// Probably a better approach is to have a factory function.
/**
 * @brief Set the global message id. It should be called when new struct message
 * created. When called, the id of the message should be set to the global
 * counter, which increases by 1.
 *
 * @param msg pointer to a message struct
 * @return  0 when no error occurs
 */
int message_set_id(struct message *msg);

#endif