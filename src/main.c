/**
 * @file main.c
 * @author Francois de Villiers (Demi 2011-2012)
 * @description The main function file for the process management application.
 *
 * @mainpage Process Simulation
 *
 * @section intro_sec Introduction
 *
 * The project consists of 3 main files parser, loader and manager. Which
 * respectively handles the loading, parsing and management of the processes.
 *
 * @section make_sec Compile
 *
 * $ make
 *
 * @section run_sec Execute
 *
 * $ ./process-management data/process.list
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "loader.h"
#include "manager.h"
#include "parser.h"
#include "queue.h"

void debug_pcb(struct processControlBlock *pcb);
void debug_mailboxes(struct mailbox *mail);

int main(int argc, char **argv) {
  char *filename;
  int schedule_alg;
  int quantum = 1;
  struct processControlBlock *pcb;
  struct resourceList *resources;
  struct mailbox *mailboxes;

  filename = NULL;

  if (argc < 1) {
    return EXIT_FAILURE;
  }

  filename = argv[1];
  schedule_alg = atoi(argv[2]);

  if (schedule_alg == 1) {
    quantum = atoi(argv[3]);
  }

  parse_process_file(filename);

  pcb = get_loaded_processes();
  resources = get_available_resources();
  mailboxes = get_mailboxes();

#ifdef DEBUG
  debug_pcb(pcb);
#endif

  schedule_processes(pcb, resources, mailboxes, schedule_alg, quantum);
  dealloc_processes();

  return EXIT_SUCCESS;
}

#ifdef DEBUG
void debug_pcb(struct processControlBlock *pcb) {
  struct processControlBlock *debug;
  struct instruction *debugInst;

  debug = pcb;
  do {
    printf("PCB %s\n", debug->pagePtr->name);
    printf("State: %d\n", debug->processState);
    debugInst = debug->nextInstruction;
    do {
      if (debugInst == NULL) {
        break;
      }
      printf("(%d, %s, %s)\n", debugInst->type, debugInst->resource,
             debugInst->msg);
      debugInst = debugInst->next;
    } while (debugInst != NULL);

    debug = debug->next;
  } while (debug != NULL);
}

void debug_mailboxes(struct mailbox *mail) {
  struct mailbox *debug;
  debug = mail;
  do {
    printf("Mailbox %s\n", debug->name);
    debug = debug->next;
  } while (debug != NULL);
}
#endif
