/**
 * @file manager.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "manager.h"
#include "queue.h"

#define QUANTUM 1
#define TRUE 1
#define FALSE 0

void process_release(struct processControlBlock *current,
                     struct instruction *instruct,
                     struct resourceList *resource);
void process_request(struct processControlBlock *current,
                     struct instruction *instruct,
                     struct resourceList *resource);
void process_send_message(struct processControlBlock *pcb,
                          struct instruction *instruct, struct mailbox *mail);
void process_receive_message(struct processControlBlock *pcb,
                             struct instruction *instruct,
                             struct mailbox *mail);
int acquire_resource(char *resourceName, struct resourceList *resource,
                     struct processControlBlock *p);
int release_resource(char *resourceName, struct resourceList *resource,
                     struct processControlBlock *p);
void add_resource_to_process(struct processControlBlock *current,
                             struct resourceList *resource);
void release_resource_from_process(struct processControlBlock *current,
                                   struct resourceList *resource);
void process_to_readyq(struct cpuSchedule *schedule,
                       struct processControlBlock *proc);
void process_to_waitingq(struct cpuSchedule *schedule,
                         struct processControlBlock *proc);
void process_to_terminateq(struct cpuSchedule *schedule,
                           struct processControlBlock *proc);
int processes_finished(struct processControlBlock *firstPCB);
int processes_deadlocked(struct processControlBlock *firstPCB);
int is_resource_available(char *resourceName, struct resourceList *resource);
void send_processes_to_readyq(struct queue *waitingQueue,
                              struct resourceList *resource);
void release_all_resources_from_process(struct processControlBlock *pcb);
void print_available_resources(struct resourceList *resource);

/**
 * @brief Schedules processes by either robin-round fashion or first come
 * first serve depending on the alogrithm selected through the variable
 * schedule_alg
 *
 * @param pcb The process control block which contains the current process as
 * well as a pointer to the next process control block.
 * @param resource The list of resources available to the system.
 * @param mail The list of mailboxes available to the system.
 * @param schedule_alg Bit indicating which scheduling alogrithm to use
 * @param quantum Number of instructions a process should run before it is
 * preemptied
 */

void schedule_processes(struct processControlBlock *pcb,
                        struct resourceList *resource, struct mailbox *mail,
                        int schedule_alg, int quantum) {

  struct processControlBlock *firstPCB = pcb;
  int num = pcb->cpuSchedulePtr->readyQueue->tail->item->pagePtr->number;

  if (schedule_alg == 0) {
    schedule_processes_fcfs(pcb, resource, mail);
  } else if (schedule_alg == 1) {
    schedule_processes_rr(pcb, firstPCB, resource, mail, quantum, num, 0);
  }
}

/**
 * @brief Schedules each instruction of each process in a round-robin fashion.
 * The number of instruction to execute for each process is governed by the
 * QUANTUM variable.
 *
 * @param pcb The process control block which contains the current process as
 * well as a pointer to the next process control block
 * @param resource The list of resources available to the system
 * @param mail     The list of mailboxes available to the system
 * @param quantum  Number of instructions the process should be allowed to
 * run before it is preemptied
 * @param tally    The number of times the current process has been running
 */

