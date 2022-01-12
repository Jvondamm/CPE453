#include <stdio.h>
#include "lwp.h"
#include <stdlib.h>


unsigned long PID = 0;              /* PID                                      */
lwp_procs = 0;                      /* the current number of LWPs               */
extern int lwp_running = 0;         /* the index of the currently running LWP   */

int new_lwp(lwpfun func, void * arg, size_t stack_size)
{
    if (lwp_procs == LWP_PROC_LIMIT) {
        return -1
    }

    /* number of LWPs and PID will not necessarily match */
    lwp_procs++;
    PID++;
    lwp_running = lwp_procs;
    lwp_ptable[lwp_running].pid = PID;
    lwp_ptable[lwp_running].stack = malloc(sizeof(int) * stack_size);
    lwp_ptable[lwp_running].stacksize = stack_size;

    esp

    return lwp_ptable[lwp_running].pid;
}

void lwp_exit();

int lwp_getpid() {
   return lwp_ptable[lwp_running].pid;
}

void lwp_yield() {
   SAVE_STATE();
    /* TODO: who knows? */
}

void lwp_start()

void lwp_stop()
{
    /* TODO: Restore original stack pointer.*/
    RESTORE_STATE();
}

void lwp_set_scheduler(schedfun sched);