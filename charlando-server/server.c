/*
 * server.c
 *
 *  Created on: 13 sep 2013
 *      Author: poyan
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include "common.h"



int main(int argc, char **argv) {
	printf("Server starting. \n");

	// Create a socket
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);

	// Create address we want to bind to

	struct sockaddr_in *addr = malloc(sizeof(struct sockaddr_in));
	memset(addr, 0, sizeof(struct sockaddr_in));

	// Bind the socket
	if (sockfd, *addr, sizeof(struct sockaddr_in)) {
		printf("Successfully bound address. \n");
	} else {
		printf("Couldn't bind address. \n");
	}

	close(sockfd);

	printf("Server shutting down. \n");
	return 0;
}