void schedule_processes_rr(struct processControlBlock *pcb,
                           struct processControlBlock *firstPCB,
                           struct resourceList *resource, struct mailbox *mail,
                           int quantum, int num, int tally) {

  quantum = quantum == 0 ? QUANTUM : quantum;

  while (pcb->cpuSchedulePtr->readyQueue->head != NULL) {
    struct processControlBlock *p =
        dequeue(pcb->cpuSchedulePtr->readyQueue)->item;

    tally = 0;
    while (tally < quantum) {
      if (p->nextInstruction == NULL) {
        break;
      }

      switch (p->nextInstruction->type) {
      case REQ_V:
        process_request(p, p->nextInstruction, resource);
        ++tally;
        break;
      case REL_V:
        process_release(p, p->nextInstruction, resource);
        ++tally;
        break;
      case SEND_V:
        process_send_message(p, p->nextInstruction, mail);
        ++tally;
        break;
      case RECV_V:
        process_receive_message(p, p->nextInstruction, mail);
        ++tally;
        break;
      default:
        break;
      }
    }

    send_processes_to_readyq(pcb->cpuSchedulePtr->waitingQueue, resource);

    if (p->processState == RUNNING && p->nextInstruction != NULL) {
      process_to_readyq(p->cpuSchedulePtr, p);
    }

    if (processes_deadlocked(firstPCB)) {
#ifdef DEBUG
      printf("DEADLOCKED\n");
#endif

      struct processControlBlock *firstPCB_copy = firstPCB;
      while (processes_deadlocked(firstPCB)) {
#ifdef DEBUG
        printf("RECOVERING FROM DEADLOCK\n");
#endif
        release_all_resources_from_process(firstPCB_copy);
        process_to_terminateq(firstPCB_copy->cpuSchedulePtr, firstPCB_copy);
        send_processes_to_readyq(firstPCB->cpuSchedulePtr->waitingQueue,
                                 resource);
        firstPCB_copy = firstPCB_copy->next;
      }
    }
  }
}

void schedule_processes_fcfs(struct processControlBlock *pcb,
                             struct resourceList *resource,
                             struct mailbox *mail) {

  while (pcb->nextInstruction != NULL) {
    switch (pcb->nextInstruction->type) {
	  case REQ_V:
		process_request(pcb, pcb->nextInstruction, resource);
		break;
	  case REL_V:
		process_release(pcb, pcb->nextInstruction, resource);
		break;
	  case SEND_V:
		process_send_message(pcb, pcb->nextInstruction, mail);
		break;
	  case RECV_V:
		process_receive_message(pcb, pcb->nextInstruction, mail);
		break;
	  default:
		break;
	}
  }

  if (pcb->cpuSchedulePtr->readyQueue->head != NULL) {
    struct processControlBlock *h =
        dequeue(pcb->cpuSchedulePtr->readyQueue)->item;
    schedule_processes_fcfs(h, resource, mail);
  }
  return;
}

/**
 * @brief Handles the request resource instruction.
 *
 * Executes the request instruction for the process. The function loops
 * through the list of resources and acquires the resource if it is available.
 * If the resource is not available the process sits in the waiting queue and
 * tries to acquire the resource on the next cycle.
 *
 * @param current The current process for which the resource must be acquired.
 * @param instruct The instruction which requests the resource.
 * @param resource The list of resources.
 */

void process_request(struct processControlBlock *current,
                     struct instruction *instruct,
                     struct resourceList *resource) {
  int acquired;

  current->processState = RUNNING;

  acquired = acquire_resource(instruct->resource, resource, current);

  if (!acquired) {
    printf("%s req %s: waiting;\n", current->pagePtr->name, instruct->resource);
    process_to_waitingq(current->cpuSchedulePtr, current);
    return;
  }

  printf("%s req %s: acquired; ", current->pagePtr->name, instruct->resource);
  print_available_resources(resource);

  current->nextInstruction = current->nextInstruction->next;
  dealloc_instruction(instruct);
}

/**
 * @brief Handles the release resource instruction.
 *
 * Executes the release instruction for the process. If the resource can be
 * released the process is ready for next execution. If the resource can not
 * be released the process sits in the waiting queue.
 *
 * @param current The process which releases the resource.
 * @param instruct The instruction to release the resource.
 * @param resource The list of resources.
 */

