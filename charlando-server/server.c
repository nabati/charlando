/*
 * server.c
 *
 *  Created on: 13 sep 2013
 *      Author: poyan
 */

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>
#include <wait.h>
#include "common.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>

/**
 * Waits for terminated children
 * @param status signal code
 */
void sigchld_handler(int status) {
	while (waitpid(-1, NULL, WNOHANG) != 0) {
		if (errno == ECHILD) {
			break;
		}
	}
}

void sigpipe_handler(int status) {
	printf("Sigpipe.");
}

/**
 * Registers a handler for the message queue referenced by mqd_ptr
 * If NULL is passed, tries using the last used one.
 * @param mqd_ptr
 */
void register_mq_notify(mqd_t *mqd_ptr) {
	static *mqd_static;
	if (mqd_ptr != NULL)
		mqd_static = mqd_ptr;

	if (mqd_static != NULL) {
		struct sigevent sev;
		sev.sigev_notify = SIGEV_SIGNAL;
		sev.sigev_signo = SIGUSR1;
		mq_notify(*mqd_static, &sev);
	}
}

void sigusr1_handler(int status) {
	// Re-register handler
	register_mq_notify(NULL);
	// Print message
	mq_re
	printf()
}

/**
 * Reverses a null-terminated string
 * @param string null-terminated string
 * @param length
 */
void strrev(char *string, size_t length) {
	assert(*(string + length - 1) == '\0');
	size_t content = length - 1;
	char *reversed = malloc(content);

	int i;
	for (i = 0; i < content; i++) {
		*(reversed + i) = *(string + content - i - 1);
	}
	// How to secure? Check if if \0 present?
	memcpy(string, reversed, content);
}

int main(int argc, char **argv) {
	printf("Server starting. \n");

	// Create a socket
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
	signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE on writing to dying client

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
	// Bind the socket
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))
			== 0) {
		printf("Main>Successfully bound address. \n");

		if (listen(sockfd, 10) == 0) {
			printf("Main>Listening for connections.\n");
			printf("Main>Accepting connections! Will block. \n");
			while (1) {

				int bufsize = 100;
				char *buf = malloc(bufsize + 1);
				char chr;
				accepted_socket = accept(sockfd, NULL, 0);

				printf("Main>Accepted socket, forking \n");

				pid_t child_pid = fork();
				switch (child_pid) {
				case 0: {
					// Child process
					pid_t process_pid = getpid();
					while (1) {
						// Open queue to parent
						mqd_t outgoing = mq_open("/charlando_main_server_input",
						O_CREAT | O_WRONLY);

						// Read if possible
						int i;
						int bytes;
						for (i = 0;
								(bytes = recv(accepted_socket, &chr, 1, 0)) > 0;
								i++) {
							buf[i] = chr;
							if (chr == '\0')
								break;
						}

						// Check if it above read failed with error
						if (bytes > 0) {
							// TODO: Send to parent via message queue
//							printf("Client[%d]>%s\n", process_pid, buf);
							mq_send(outgoing, buf, strlen(buf) + 1, 0);

							strrev(buf, strlen(buf) + 1);
							printf("Server[%d]>%s\n", process_pid, buf);
							if (write(accepted_socket, buf, strlen(buf) + 1)
									== -1) {
								break;
							}
						} else {
							break;
						}

					}
					free(buf);
					printf("Server[%d]>Client disconnected, exiting.\n",
							process_pid);
					exit(EXIT_SUCCESS);
					break;
				}
				default: {
					// Parent process
					// Register handler for SIGCHLD to wait for terminated children
					struct sigaction sa;
					memset(&sa, 0, sizeof sa);
					sa.sa_handler = &sigchld_handler;
					sa.sa_flags = SA_RESTART;
					sigaction(SIGCHLD, &sa, NULL);

					// Open message queue for incoming messages
					mqd_t incoming = mq_open("/charlando_main_server_input",
					O_CREAT | O_RDONLY);

					// Register handler for SIGUSR1 to grab new messages
					struct sigaction sau;
					memset(&sa, 0, sizeof sau);
					sau.sa_handler = &sigusr1_handler;
					sau.sa_flags = SA_RESTART;
					sigaction(SIGUSR1, &sau, NULL);

					register_mq_notify(incoming);

					printf("Main>Child created Server[%d]", child_pid);
					break;
				}
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

