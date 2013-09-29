/**
 *******************************************************************************
 * @file    sched.c
 * @author  Olli Vanhoja
 * @brief   Kernel scheduler
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013, Ninjaware Oy, Olli Vanhoja <olli.vanhoja@ninjaware.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

/** @addtogroup Scheduler
  * @{
  */

#include <stdint.h>
#include <stddef.h>
#include <kstring.h>

#ifndef KERNEL_INTERNAL
#define KERNEL_INTERNAL
#endif

#include <hal/hal_core.h>
#include <hal/hal_mcu.h>
#include <heap.h>
#if configFAST_FORK != 0
#include <queue.h>
#endif
#include <timers.h>
#include <ksignal.h>
#include <syscall.h>
#include <kernel.h>
#include <sched.h>

/* Definitions for load average calculation **********************************/
#define LOAD_FREQ   (configSCHED_LAVG_PER * (int)configSCHED_FREQ)
/* FEXP_N = 2^11/(2^(interval * log_2(e/N))) */
#if configSCHED_LAVG_PER == 5
#define FSHIFT      11      /*!< nr of bits of precision */
#define FEXP_1      1884    /*!< 1/exp(5sec/1min) */
#define FEXP_5      2014    /*!< 1/exp(5sec/5min) */
#define FEXP_15     2037    /*!< 1/exp(5sec/15min) */
#elif configSCHED_LAVG_PER == 11
#define FSHIFT      11
#define FEXP_1      1704
#define FEXP_5      1974
#define FEXP_15     2023
#else
#error Incorrect value of kernel configuration configSCHED_LAVG_PER
#endif
#define FIXED_1     (1 << FSHIFT) /*!< 1.0 in fixed-point */
#define CALC_LOAD(load, exp, n)                  \
                    load *= exp;                 \
                    load += n * (FIXED_1 - exp); \
                    load >>= FSHIFT;
/** Scales fixed-point load average value to a integer format scaled to 100 */
#define SCALE_LOAD(x) (((x + (FIXED_1/200)) * 100) >> FSHIFT)
/* End of Definitions for load average calculation ***************************/

volatile uint32_t sched_enabled = 0; /* If this is set to != 0 interrupt
                                      * handlers will be able to call context
                                      * switching. */

/* Task containers */
static threadInfo_t task_table[configSCHED_MAX_THREADS]; /*!< Array of all
                                                          *   threads */
static heap_t priority_queue = HEAP_NEW_EMPTY;   /*!< Priority queue of active
                                                  * threads */
#if configFAST_FORK != 0
static queue_cb_t next_threadId_queue_cb;
static int next_threadId_queue[configSCHED_MAX_THREADS - 1];
#endif

volatile threadInfo_t * current_thread; /*!< Pointer to the currently active
                                         *   thread */
static uint32_t loadavg[3]  = { 0, 0, 0 }; /*!< CPU load averages */

/** Stack for idle task */
static char sched_idle_stack[sizeof(sw_stack_frame_t)
                             + sizeof(hw_stack_frame_t)
                             + 40]; /* Absolute minimum with current idle task
                                     * implementation */

/* Static init */
void sched_init(void) __attribute__((constructor));

/* Static function declarations **********************************************/
void idleTask(void * arg);
static void calc_loads(void);
static void context_switcher(void);
static void sched_thread_set(int i, osThreadDef_t * thread_def, void * argument, threadInfo_t * parent);
static void sched_thread_set_inheritance(osThreadId i, threadInfo_t * parent);
static void _sched_thread_set_exec(int thread_id, osPriority pri);
static void sched_thread_remove(osThreadId id);
static void del_thread(void);
/* End of Static function declarations ***************************************/

/* Functions called from outside of kernel context ***************************/

/**
 * Initialize the scheduler
 */