void process_release(struct processControlBlock *current,
                     struct instruction *instruct,
                     struct resourceList *resource) {

  current->processState = RUNNING;

  if (release_resource(instruct->resource, resource, current)) {
    printf("%s rel %s: released; ", current->pagePtr->name, instruct->resource);
    print_available_resources(resource);

    if (current->nextInstruction->next == NULL) {
      process_to_terminateq(current->cpuSchedulePtr, current);
    }
  } else {
    process_to_waitingq(current->cpuSchedulePtr, current);
  }

  current->nextInstruction = current->nextInstruction->next;
  dealloc_instruction(instruct);
}

/**
 * @brief Sends the message the prescribed mailbox.
 *
 * Sends the message specified in the instruction of the current process, to
 * the mailbox specified in the instruction. The function gets the pointer to
 * the first mailbox and loops through all the mailboxes to find the one to
 * which the message must be sent.
 *
 * @param pcb The current process which instruct us to send a message.
 * @param instruct The current send instruction which contains the message.
 * @param mail The list of available mailboxes.
 */
void process_send_message(struct processControlBlock *pcb,
                          struct instruction *instruct, struct mailbox *mail) {

  struct mailbox *currentMbox;

  pcb->processState = RUNNING;

  currentMbox = mail;
  do {
    if (strcmp(currentMbox->name, instruct->resource) == 0) {
      /* We found the mailbox in which a message should be left */
      break;
    }
    currentMbox = currentMbox->next;
  } while (currentMbox != NULL);

  printf("%s send: Message \033[22;31m %s \033[0m addede to %s\n",
         pcb->pagePtr->name, instruct->msg, currentMbox->name);

  currentMbox->msg = instruct->msg;
  pcb->nextInstruction = pcb->nextInstruction->next;
  dealloc_instruction(instruct);
}

/**
 * @brief Retrieves the message from the mailbox specified in the instruction
 * and stores it in the instruction message field.
 *
 * The function loops through the available mailboxes and finds the mailbox
 * from which the message must be retrieved. The retrieved message is stored
 * in the message field of the instruction of the process.
 *
 * @param pcb The current process which requests a message retrieval.
 * @param instruct The instruction to retrieve a message from a specific
 * mailbox.
 * @param mail The list of mailboxes.
 */
void process_receive_message(struct processControlBlock *pcb,
                             struct instruction *instruct,
                             struct mailbox *mail) {

  struct mailbox *currentMbox;

  pcb->processState = RUNNING;

  currentMbox = mail;
  do {
    if (strcmp(currentMbox->name, instruct->resource) == 0) {
      /* We found the mailbox from which a message must be read. */
      break;
    }
    currentMbox = currentMbox->next;
  } while (currentMbox != NULL);

  printf("%s recv: Message \033[22;32m %s "
         "\033[0m removed from %s\n",
         pcb->pagePtr->name, currentMbox->msg, currentMbox->name);

  instruct->msg = currentMbox->msg;
  currentMbox->msg = NULL;
  pcb->nextInstruction = pcb->nextInstruction->next;
  dealloc_instruction(instruct);
}

/**
 * @brief Acquires the resource specified by resourceName.
 *
 * The function iterates over the list of resources trying to acquire the
 * resource specified by resourceName. If the resources is available, the
 * process acquires the resource. The resource is indicated as not available
 * in the resourceList and 1 is returned indicating that the resource has been
 * acquired successfully.
 *
 * @param resourceName The name of the resource to acquire.
 * @param resources The list of resources.
 * @param p The process which acquires the resource.
 *
 * @return 1 for TRUE if the resource is available. 0 for FALSE if the
 * resource is not available.
 */

int acquire_resource(char *resourceName, struct resourceList *resource,
                     struct processControlBlock *p) {

  if (resource == NULL) {
    return FALSE;
  }

  if (strcmp(resource->name, resourceName) == 0) {
    if (resource->available == 1) {
#ifdef DEBUG
      printf("%s acquiring resource %s\n", p->pagePtr->name, resource->name);
#endif
      add_resource_to_process(p, resource);

      return TRUE;
    } else {
      // search for another instance of this resource
      return acquire_resource(resourceName, resource->next, p);
    }
  } else {
    return acquire_resource(resourceName, resource->next, p);
  }
}

