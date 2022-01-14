#include <stdio.h>
#include "lwp.h"
#include <stdlib.h>
#include <stdbool.h>

unsigned long real_sp;      /* real stack pointer that will return to orignal func that calls it */
unsigned long PID = 0;      /* PID                                      */
int lwp_procs = 0;              /* the current number of LWPs               */
int lwp_running = 0;            /* the index of the currently running LWP   */
schedfun schedular = NULL;  /* schedular function                       */
void round_robin(bool type);

int new_lwp(lwpfun func, void *arg, size_t stack_size)
{
    if (lwp_procs == LWP_PROC_LIMIT) {
        return -1;
    }

    ptr_int_t *ebp, *esp;

    /* number of LWPs and PID will not necessarily match */
    lwp_procs++;
    PID++;
    lwp_running = lwp_procs;
    lwp_ptable[lwp_running].pid = PID;
    if (!(lwp_ptable[lwp_running].stack = malloc(sizeof(int) * stack_size)))
        return -1;
    lwp_ptable[lwp_running].stacksize = stack_size;
    ebp = esp = lwp_ptable[lwp_running].stack + lwp_ptable[lwp_running].stacksize;

    *esp = (ptr_int_t) arg;
    esp--;
    *esp = (ptr_int_t) lwp_exit;
    esp--;
    *esp = (ptr_int_t) func;
    esp--;
    *esp=0xB; /* bogus base pointer */
    ebp = esp;

    esp--;
    esp -=6; /* push the bogus registers */
    esp --; 
    *esp = (ptr_int_t) ebp; /* address of bogus base pointer */

    lwp_ptable[lwp_running].sp = esp;

    return lwp_running;
}

void lwp_exit() {

    /* free current thread stack as we are done with it */
    free(lwp_ptable[lwp_running].stack);

    /* If no more threads, call stop b/c this thread also exiting */
    lwp_procs--;
    if (lwp_procs <= 0) {
        lwp_stop();
    }

    /* shift ptable down to remove current process */
    for(int i = lwp_running + 1; i < lwp_procs + 1; i++) {
        lwp_ptable[i - 1] = lwp_ptable[i];
    }

    round_robin(false);

    SetSP(lwp_ptable[lwp_running].sp);
    RESTORE_STATE();
}

int lwp_getpid() {
   return lwp_ptable[lwp_running].pid;
}

void lwp_yield() {
    SAVE_STATE();
    GetSP(lwp_ptable[lwp_running].sp);
    round_robin(true);
    SetSP(lwp_ptable[lwp_running].sp);
    RESTORE_STATE();
}

void lwp_start() {
    if (lwp_procs == 0) {
        return;
    }
    SAVE_STATE();
    GetSP(real_sp);
    round_robin(false);
    SetSP(lwp_ptable[lwp_running].sp);
    RESTORE_STATE();
}

void lwp_stop() {
    SAVE_STATE();
    SetSP(real_sp)
    RESTORE_STATE();
}

void lwp_set_scheduler(schedfun sched) {
    schedular = sched;
}

void round_robin(bool type) {
    if (type) {
        if (lwp_running++ == lwp_procs) {
            lwp_running = 0;
        } else {
            lwp_running = schedfun();
        }
    } else {
        if (schedfun == NULL) {
            lwp_running = 0;
        } else {
            lwp_running = schedfun();
        }

    }
}