void sched_init(void)
{
    /* Create the idle task as task 0 */
    osThreadDef_t tdef_idle = { (os_pthread)(&idleTask),
                                osPriorityIdle,
                                sched_idle_stack,
                                sizeof(sched_idle_stack) / sizeof(char)
                              };
    sched_thread_set(0, &tdef_idle, NULL, NULL);

    /* Set idle thread as a currently running thread */
    current_thread = &(task_table[0]);

    /* sw_stack_frame will be overwritten when scheduler is run for the first
     * time. This also means that sw stacked registers are invalid when
     * idle task executes for the first time. */
    current_thread->sp = (uint32_t *)((uint32_t)(current_thread->sp)
                                          + sizeof(sw_stack_frame_t));

    /* Set initial value for PSP */
    wr_thread_stack_ptr(task_table[0].sp);

#if configFAST_FORK != 0
    next_threadId_queue_cb = queue_create(next_threadId_queue, sizeof(int),
        sizeof(next_threadId_queue) / sizeof(int));

    /* Fill the queue */
    int i = 1, q_ok;
    do {
        q_ok = queue_push(&next_threadId_queue_cb, &i);
        i++;
    } while (q_ok);
#endif
}

void sched_start(void)
{
    disable_interrupt();
    sched_enabled = 1;
    enable_interrupt();
}

/* End of Functions called from outside of kernel context ********************/

/**
 * Kernel idle task
 * @note sw stacked registers are invalid when this thread executes for the
 * first time.
 */
void idleTask(/*@unused@*/ void * arg)
{
#ifndef PU_TEST_BUILD
    while(1) {
        idle_sleep();
    }
#endif
}

#ifndef PU_TEST_BUILD
void sched_handler(void)
{
    current_thread->sp = (void *)rd_thread_stack_ptr();

    /* Ensure that this scheduler call was due to a systick */
    eval_kernel_tick();

    /* Pre-scheduling tasks */
    if (flag_kernel_tick) { /* Run only if tick was set */
        timers_run();
    }

    /* Call the actual context switcher function that schedules the next thread.
     */
    context_switcher();

    /* Post-scheduling tasks */
    if (flag_kernel_tick) {
        calc_loads();
    }
}
#endif

/**
 * Calculate load averages
 *
 * This function calculates unix-style load averages for the system.
 * Algorithm used here is similar to algorithm used in Linux, although I'm not
 * exactly sure if it's O(1) in current Linux implementation.
 */
static void calc_loads(void)
{
    uint32_t active_threads; /* Fixed-point */
    static int count = LOAD_FREQ;

    count--;
    if (count < 0) {
        count = LOAD_FREQ;
        active_threads = (uint32_t)(priority_queue.size * FIXED_1);

        /* Load averages */
        CALC_LOAD(loadavg[0], FEXP_1, active_threads);
        CALC_LOAD(loadavg[1], FEXP_5, active_threads);
        CALC_LOAD(loadavg[2], FEXP_15, active_threads);
    }
}

void sched_get_loads(uint32_t loads[3])
{
    loads[0] = SCALE_LOAD(loadavg[0]);
    loads[1] = SCALE_LOAD(loadavg[1]);
    loads[2] = SCALE_LOAD(loadavg[2]);
}

/**
 * Selects the next thread.
 */
static void context_switcher(void)
{
    /* Select the next thread */
    do {
        /* Get next thread from the priority queue */
        current_thread = *priority_queue.a;

        if (   ((current_thread->flags & SCHED_EXEC_FLAG) == 0)
            || ((current_thread->flags & SCHED_IN_USE_FLAG) == 0)) {
            /* Remove the top thread from the priority queue as it is
             * either in sleep or deleted */
            (void)heap_del_max(&priority_queue);
            continue; /* Select next thread */
        } else if ( /* if maximum time slices for this thread is used */
            (current_thread->ts_counter <= 0)
            /* and process is not a realtime process */
            && ((int)current_thread->priority < (int)osPriorityRealtime)
            /* and its priority is yet higher than low */
            && ((int)current_thread->priority > (int)osPriorityLow))
        {
            /* Penalties
             * =========
             * Penalties are given to CPU hog threads (CPU bound) to prevent
             * starvation of other threads. This is done by dynamically lowering
             * the priority (gives less CPU time in CMSIS RTOS) of a thread.
             */

            /* Give a penalty: Set lower priority
             * and perform reschedule operation on heap. */
            heap_reschedule_root(&priority_queue, osPriorityLow);

            continue; /* Select next thread */
        }

        /* Thread is skipped if its EXEC or IN_USE flags are unset. */
    } while ((current_thread->flags & SCHED_CSW_OK_FLAGS) != SCHED_CSW_OK_FLAGS);

    /* ts_counter is used to determine how many time slices has been used by the
     * process between idle/sleep states. We can assume that this measure is
     * quite accurate even it's not confirmed that one tick has been elapsed
     * before this line. */
    current_thread->ts_counter--;

    /* Write the value of the PSP for the next thread in exec */
    wr_thread_stack_ptr(current_thread->sp);
}