/**
 * @brief Releases the resource specified by resourceName
 *
 * Iterates over the resourceList finding the resource which must be set to
 * available again. The resource is then released from the process.
 *
 * @param resourceName The name of the resource to release.
 * @param resources The list of available resources.
 * @param p The current process.
 *
 * @return 1 (TRUE) if the resource was released succesfully else 1 (FALSE).
 */

int release_resource(char *resourceName, struct resourceList *resource,
                     struct processControlBlock *p) {
  if (resource == NULL) {
    printf("%s rel %s: ERROR: Nothing to release\n", p->pagePtr->name,
           resourceName);
    return FALSE;
  }

  if (strcmp(resource->name, resourceName) == 0) {
    resource->available = 1;
    release_resource_from_process(p, resource);
    return TRUE;
  } else {
    return release_resource(resourceName, resource->next, p);
  }
}

/**
 * @brief Adds the specified resource to the process acquired resource list.
 *
 * After the resource has succesfully been required by the process. This
 * function is called and adds the resource to the list of resources currently
 * held by this process.
 *
 * @param current The process to which the resource must be added.
 * @param resource The resource to add to the process.
 */

void add_resource_to_process(struct processControlBlock *current,
                             struct resourceList *resource) {
  struct resourceList *proc_rsrc = current->resourceListPtr;
  struct resourceList *r = malloc(sizeof(struct resourceList));

  if (current->resourceListPtr == NULL) {
    r->available = resource->available = 0;
    r->name = resource->name;
    r->next = resource->next;
    current->resourceListPtr = r;
    return;
  }

  while (proc_rsrc != NULL) {
    proc_rsrc = proc_rsrc->next;
  }

  resource->available = r->available = 0;
  r->name = resource->name;
  r->next = resource->next;
  proc_rsrc = r;
}

/**
 * @brief Release the specified resource from the process acquired list.
 *
 * The function releases the specified resource from the current process
 * acquired list.
 *
 * @param current The current process from which the resource must be
 * released.
 * @param resource The resource to release.
 */

void release_resource_from_process(struct processControlBlock *current,
                                   struct resourceList *resource) {
  struct resourceList *cur = current->resourceListPtr;
  struct resourceList *prev = NULL;

  while (cur != NULL) {
    if (strcmp(cur->name, resource->name) == 0) {
      if (prev == NULL) {
        cur = cur->next;
        return;
      }
      prev->next = cur->next;
      return;
    }
    prev = cur;
    cur = cur->next;
  }

  return;
}

/**
 * @brief Add process (with id proc) to readyQueue
 *
 * If readyQueue is a bitvector then set the bit in the readyQueue for the
 * process with id proc.
 *
 * @param schedule The struct which stores the queues.
 * @param proc The process which must be set to ready.
 */

void process_to_readyq(struct cpuSchedule *schedule,
                       struct processControlBlock *proc) {
  struct queue *readyq;

  proc->processState = READY;

  readyq = schedule->readyQueue;

#ifdef DEGUB
  printf("Added Process %s to the readyQueue\n", proc->pagePtr->name);
#endif

  enqueue(readyq, proc);

  return;
}

/**
 * @brief Add process (with id proc) to the waitingQueue
 *
 * If waitingQueue is a bitvector then set the bit in the waitingQueue for the
 * process with id proc.
 *
 * @param schedule The struct which stores the queues.
 * @param proc The process which must be set to waiting.
 */

void process_to_waitingq(struct cpuSchedule *schedule,
                         struct processControlBlock *proc) {
  struct queue *waitingQueue;

  proc->processState = WAITING;

  waitingQueue = schedule->waitingQueue;

#ifdef DEGUB
  printf("Added Process %s to the waitingQueue\n", proc->pagePtr->name);
#endif

  enqueue(waitingQueue, proc);

  return;
}

