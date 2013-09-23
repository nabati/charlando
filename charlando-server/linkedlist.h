/*
 * linkedlist.h
 *
 *  Created on: 23 sep 2013
 *      Author: poyan
 */

#ifndef LINKEDLIST_H_
#define LINKEDLIST_H_

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Node-container
typedef struct {
	struct _node *next;
	char *data;
	size_t length;
} Node;

typedef struct {
	Node *head; // Points to the head
	Node *tail; // Points to the tail
	Node *cur; // Points to the current element
} LinkedList;

LinkedList *ll_init() {
	LinkedList *ll = malloc(sizeof(LinkedList));
	ll->head = NULL;
	ll->tail = NULL;
	ll->cur = NULL;
	return ll;
}

/**
 * Gets the next Node if exists, otherwise, returns null
 * @param linked_list
 * @return
 */
int ll_next(LinkedList *ll) {
	if (ll->cur == NULL) {
		ll->cur = ll->head;
	} else {
		ll->cur = ll->cur->next;
	}
	return ll->cur;
}

void ll_reset(LinkedList *ll) {
	ll->cur = NULL;
}

void ll_add(LinkedList *ll, Node *newNode) {
	if (ll->head == NULL) {
		ll->head = newNode;
		ll->tail = newNode;
	} else {
		ll->tail->next = newNode;
		ll->tail = newNode;
		return;
	}
}

// Initializes a new node
Node *node_init(char *data, size_t length) {
	Node *newNode = malloc(sizeof(Node));
	newNode->data = data;
	newNode->length = length;
	newNode->next = NULL;
	return newNode;
}

#endif /* LINKEDLIST_H_ */