threadInfo_t * sched_get_pThreadInfo(int thread_id)
{
    if (thread_id > configSCHED_MAX_THREADS)
        return NULL;

    if ((task_table[thread_id].flags & (uint32_t)SCHED_IN_USE_FLAG) == 0)
        return NULL;

    return &(task_table[thread_id]);
}

/**
  * Set thread initial configuration
  * @note This function should not be called for already initialized threads.
  * @param i            Thread id
  * @param thread_def   Thread definitions
  * @param argument     Thread argument
  * @param parent       Parent thread id, NULL = doesn't have a parent
  * @todo what if parent is stopped before this function is called?
  */
static void sched_thread_set(int i, osThreadDef_t * thread_def, void * argument, threadInfo_t * parent)
{
    /* This function should not be called for already initialized threads. */
    if ((task_table[i].flags & (uint32_t)SCHED_IN_USE_FLAG) != 0)
        return;

    /* Init core specific stack frame */
    init_stack_frame(thread_def, argument, (uint32_t)del_thread);

    /* Mark that this thread position is in use.
     * EXEC flag is set later in sched_thread_set_exec */
    task_table[i].flags         = (uint32_t)SCHED_IN_USE_FLAG;
    task_table[i].id            = i;
    task_table[i].def_priority  = thread_def->tpriority;
    /* task_table[i].priority is set later in sched_thread_set_exec */

    /* Clear signal flags & wait states */
    task_table[i].signals = 0;
    task_table[i].sig_wait_mask = 0x0;
    task_table[i].wait_tim = -1;

    /* Clear events */
    memset(&(task_table[i].event), 0, sizeof(osEvent));

    /* Update parent and child pointers */
    sched_thread_set_inheritance(i, parent);

    /* Update stack pointer */
    task_table[i].sp = (void *)((uint32_t)(thread_def->stackAddr)
                                         + thread_def->stackSize
                                         - sizeof(hw_stack_frame_t)
                                         - sizeof(sw_stack_frame_t));

    /* Put thread into execution */
    _sched_thread_set_exec(i, thread_def->tpriority);
}

/**
 * Set thread inheritance
 * Sets linking from the parent thread to the thread id.
 */
static void sched_thread_set_inheritance(osThreadId id, threadInfo_t * parent)
{
    threadInfo_t * last_node, * tmp;

    /* Initial values for all threads */
    task_table[id].inh.parent = parent;
    task_table[id].inh.first_child = NULL;
    task_table[id].inh.next_child = NULL;

    if (parent == NULL)
        return;

    if (parent->inh.first_child == NULL) {
        /* This is the first child of this parent */
        parent->inh.first_child = &(task_table[id]);
        task_table[id].inh.next_child = NULL;

        return; /* All done */
    }

    /* Find last child thread
     * Assuming first_child is a valid thread pointer
     */
    tmp = (threadInfo_t *)(parent->inh.first_child);
    do {
        last_node = tmp;
    } while ((tmp = last_node->inh.next_child) != NULL);

    /* Set newly created thread as a last child */
    last_node->inh.next_child = &(task_table[id]);
}

void sched_thread_set_exec(int thread_id)
{
    _sched_thread_set_exec(thread_id, task_table[thread_id].def_priority);
}

/**
  * Set thread into execution mode/ready to run mode
  *
  * Sets EXEC_FLAG and puts thread into the scheduler's priority queue.
  * @param thread_id    Thread id
  * @param pri          Priority
  */
