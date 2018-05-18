#include "loader.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Enqueues an item to the end of a queue
 * @param q   The queue on which the item is enqueue to
 * @param pcb The process control block to add to the queue.
 */
void enqueue(struct queue *q, struct processControlBlock *pcb) {
  ++q->n;

  if (q->head == NULL) {
    q->head = malloc(sizeof(struct queueItem));
    q->head->next = NULL;
    q->head->item = pcb;
    q->tail = q->head;

#ifdef DEGUB
    print_queue(q);
#endif

    return;
  }

  struct queueItem *newTail = malloc(sizeof(struct queueItem));

  newTail->next = NULL;
  newTail->item = pcb;

  q->tail->next = newTail;
  q->tail = newTail;

#ifdef DEGUB
  print_queue(q);
#endif

  return;
}

/**
 * @brief Dequeues the head of a queue
 * @param  q The queue on which the item is dequeued from
 * @return  returns the pointer to the dequeued item
 */
struct queueItem *dequeue(struct queue *q) {
  if (q->head == NULL) {
    return NULL;
  }

  struct queueItem *head = q->head;

  q->head = q->head->next;
  --q->n;

#ifdef DEBUG
  print_queue(q);
#endif

  return head;
}

/**
 * @brief Prints the contents of a queue
 * @param q The queue to print
 */
void print_queue(struct queue *q) {
  if (q->head == NULL) {
    printf("NULL\n");
    return;
  }
  struct queueItem *h = q->head;

  while (h != NULL) {
    printf("%s -> ", h->item->pagePtr->name);
    h = h->next;
  }

  printf("NULL\n");
}
