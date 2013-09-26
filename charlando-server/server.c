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
#include "linkedlist.h"

#define PARENT_MESSAGE_QUEUE "/charlando_server_parent"

// Create a linked-list to hold the message queue descriptors of all children
LinkedList *mq_ll;

// Hold incoming message queue descriptor for parent
mqd_t mqd_parent_incoming;
mqd_t incoming;

int accepted_socket;

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
	fprintf(stderr, "Sigpipe.\n");
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

/**
 * Processes all messages on the parents message queue
 */
void process_mq_parent() {
	// Register SIGUSR1 to be signaled when new message has arrived
	struct sigevent sev;
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGUSR1;
	mq_notify(mqd_parent_incoming, &sev);

	mqd_t *mqd_child;
	Node *node = NULL;

	// Allocate message buffer
	char *msg = malloc(8192);
	memset(msg, 0, 8192);

	// Fetch messages until queue is empty
	while (mq_receive(mqd_parent_incoming, msg, 8192, 0) >= 0) {
		fprintf(stderr, "%s\n", msg);
		// Send received message to each child
		while ((node = ll_next(mq_ll))) {
			mqd_child = node->data;
			mq_send(*mqd_child, msg, 8192, 0);
		}
	}
}

void process_mq_child() {
	// Register SIGUSR1 to be signaled when new message has arrived
	struct sigevent sev;
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGUSR1;
	mq_notify(incoming, &sev);

//	 Allocate message buffer
	char *msg = malloc(8192);
	memset(msg, 0, 8192);

	// Read message from buffer
	while (mq_receive(incoming, msg, 8192, 0) >= 0) {
		fprintf(stderr, "R:%s\n", msg);
		// Write to socket
		if (write(accepted_socket, msg, 8192) == -1) {
			fprintf(stderr, "No write%s\n", msg);
		}
	}
}

int main(int argc, char **argv) {
	// Initialize mq_ll
	mq_ll = ll_init();

	// Open incoming mqd for parent
	mqd_parent_incoming = mq_open(PARENT_MESSAGE_QUEUE,
	O_CREAT | O_RDONLY | O_NONBLOCK,
	S_IRWXG | S_IRWXU | S_IRWXO, NULL);

	fprintf(stderr, "Server starting. \n");

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

	// Bind the socket
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))
			== 0) {
		fprintf(stderr, "Main>Successfully bound address. \n");

		if (listen(sockfd, 10) == 0) {
			fprintf(stderr, "Main>Listening for connections.\n");
			fprintf(stderr, "Main>Accepting connections! Will block. \n");
			while (1) {

				int bufsize = 8192;
				char *buf = malloc(bufsize + 1);
				char chr;

				accepted_socket = accept(sockfd, NULL, 0);

				fprintf(stderr, "Main>Accepted socket, forking \n");

				pid_t child_pid = fork();

				switch (child_pid) {
				case 0: {
					// Child process
					pid_t process_pid = getpid();

					// Open outgoing queue to parent
					mqd_t outgoing = mq_open(PARENT_MESSAGE_QUEUE,
					O_CREAT | O_WRONLY, S_IRWXG | S_IRWXU | S_IRWXO, NULL);

					// Open incoming queue from parent
					char *queue_name = malloc(100);
					sprintf(queue_name, "/charlando_server_child_%d",
							process_pid);
					// TODO: Clean global variable?
					incoming = mq_open(queue_name,
					O_CREAT | O_RDONLY | O_NONBLOCK,
					S_IRWXG | S_IRWXU | S_IRWXO, NULL);
					free(queue_name);

					// Register handler for SIGUSR1 to receive messages
					struct sigaction sa;
					memset(&sa, 0, sizeof sa);
					sa.sa_handler = &process_mq_child;
					sa.sa_flags = SA_RESTART;
					sigaction(SIGUSR1, &sa, NULL);

					// Fire once to bind
					process_mq_child();

					while (1) {
						// Read if possible from socket
						int i;
						int bytes = 0;
						for (i = 0;
								(bytes += recv(accepted_socket, &chr, 1, 0)) > 0;
								i++) {
							buf[i] = chr;
							if (chr == '\0')
								break;
						}

						// If we read a message
						if (bytes > 0) {
							// Send to parent via message queue
							char *msg = malloc(8192);
							memset(msg, 0, 8192);
							sprintf(msg, "Client[%d]>%s\n", process_pid, buf);
							mq_send(outgoing, msg, strlen(msg), 0);
							free(msg);
						} else {
							break;
						}

					}
					free(buf);
					fprintf(stderr,
							"Server[%d]>Client disconnected, exiting.\n",
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

					// Register handler for SIGUSR1 to grab new messages
					memset(&sa, 0, sizeof sa);
					sa.sa_handler = &process_mq_parent;
					sa.sa_flags = SA_RESTART;
					sigaction(SIGUSR1, &sa, NULL);

					// Open child message queue, and add to linked list
					char *queue_name = malloc(100);
					memset(queue_name, 0, 100);
					sprintf(queue_name, "/charlando_server_child_%d",
							child_pid);
					fprintf(stderr, "%s\n", queue_name);
					mqd_t child_mqd = mq_open(queue_name, O_CREAT | O_WRONLY,
					S_IRWXG | S_IRWXU | S_IRWXO, NULL);
					ll_add(mq_ll, node_init(&child_mqd, sizeof(child_mqd)));
					free(queue_name);

					process_mq_parent();

					fprintf(stderr, "Main>Child created Server[%d]\n",
							child_pid);
					break;
				}
				}

			}

		} else {
			fprintf(stderr, "Couldn't listen. \n");
		}

	} else {
		fprintf(stderr, "Couldn't bind address. \n");
	}

	close(sockfd);

	fprintf(stderr, "Server shutting down. \n");
	return 0;
}
