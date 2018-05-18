/**
  * @file loader.h
  * @description A definition of the structures and functions to store and
  *              represent the different elements of an process.
  */

#ifndef _LOADER_H
#define _LOADER_H

/** The process NEW state */
#define NEW 0
/** The process READY state */
#define READY 1
/** The process RUNNNING state */
#define RUNNING 2
/** The process WAITING state */
#define WAITING 3
/** The process TERMINATED state */
#define TERMINATED 4

#define REQ_V 0
#define REL_V 1
#define SEND_V 2
#define RECV_V 3

/**
 * Each process has a list of instructions to execute, with a pointer to the
 * next instruction to execute. An instruction stores the type, participating
 * resource and if it is a send and receive instruction the message. A pointer
 * to the next instruction is also included.
 */
struct instruction {
  /** The type of instruction */
  int type;
  /** The resource or mailbox name used in the instruction */
  char *resource; /* any resource, including a mailbox name */
  /** The message of a send and receive instruction */
  char *msg;
  /** A pointer to the next instruction */
  struct instruction *next;
};

/**
 * A process page stores the name and number of the process.
 */
struct page {
  /** Stores the number of the process. Used to index in the queues */
  int number;
  /** The name of the process */
  char *name;
  /** A Linked list of the process's instructions */
  struct instruction *firstInstruction;
};

/**
 * Represents the mailbox resource. A message sent between two processes are
 * stored and retrieve from the mailbox struct.
 */
struct mailbox {
  /** The name of the mailbox. Used to find the correct mailbox for sending and
   * receiving */
  char *name;
  /** The variable is used to store the sent message for retrieval */
  char *msg;
  /** A pointer to the next mailbox in the system */
  struct mailbox *next;
};

/**
 * Each process has a priority and reference to the cpu scheduling queues.
 */
struct cpuSchedule {
  /** The priority of the process */
  int processPriority;
  /** The readyQueue, where a bit is set if the process is ready */
  struct queue* readyQueue;
  /** The waitingQueue, where a bit is set if the process is waiting for
   * a resource */
  struct queue *waitingQueue;
  /** The terminatedQueue, where a bit is set if the process has finished
   * executing all the instructions and terminated */
  struct queue *terminatedQueue;
};

/**
 * Used to store the available resources in the system as well as the acquired
 * resources for each process.
 */
struct resourceList {
  /** The name of the resource */
  char *name;
  /** The status of the result, either available or occupied */
  int available;
  /** The next resource in the list */
  struct resourceList *next;
};

/**
  * The process control block (PCB). The PCB should point to the
  * page-directory that should point to the pages in memory where
  * a process is stored. In this project each process will be
  * stored in a data structure, called a page; therefore  the PCB
  * can simply point to the page.
  */
struct processControlBlock {
  /** The process page, which stores the number and name of the process */
  struct page *pagePtr;
  /** The current state of the process */
  int processState;
  /** A pointer to the next instruction to be executed */
  struct instruction *nextInstruction;
  /** Pointer to the process priority and scheduling queues */
  struct cpuSchedule *cpuSchedulePtr;
  /** The resources which the current process occupies */
  struct resourceList *resourceListPtr;
  /** Pointer to the next process control block in memory */
  struct processControlBlock *next;
};

/*
 * Creates the process control block and adds
 * it to a linked list of of process control blocks
 */
void load_process ( char* process_name );
/*
 * Loads and stores the instruction of the process
 */
void load_process_instruction ( char* process_name, char* instruction,
    char* resource_name, char *msg );
/*
 * Loads the mailbox and those things associated with
 * it
 */
void load_mailbox ( char* mailboxName );
/*
 * Loads the available system resources for the processes
 */
void load_resource ( char* resource_name );

/*
 * Returns a pointer to the first pcb in the list of loaded processes.
 */
struct processControlBlock* get_loaded_processes();

/*
 * Return the list of available resources
 */
struct resourceList* get_available_resources();

/*
 * Returns a list of the available mailbox resources.
 */
struct mailbox* get_mailboxes();

/*
 * Frees all the processes after termination.
 */
void dealloc_processes();

/*
 *  Frees the instruction i 
 */
void dealloc_instruction(struct instruction *i);

#endif
