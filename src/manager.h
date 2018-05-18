/**
 * @file manager.h
 */
#ifndef _MANAGER_H
#define _MANAGER_H

#include "loader.h"

void schedule_processes(struct processControlBlock *pcb,
    struct resourceList *resource, struct mailbox *mail, int quantum, int schedule_alg);

void schedule_processes_fcfs(struct processControlBlock *pcb,
    struct resourceList *resource, struct mailbox *mail);

void schedule_processes_rr(struct processControlBlock *pcb,
    struct processControlBlock *firstPCB,
		struct resourceList *resource, struct mailbox *mail, int quantum,int num, int tally);

void process_to_readyq(struct cpuSchedule *schedule,
	struct processControlBlock *proc);

#endif
