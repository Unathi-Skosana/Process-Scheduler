/**
  * @file queue.h
  * @description A definition of the structures and functions related to queues.
  */



#ifndef _QUEUE_H
#define _QUEUE_H

#include "loader.h"

struct queueItem {
	struct processControlBlock *item;
	struct queueItem* next;
};

struct queue {
	struct queueItem* head;
	struct queueItem* tail;
	int n;
};


void enqueue(struct queue *q, struct processControlBlock *pcb);
void print_queue(struct queue *q);
struct queueItem* dequeue(struct queue *q);

#endif