static void _sched_thread_set_exec(int thread_id, osPriority pri)
{
    /* Check that given thread is in use but not in execution */
    if ((task_table[thread_id].flags & (SCHED_EXEC_FLAG | SCHED_IN_USE_FLAG))
        == SCHED_IN_USE_FLAG) {
        task_table[thread_id].ts_counter = 4 + (int)pri;
        task_table[thread_id].priority = pri;
        task_table[thread_id].flags |= SCHED_EXEC_FLAG; /* Set EXEC flag */
        (void)heap_insert(&priority_queue, &(task_table[thread_id]));
    }
}

void sched_thread_sleep_current(void)
{
    /* Sleep flag */
    current_thread->flags &= ~SCHED_EXEC_FLAG;

    /* Find index of the current thread in the priority queue and move it
     * on top */
    current_thread->priority = osPriorityError;
    heap_inc_key(&priority_queue, heap_find(&priority_queue, current_thread->id));
}

/**
 * Removes a thread from execution.
 * @param tt_id Thread task table id
 */
static void sched_thread_remove(osThreadId tt_id)
{
    #if configFAST_FORK != 0
    /* next_threadId_queue may break if this is not checked, otherwise it should
     * be quite safe to remove a thread multiple times. */
    if ((task_table[tt_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return;
    }
    #endif

    task_table[tt_id].flags = 0; /* Clear all flags */

    /* Release wait timeout timer */
    if (task_table[tt_id].wait_tim >= 0) {
        timers_release(task_table[tt_id].wait_tim);
    }

    /* Increment the thread priority to the highest possible value so context
     * switch will garbage collect it from the priority queue on the next run.
     */
    task_table[tt_id].priority = osPriorityError;
    heap_inc_key(&priority_queue, heap_find(&priority_queue, tt_id));

#if configFAST_FORK != 0
    queue_push(&next_threadId_queue_cb, &tt_id);
#endif
}

/**
 * Deletes thread on exit
 * @note This function is called while execution is in thread context.
 */
static void del_thread(void)
{
    /* It's considered to be safer to call osThreadTerminate syscall here and
     * terminate the running process while in kernel context even there is
     * no separate privileged mode in Cortex-M0. This atleast improves
     * portability in the future.
     */
    osThreadId thread_id = (osThreadId)syscall(SYSCALL_SCHED_THREAD_GETID, NULL);
    (void)syscall(SYSCALL_SCHED_THREAD_TERMINATE, &thread_id);
    req_context_switch();

    while(1); /* Once the context changes, the program will no longer return to
               * this thread */
}


/* Functions defined in header file (and used mainly by syscalls)
 ******************************************************************************/

/**
  * @}
  */

/** @addtogroup Kernel
  * @{
  */

/** @addtogroup External_routines
  * @{
  */

/*  ==== Thread Management ==== */

osThreadId sched_threadCreate(osThreadDef_t * thread_def, void * argument)
{
    int i;

    /* Disable context switching to support multi-threaded calls to this fn */
    //__istate_t s = __get_interrupt_state();
    //__disable_interrupt();

#if configFAST_FORK != 0
    if (!queue_pop(&next_threadId_queue_cb, &i)) {
        /* New thread could not be created */
        return 0;
    }
#else
    for (i = 1; i < configSCHED_MAX_THREADS; i++) {
        if (task_table[i].flags == 0) {
#endif
            sched_thread_set(i,                  /* Index of created thread */
                             thread_def,         /* Thread definition */
                             argument,           /* Argument */
                             (void *)current_thread); /* Pointer to parent
                                                  * thread, which is expected to
                                                  * be the current thread */
#if configFAST_FORK == 0
            break;
        }
    }
#endif
    //__set_interrupt_state(s); /* Restore interrupts */

    if (i == configSCHED_MAX_THREADS) {
        /* New thread could not be created */
        return 0;
    } else {
        /* Return the id of the new thread */
        return (osThreadId)i;
    }
}

osThreadId sched_thread_getId(void)
{
    return (osThreadId)(current_thread->id);
}

osStatus sched_thread_terminate(osThreadId thread_id)
{
    threadInfo_t * child;

    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return (osStatus)osErrorParameter;
    }

    /* Remove all childs from execution */
    child = task_table[thread_id].inh.first_child;
    if (child != NULL) {
        do {
            sched_thread_terminate(child->id);
        } while ((child = child->inh.next_child) != NULL);
    }

    /* Remove thread itself */
    sched_thread_remove(task_table[thread_id].id);

    return (osStatus)osOK;
}

osStatus sched_thread_setPriority(osThreadId thread_id, osPriority priority)
{
    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return (osStatus)osErrorParameter;
    }

    /* Only thread def_priority is updated to make this syscall O(1)
     * Actual priority will be updated anyway some time later after one sleep
     * cycle.
     */
    task_table[thread_id].def_priority = priority;

    return (osStatus)osOK;
}

