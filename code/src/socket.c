#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "message.h"
#include "socket.h"

struct addrinfo *ai_create(const char *address, const char *port);
void ai_destroy(struct addrinfo *ai);

int socket_init(const char *address, const char *port)
{
	char *err_msg = NULL;
	struct addrinfo *ai = NULL;
	int rv = -1;
	int fd = -1;

	ai = ai_create(address, port);
	if (ai == NULL) {
		err_msg = "ai_create() fails";
		goto error;
	}

	fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
	if (fd == -1)
		goto error;

	rv = bind(fd, ai->ai_addr, ai->ai_addrlen);
	if (rv != 0)
		goto error;

clean:
	ai_destroy(ai);
	ai = NULL;

	return fd;

error:
	if (err_msg != NULL)
		fprintf(stderr, "socket_init: %s\n", err_msg);
	else
		perror("socket_init");
	socket_free(fd);
	fd = -1;
	goto clean;
}

void socket_free(int fd)
{
	if (fd != -1) {
		close(fd);
		fd = -1;
	}
}

int socket_receive(int fd, struct message *msg)
{
	char *err_msg = NULL;
	size_t received = -1;
	size_t expected = sizeof(struct message);

	if (fd == -1) {
		err_msg = "socket is invalid";
		goto error;
	}
	if (msg == NULL) {
		err_msg = "msg is NULL";
		goto error;
	}

	received = recvfrom(fd, msg, expected, 0, NULL, NULL);
	if (received == -1)
		goto error;
	else if (received < expected) {
		err_msg = "cannot receive the whole message";
		goto error;
	}

	return 0;

error:
	if (err_msg != NULL)
		fprintf(stderr, "socket_receive: %s\n", err_msg);
	else
		perror("socket_receive");
	return -1;
}

int socket_send(int fd, const struct message *msg, const char *address,
		const char *port)
{
	char *err_msg = NULL;
	struct addrinfo *ai = NULL;
	size_t size = sizeof(struct message);
	int rv = -1;

	if (fd == -1) {
		err_msg = "socket is invalid";
		goto error;
	}
	if (msg == NULL) {
		err_msg = "msg is NULL";
		goto error;
	}

	ai = ai_create(address, port);
	if (ai == NULL) {
		err_msg = "ai_create() fails";
		goto error;
	}

	rv = sendto(fd, msg, size, 0, ai->ai_addr, ai->ai_addrlen);
	if (rv == -1)
		goto error;
	else if (rv < size) {
		err_msg = "cannot send the whole message";
		goto error;
	}

	rv = 0;

clean:
	ai_destroy(ai);
	ai = NULL;
	return rv;

error:
	if (err_msg != NULL)
		fprintf(stderr, "socket_send: %s\n", err_msg);
	else
		perror("socket_send");
	goto clean;
}

struct addrinfo *ai_create(const char *address, const char *port)
{
	char *err_msg = NULL;
	int rv = -1;
	struct addrinfo hints;
	struct addrinfo *res = NULL;

	if (address == NULL) {
		err_msg = "address is NULL";
		goto error;
	}
	if (port == NULL) {
		err_msg = "port is NULL";
		goto error;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	rv = getaddrinfo(address, port, &hints, &res);
	if (rv != 0)
		goto error;

	return res;

error:
	if (err_msg != NULL)
		fprintf(stderr, "ai_create: %s\n", err_msg);
	else
		perror("ai_create");
	ai_destroy(res);
	return NULL;
}

void ai_destroy(struct addrinfo *ai)
{
	if (ai != NULL) {
		freeaddrinfo(ai);
		ai = NULL;
	}
}