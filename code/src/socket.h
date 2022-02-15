#ifndef SOCKET_H
#define SOCKET_H

#include "message.h"

/**
 * @brief Initialize a socket with the address and port.
 *
 * @param address socket address
 * @param port socket port
 * @return socket file descriptor, -1 when errors occur
 */
int socket_init(const char *address, const char *port);

/**
 * @brief Free and clean a socket.
 *
 * @param fd socket file descriptor
 */
void socket_free(int fd);

/**
 * @brief Receive a message through the socket.
 *
 * @param fd socket file descriptor
 * @param msg pointer to a message struct
 * @return 0 when no error occurs
 */
int socket_receive(int fd, struct message *msg);

/**
 * @brief Send a message through the socket with the destination address and
 * port.
 *
 * @param fd socket file descriptor
 * @param msg pointer to a message struct
 * @param address destination socket address
 * @param port destination socket port
 * @return 0 when no error occurs
 */
int socket_send(int fd, const struct message *msg, const char *address,
		const char *port);

#endif