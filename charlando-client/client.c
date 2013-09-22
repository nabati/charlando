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
#include <assert.h>

void trim_newlines(char *string, size_t length) {
	int i;
	for (i=0; i<length; i++) {
		if (string[i]=='\n')
			string[i]=' ';
	}
}

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

		char chr = NULL;
		char *buf = malloc(100 + 1);
		size_t bufsize = 100;
		size_t input_size = 0;

		while (1) {
			printf("Client>");
			input_size = getline(&buf, &bufsize, stdin);
			trim_newlines(buf, input_size);
			write(sockfd, buf, (input_size + 1));

			int i;
			int result;
			for (i = 0; (result = recv(sockfd, &chr, 1, 0)); i++) {
				buf[i]=chr;
				if (chr == '\0')
					break;
			}
			printf("Server>%s\n", buf);
		}

	} else {
		printf("Failed");
	}

	close(sockfd);
	printf("Client shutting down. \n");
	return 0;
}