/**
 * @brief Add process (with id proc) to the terminatedQueue
 *
 * If terminateQueue is a bitvector then set the bit in the terminatedQueue
 * for the process with id proc.
 *
 * @param schedule The struct which stores the queues.
 * @param proc The process which must be set to waiting.
 */

void process_to_terminateq(struct cpuSchedule *schedule,
                           struct processControlBlock *proc) {
  struct queue *terminatedQueue;

  proc->processState = TERMINATED;

  terminatedQueue = schedule->terminatedQueue;

#ifdef DEBUG
  printf("Added Process %s to the terminatedQueue\n", proc->pagePtr->name);
#endif

  enqueue(terminatedQueue, proc);

  return;
}

/**
 * @brief Prints all available resources in the resource list
 * @param resource  resource list containing all the resources
 */

void print_available_resources(struct resourceList *resource) {
  struct resourceList *cur = resource;

  printf("Available : ");
  while (cur != NULL) {
    if (cur->available == 1) {
      printf("%s ", cur->name);
    }
    cur = cur->next;
  }
  printf("\n");
}

/**
 * @brief Iterates over each of the loaded processes and checks if it has been
 * terminated.
 *
 * Iterates over the processes to determine if they have terminated.
 *
 * @param firstPCB A pointer to start of all the processes.
 *
 * @return 1 (TRUE) if all the processes are terminated else 0 (FALSE).
 */

int processes_finished(struct processControlBlock *firstPCB) {
  if (firstPCB == NULL) {
    return 1;
  }
  if (firstPCB->processState != TERMINATED) {
    return 0;
  }
  return processes_finished(firstPCB->next);
}

/**
 * @brief Detects deadlock.
 *
 * This function implements a deadlock detection algorithm.
 *
 * @param firstPCB A pointer to the start of all the processes.
 *
 * @return 1 (TRUE) if all the processes are in the waiting state else
 * 0 (FALSE).
 */

int processes_deadlocked(struct processControlBlock *firstPCB) {
  if (firstPCB == NULL) {
    return 1;
  }
  if (firstPCB->processState == WAITING) {
    return processes_deadlocked(firstPCB->next);
  }

  return 0;
}

/**
 * @brief Releases all resources currently acquired by the process p
 * @param p Process to release resources from
 */
void release_all_resources_from_process(struct processControlBlock *p) {
  struct resourceList *r = p->resourceListPtr;
  while (r != NULL) {
    release_resource(r->name, r, p);
    r = r->next;
  }
  return;
}

/**
 * @brief Checks if a certain resource is available
 * @param  resourceName The name of the process to check
 * @param  resource     List of all available resources to the system
 * @return              1 (TRUE) if the resource is available,
 *                      returns 0 (FALSE) otherwise.
 */
int is_resource_available(char *resourceName, struct resourceList *resource) {
  if (resource == NULL) {
    return FALSE;
  }

  if (strcmp(resource->name, resourceName) == 0) {
    if (resource->available == 1) {
      return TRUE;
    } else {
      return is_resource_available(resourceName, resource->next);
    }
  } else {
    return is_resource_available(resourceName, resource->next);
  }
}

/**
 * @brief Checks for processes in the waiting queue ready to run and puts them
 *        in a ready queue
 * @param waitingQueue The waiting queue to check in
 * @param resource     List of resources svailable to the system
 */

void send_processes_to_readyq(struct queue *waitingQueue,
                              struct resourceList *resource) {
  if (waitingQueue->head != NULL) {
    int n = waitingQueue->n;
    while (n > 0) {

      struct queueItem *q = dequeue(waitingQueue);
      if (is_resource_available(q->item->nextInstruction->resource, resource)) {
        process_to_readyq(q->item->cpuSchedulePtr, q->item);

      } else {
        enqueue(waitingQueue, q->item);
      }
      --n;
    }
  }
}
