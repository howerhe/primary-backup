#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/message.h"
#include "../src/socket.h"

#define ROLE_SERVER 0
#define ROLE_CLIENT 1

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: ./socket_test role\n");
		exit(EXIT_FAILURE);
	}
	int role = strtol(argv[1], NULL, 10) == ROLE_SERVER ? ROLE_SERVER :
								    ROLE_CLIENT;

	char *server_addr = "127.0.0.1";
	char *server_port = "9999";
	struct message *server_message = malloc(sizeof(struct message));
	assert(server_message);
	server_message->type = MESSAGE_SERVER_RESPONSE;
	server_message->index = 1;
	server_message->value = 2;
	server_message->version = 3;
	strncpy(server_message->addr, server_addr, MESSAGE_ADDR_SIZE);
	strncpy(server_message->port, server_port, MESSAGE_PORT_SIZE);

	char *client_addr = "127.0.0.1";
	char *client_port = "9998";
	struct message *client_message = malloc(sizeof(struct message));
	assert(client_message);
	client_message->type = MESSAGE_CLIENT_READ;
	client_message->index = 4;
	client_message->value = 5;
	client_message->version = 6;
	strncpy(client_message->addr, client_addr, MESSAGE_ADDR_SIZE);
	strncpy(client_message->port, client_port, MESSAGE_PORT_SIZE);

	if (role == ROLE_SERVER) {
		int fd = -1;
		int rv = -1;
		struct message *message = malloc(sizeof(struct message));
		assert(message);

		// socket_init().
		fd = socket_init(NULL, server_port);
		assert(fd == -1);
		fd = socket_init(server_addr, NULL);
		assert(fd == -1);
		fd = socket_init(server_addr, server_port);
		assert(fd != -1);

		// socket_receive().
		rv = socket_receive(-1, message);
		assert(rv != 0);
		rv = socket_receive(fd, NULL);
		assert(rv != 0);

		rv = socket_receive(fd, message);
		assert(rv == 0);
		assert(message->type == client_message->type);
		assert(message->index == client_message->index);
		assert(message->value == client_message->value);
		assert(message->version == client_message->version);
		assert(strcmp(message->addr, client_addr) == 0);
		assert(strcmp(message->port, client_port) == 0);

		// socket_send().
		rv = socket_send(-1, server_message, message->addr,
				 message->port);
		assert(rv != 0);
		rv = socket_send(fd, NULL, message->addr, message->port);
		assert(rv != 0);
		rv = socket_send(fd, server_message, NULL, message->port);
		assert(rv != 0);
		rv = socket_send(fd, server_message, message->addr, NULL);
		assert(rv != 0);
		rv = socket_send(fd, server_message, message->addr,
				 message->port);
		assert(rv == 0);

		// socket_free().
		socket_free(fd);
		fd = -1;
	} else {
		int fd = -1;
		int rv = -1;
		struct message *message = malloc(sizeof(struct message));
		assert(message);

		// socket_init().
		fd = socket_init(NULL, client_port);
		assert(fd == -1);
		fd = socket_init(client_addr, NULL);
		assert(fd == -1);
		fd = socket_init(client_addr, client_port);
		assert(fd != -1);

		// socket_send().
		rv = socket_send(-1, client_message, server_addr, server_port);
		assert(rv != 0);
		rv = socket_send(fd, NULL, server_addr, server_port);
		assert(rv != 0);
		rv = socket_send(fd, client_message, NULL, server_port);
		assert(rv != 0);
		rv = socket_send(fd, client_message, server_addr, NULL);
		assert(rv != 0);
		rv = socket_send(fd, client_message, server_addr, server_port);
		assert(rv == 0);

		// socket_receive().
		rv = socket_receive(-1, message);
		assert(rv != 0);
		rv = socket_receive(fd, NULL);
		assert(rv != 0);

		rv = socket_receive(fd, message);
		assert(rv == 0);
		assert(message->type == server_message->type);
		assert(message->index == server_message->index);
		assert(message->value == server_message->value);
		assert(message->version == server_message->version);
		assert(strcmp(message->addr, server_addr) == 0);
		assert(strcmp(message->port, server_port) == 0);

		// socket_free().
		socket_free(fd);
		fd = -1;
	}

	free(client_message);
	client_message = NULL;
	free(server_message);
	server_message = NULL;

	printf("All tests passed.\n");

	return 0;
}