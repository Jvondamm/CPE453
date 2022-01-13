#include <stdio.h>
#include "lwp.h"
#include <stdlib.h>


unsigned long PID = 0;   /* PID                                      */
lwp_procs = 0;           /* the current number of LWPs               */
lwp_running = 0;         /* the index of the currently running LWP   */
/* do I need to malloc lwp_ptable with LWP_PROC_LIMIT here? */

int new_lwp(lwpfun func, void *arg, size_t stack_size)
{
    if (lwp_procs == LWP_PROC_LIMIT) {
        return -1;
    }

    ptr_int_t * ebp, * esp;

    /* number of LWPs and PID will not necessarily match */
    lwp_procs++;
    PID++;
    lwp_running = lwp_procs;
    lwp_ptable[lwp_running].pid = PID;
    if (!(lwp_ptable[lwp_running].stack = malloc(sizeof(int) * stack_size)))
        return -1;
    lwp_ptable[lwp_running].stacksize = stack_size;

    ebp = esp = lwp_ptable[lwp_running].stack + lwp_ptable[lwp_running].stacksize;

    *esp = arg;
    esp--;
    *esp = lwp_exit;
    esp--;
    *esp = func;
    esp--;
    *esp=NULL; /* bogus base pointer */
    esp--;

    esp -=6; /* push the bogus registers? */

    lwp_ptable[lwp_running].sp = esp;

    return lwp_running;
}

void lwp_exit() {
    lwp_procs--;
    if (lwp_procs <= 0) {
        lwp_stop();
    }
    free(lwp_ptable[lwp_running].stack);
}

int lwp_getpid() {
   return lwp_ptable[lwp_running].pid;
}

void lwp_yield() {
   SAVE_STATE();
    /* TODO: who knows? */

    RESTORE_STATE(); /* restore state of next process */
}

void lwp_start() {
    if (lwp_procs == 0) {
        return;
    }
}

void lwp_stop()

void lwp_set_scheduler(schedfun sched)