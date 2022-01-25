#ifndef SOCKET_H
#define SOCKET_H

#include "message.h"

/**
 * @brief Initialize a socket with an address and a port.
 * 
 * @param address the address
 * @param port the port
 * @return int representing the socket file descriptor, -1 when errors occur
 */
int socket_init(const char *address, const char *port);

/**
 * @brief Clean up a socket.
 * 
 * @param fd the socket file descriptor
 */
void socket_free(int fd);

/**
 * @brief Receive a message through a socket.
 * 
 * @param fd the socket file descriptor
 * @param msg the pointer to the struct message
 * @return int 0 when no error occurs
 */
int socket_receive(int fd, struct message *msg);

/**
 * @brief Send a message through a socket with a destination address and port.
 * 
 * @param fd the socket file descriptor
 * @param msg the pointer to the struct message
 * @param address the address
 * @param port the port
 * @return int 0 when no error occurs
 */
int socket_send(int fd, const struct message *msg, const char *address,
		const char *port);

#endif