osPriority sched_thread_getPriority(osThreadId thread_id)
{
    if ((task_table[thread_id].flags & SCHED_IN_USE_FLAG) == 0) {
        return (osPriority)osPriorityError;
    }

    /* Not sure if this function should return "dynamic" priority or
     * default priorty.
     */
    return task_table[thread_id].def_priority;
}


/* ==== Generic Wait Functions ==== */

osStatus sched_threadDelay(uint32_t millisec)
{
    /* osOK is always returned from delay syscall if everything went ok,
     * where as threadWait returns a pointer which may change during
     * wait time. */
    current_thread->event.status = osOK;

    if (millisec != (uint32_t)osWaitForever) {
        if ((current_thread->wait_tim = timers_add(current_thread->id, TIMERS_FLAG_ENABLED, millisec)) < 0) {
             current_thread->event.status = osErrorResource;
        }
    }

    if (current_thread->event.status != osErrorResource) {
        /* This thread shouldn't get woken up by signals */
        current_thread->flags |= SCHED_NO_SIG_FLAG;
        sched_thread_sleep_current();
    }

    return current_thread->event.status;
}

osStatus sched_threadWait(uint32_t millisec)
{
    return ksignal_threadSignalWait(0x7fffffff, millisec);
}

/**
  * @}
  */

/* Syscall handlers ***********************************************************/
/** @addtogroup Syscall_handlers
  * @{
  */

uint32_t sched_syscall(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_SCHED_DELAY:
        return (uint32_t)sched_threadDelay(
                    *((uint32_t *)(p))
                );

    case SYSCALL_SCHED_WAIT:
        return (uint32_t)sched_threadWait(
                    *((uint32_t *)(p))
                );

    case SYSCALL_SCHED_GET_LOADAVG:
        sched_get_loads((uint32_t *)p);
        return (uint32_t)NULL; /* Note NULL is not an error here */

    case SYSCALL_SCHED_EVENT_GET:
        *((osEvent *)(p)) = current_thread->event;
        return (uint32_t)NULL;

    default:
        return (uint32_t)NULL;
    }
}

uint32_t sched_syscall_thread(uint32_t type, void * p)
{
    switch(type) {
    case SYSCALL_SCHED_THREAD_CREATE:
        return (uint32_t)sched_threadCreate(
                    ((ds_osThreadCreate_t *)(p))->def,
                    ((ds_osThreadCreate_t *)(p))->argument
                );

    case SYSCALL_SCHED_THREAD_GETID:
        return (uint32_t)sched_thread_getId();

    case SYSCALL_SCHED_THREAD_TERMINATE:
        return (uint32_t)sched_thread_terminate(
                    *((osThreadId *)p)
                );

    case SYSCALL_SCHED_THREAD_SETPRIORITY:
        return (uint32_t)sched_thread_setPriority(
                    ((ds_osSetPriority_t *)(p))->thread_id,
                    ((ds_osSetPriority_t *)(p))->priority
                );

    case SYSCALL_SCHED_THREAD_GETPRIORITY:
        return (uint32_t)sched_thread_getPriority(
                    *((osPriority *)(p))
                );

    default:
        return (uint32_t)NULL;
    }
}

/**
  * @}
  */

/**
  * @}
  */
