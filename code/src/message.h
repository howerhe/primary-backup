#ifndef MESSAGE_H
#define MESSAGE_H

// Message addr size and port size.
#define MESSAGE_ADDR_SIZE 16
#define MESSAGE_PORT_SIZE 6

/**
 * @brief The message type.
 * 
 */
enum message_type {
	MESSAGE_CLIENT_READ,
	MESSAGE_CLIENT_WRITE,
	MESSAGE_SERVER_RESPONSE,
	MESSAGE_PRIMARY_SYNC,
	MESSAGE_BACKUP_ACK
};

// We can have functions like
// 	int message_parse_addr(char *s, unsigned *parsed);
// 	int message_parse_port(char *s, unsigned short *parsed);
// 	int message_build_addr(unsigned parsed, char *s);
// 	int message_build_port(unsigned short parsed, char *s);
// to optimize the space used by struct message. But they add more complexity
// including checking whether it's a valid IP address. So we can just ignore
// them for now.

/**
 * @brief The message for network communications through sockets.
 */
struct message {
	enum message_type type;
	unsigned index;
	int value;
	unsigned version;
	char addr[MESSAGE_ADDR_SIZE];
	char port[MESSAGE_PORT_SIZE];
	unsigned last_read;
	unsigned last_write;
};

#endif