/*
 * client.c
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
	printf("Client starting. \n");

	// Create a socket
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);

	// Create address we want to connect to
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

	// Connect to the socket
	printf("Attempting to connect. \n");

	if (connect(sockfd, &addr, sizeof(addr)) == 0) {
		printf("Connected! \n");

		char *buf = malloc(100 + 1);
		int bufsize = 100;
		size_t input_size = 0;

		while (1) {
			printf("Please enter a string to send to the server below: \n");
			input_size = getline(&buf, &bufsize, stdin);
			printf("You wrote: %s \n", buf);
			write(sockfd, buf, (input_size + 1));
		}

	} else {
		printf("Failed");
	}

	close(sockfd);
	printf("Client shutting down. \n");
	return 0;
}
