/*
 * server.c
 *
 *  Created on: 13 sep 2013
 *      Author: poyan
 */
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include "common.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv) {
	printf("Server starting. \n");

	// Create a socket
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	int optval = 1;
	setsockopt(sockfd, 0, SO_REUSEADDR, &optval, sizeof optval);

	// Create address we want to bind to

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(struct sockaddr_in));

	// Allocate 32 bits for ip-address
	void *binaryLocalhost = malloc(32);

	// Convert text-notation to binary notation
	inet_pton(AF_INET, "127.0.0.1", binaryLocalhost);
	addr.sin_family = AF_INET;

	// Convert void* to in_addr_t* and dereference
	addr.sin_addr.s_addr = *(in_addr_t *) binaryLocalhost;
	addr.sin_port = htons(PORT_NUM);

	int accepted_socket;
	int bufsize = 100;
	char *buf = malloc(bufsize + 1);
	char chr;
	// Bind the socket
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))
			== 0) {
		printf("Successfully bound address. \n");

		if (listen(sockfd, 10) == 0) {
			printf("Listening for connections.\n");
			printf("Accepting connections! Will block. \n");
			while (1) {
				accepted_socket = accept(sockfd, NULL, 0);
				printf("Accepted socket. \n");

				while (1) {
					int i;
					for (i = 0; recv(accepted_socket, &chr, 1, 0); i++) {
						buf[i]=chr;
						if (chr == '\0')
							break;
					}
					printf("Got a string: %s", buf);
				}

			}

		} else {
			printf("Couldn't listen. \n");
		}

	} else {
		printf("Couldn't bind address. \n");
	}

	close(sockfd);

	printf("Server shutting down. \n");
	return 0;
}
