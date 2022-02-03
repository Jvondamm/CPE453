#include <stdio.h>
#include "lwp.h"
#include <stdlib.h>
#include <stdbool.h>

unsigned long real_sp;                  /* real stack pointer                       */
unsigned long PID = 0;                  /* PID                                      */
int lwp_procs = 0;                      /* the current number of LWPs               */
int lwp_running = 0;                    /* the index of the currently running LWP   */
schedfun scheduler = NULL;              /* scheduler function                       */
lwp_context lwp_ptable[LWP_PROC_LIMIT]; /* thread table to hold the lwp_context     */
void round_robin(bool type);            /* function to round robin                  */

int new_lwp(lwpfun func, void *arg, size_t stack_size)
{
    /* if the number of threads exceed our limit, no new lwp's. */
    if (lwp_procs == LWP_PROC_LIMIT) {
        return -1;
    }

    /* declare stack pointer and base pointer */
    ptr_int_t *ebp, *esp;

    /* number of LWPs and PID will not necessarily match */
    /*lwp_procs++;*/
    PID++;

    /* set the current running process to be the one we are creating */
    lwp_running = lwp_procs;
    lwp_procs++;

    /* set the PID */
    lwp_ptable[lwp_running].pid = PID;

    /* malloc size for the stack int (word) * 4 */
    if (!(lwp_ptable[lwp_running].stack = malloc(sizeof(int) * stack_size)))
        return -1;

    /* save stack size */
    lwp_ptable[lwp_running].stacksize = stack_size * sizeof(int);

    /* stack starts at high addr. and GROWS DOWN, so the base pointer and
    initial stack pointer start at the top */
    ebp = esp = lwp_ptable[lwp_running].stack + lwp_ptable[lwp_running].stacksize;

    /*
    Let's say a function is running that is called by main. What
    would we expect to see on the stack?
    */

    /* first, its args, so we push those and DECREMENT the stack pointer,
    as we are growing down */
    *esp = (ptr_int_t) arg;
    esp--;

    /* After the function goes through its arguments, it will want a return
    address to go back to the line main that called it IF it was called
    organically. However in this case we don't want the thread to return
    to main, we want the process running the func to exit. So we instead
    call lwp_exit */
    *esp = (ptr_int_t) lwp_exit;
    esp--;

    /* These next two are the stack for lwp_start(). This is so lwp_start
    starts the func instead of returning to main. To do so, we then want
    to push a base pointer. It is called the bogus base pointer because
    lwp_start isn't going to return to main */
    *esp = (ptr_int_t) func;
    esp--;
    *esp=0xB;
    ebp = esp; /* base pointer = stack pointer (explained later) */

    /* We always want to RESTORE_STATE() before running a thread
    because the registers on the stack are from the previous process.
    In this case however, we have allocated a new stack and don't have
    any registers. So we push 6 bogus registers to mimic a SAVE_STATE() */
    esp -=7;    

    /* We then want the address of the bogus B.P. to be
    pushed here so that... I forgor */
    *esp = (ptr_int_t) ebp;

    /* we want to save the stack pointer so that when we
    call this thread it will know where in memory to start */
    lwp_ptable[lwp_running].sp = esp;

    /* return the process ID */
    return lwp_running;
}

void lwp_exit() {

    /* This thread has finished executing, and we no longer need its
    stack, so free the memory to avoid a memory leak */
    free(lwp_ptable[lwp_running].stack);

    /* If there are no more threads existing, we want to terminate */
    lwp_procs--;
    if (lwp_procs <= 0) {
        lwp_stop();
    }

    /* If there are threads remaining, we want to remove the current
    thread from the process table, so we shift down the current
    threads. */
    int i;
    for(i = lwp_running + 1; i < lwp_procs + 1; i++) {
        lwp_ptable[i - 1] = lwp_ptable[i];
    }

    /* Because we moved each thread down, we don't want to increase
    the index of the scheduler to execute the next thread (round robin)
    because then it would skip the first one, as we moved it down. So
    we pass the false value so it won't default to roun robin scheduling. */
    round_robin(false);

    /* now the lwp_running id is set to the next process, so we SETSP()
    which sets the stack pointer in the resgiter to the thread's sp. */
    SetSP(lwp_ptable[lwp_running].sp);

    /* We don't want to leave registers on the stack, so we pop them off */
    RESTORE_STATE();
}

/* simply returns the current PID */
int lwp_getpid() {
   return lwp_ptable[lwp_running].pid;
}

void lwp_yield() {
    /* save the current threads' context so when its run again, it can
    resume where it left off */
    SAVE_STATE();

    /* save the address of the stack pointer of the thread we are interrupting */
    GetSP(lwp_ptable[lwp_running].sp);

    /* round robin, go to the next thread */
    round_robin(true);

    /* set the stack pointer register to the new threads sp */
    SetSP(lwp_ptable[lwp_running].sp);

    /* pop off all the registers for a clean stack */
    RESTORE_STATE();
}

void lwp_start() {
    /* if there are no threads ready to run, meaning we never called
    lwp_new(), then immediately return */
    if (lwp_procs == 0) {
        return;
    }

    /* else push the registers onto the stack */
    SAVE_STATE();

    /* get the real stack pointer, meaning the address of where main called
    the lwp so we can get back to there */
    GetSP(real_sp);

    /* we don't want to round robin, but want to run the first thread */
    round_robin(false);

    /* set the stack pointer to the sp of the thread we want to run */
    SetSP(lwp_ptable[lwp_running].sp);

    /* always RESTORE_STATE() before running */
    RESTORE_STATE();
}

void lwp_stop() {

    /* if we want to stop the current thread, simply save the registers,
    return to main, then pop the registers off for a clean state. This is
    done so if we call lwp_start again, we can start where we left off */
    SAVE_STATE();
    SetSP(real_sp);
    RESTORE_STATE();
}

/* sets the scheduler */
void lwp_set_scheduler(schedfun sched) {
    scheduler = sched;
}

/* if true, does round-robin scheduling, which means it will simply
increment the process ID to the next one, and if we are at the end
of the process table, loop back to the first one. If false, either
we are starting the first thread and don't want to increment,
or we have deleted a thread (lwp_exit) so we also don't want to
increment or we will skip a thread. I kinda got lazy and don't
wana put the comments inside this func */
void round_robin(bool type) {
    if (type) {
        if (lwp_running + 1 == lwp_procs) {
            lwp_running = 0;
        } else {
            if (scheduler == NULL) {
                lwp_running++;
            } else {
                lwp_running = scheduler();
            }
        }
    } else {
        if (scheduler == NULL) {
            lwp_running = 0;
        } else {
            lwp_running = scheduler();
        }

    }
}


