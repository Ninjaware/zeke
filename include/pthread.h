/**
 *******************************************************************************
 * @file    pthread.h
 * @author  Olli Vanhoja
 * @brief   Threads.
 * @section LICENSE
 * Copyright (c) 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2013 Joni Hauhia <joni.hauhia@cs.helsinki.fi>
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy,
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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
 *
 *
 * Copyright (c) 1993, 1994 by Chris Provenzano, proven@mit.edu
 * Copyright (c) 1995-1998 by John Birrell <jb@cimlogic.com.au>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *  This product includes software developed by Chris Provenzano.
 * 4. The name of Chris Provenzano may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CHRIS PROVENZANO ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL CHRIS PROVENZANO BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *******************************************************************************
 */

/**
 * @addtogroup LIBC
 * @{
 */

#ifndef PTHREAD_H
#define PTHREAD_H

#include <sys/types_pthread.h>
#include <sys/types/_errno_t.h>
#include <sched.h>
#include <time.h>

/**
 * @addtogroup Threads
 * @{
 */

/*
 * Run-time invariant values:
 */
#define PTHREAD_DESTRUCTOR_ITERATIONS   4
#define PTHREAD_KEYS_MAX                256
#define PTHREAD_STACK_MIN               __MINSIGSTKSZ
#define PTHREAD_THREADS_MAX             __ULONG_MAX
#define PTHREAD_BARRIER_SERIAL_THREAD   -1

/*
 * Flags for threads and thread attributes.
 */
#define PTHREAD_DETACHED            0x1
#define PTHREAD_SCOPE_SYSTEM        0x2
#define PTHREAD_INHERIT_SCHED       0x4
#define PTHREAD_NOFLOAT             0x8

#define PTHREAD_CREATE_DETACHED     PTHREAD_DETACHED
#define PTHREAD_CREATE_JOINABLE     0
#define PTHREAD_SCOPE_PROCESS       0
#define PTHREAD_EXPLICIT_SCHED      0

/*
 * Flags for read/write lock attributes
 */
#define PTHREAD_PROCESS_PRIVATE     0
#define PTHREAD_PROCESS_SHARED      1

/*
 * Flags for cancelling threads
 */
#define PTHREAD_CANCEL_ENABLE       0
#define PTHREAD_CANCEL_DISABLE      1
#define PTHREAD_CANCEL_DEFERRED     0
#define PTHREAD_CANCEL_ASYNCHRONOUS 2
#define PTHREAD_CANCELED            ((void *) 1)

/*
 * Flags for once initialization.
 */
#define PTHREAD_NEEDS_INIT          0
#define PTHREAD_DONE_INIT           1

/*
 * Static once initialization values.
 */
#define PTHREAD_ONCE_INIT           { PTHREAD_NEEDS_INIT }

/*
 * Static initialization values.
 */
#define PTHREAD_MUTEX_INITIALIZER {0, 0, -1, -1, -1}
#define PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP {0, 0, -1, -1, -1}
#define PTHREAD_COND_INITIALIZER    NULL
#define PTHREAD_RWLOCK_INITIALIZER  NULL

/*
 * Default attribute arguments (draft 4, deprecated).
 */
#ifndef PTHREAD_KERNEL
#define pthread_condattr_default    NULL
#define pthread_mutexattr_default   NULL
#define pthread_attr_default        NULL
#endif

#define PTHREAD_PRIO_NONE           0
#define PTHREAD_PRIO_INHERIT        1
#define PTHREAD_PRIO_PROTECT        2

/**
 * Mutex types.
 *
 * Note that a mutex attribute with one of the following types:
 *
 * - PTHREAD_MUTEX_NORMAL
 * - PTHREAD_MUTEX_RECURSIVE
 *
 * will deviate from POSIX specified semantics.
 *
 */
enum pthread_mutextype {
    PTHREAD_MUTEX_NORMAL        = 0, /*!< No error checking */
    PTHREAD_MUTEX_RECURSIVE     = 1, /*!< Recursive mutex */
    PTHREAD_MUTEX_ERRORCHECK    = 2, /*!< Default POSIX mutex */
    PTHREAD_MUTEX_TYPE_MAX
};

#define PTHREAD_MUTEX_DEFAULT    PTHREAD_MUTEX_NORMAL

typedef int pthread_key_t;
typedef struct _pthread_once {
    int state;
    pthread_key_t key;
} pthread_once_t;

struct _pthread_cleanup_info {
    void (*rtn)(void *);
    void * arg;
    struct _pthread_cleanup_info * next;
};
extern pthread_key_t _pthread_cleanup_handler_key;

/**
 * A struct for thread local storage.
 */
struct _sched_tls_desc {
    pthread_t thread_id;    /*!< Thread id of the current thread. */
    errno_t errno_val;      /*!< Thread local errno. */
};

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
/**
 * Argument struct for SYSCALL_SCHED_THREAD_CREATE
 */
struct _sched_pthread_create_args {
    struct sched_param param;
    void          * stack_addr; /*!< Stack address */
    size_t          stack_size; /*!< Size of stack reserved for the thread. */
    unsigned        flags;      /*!< Thread creation flags */
    start_routine   start;      /*!< Thread start routine. */
    uintptr_t       arg1;
    uintptr_t       arg2;
    uintptr_t       arg3;
    uintptr_t       arg4;
    void (*del_thread)(void *); /*!< Thread exit function. */
};

/**
 * Argument struct for SYSCALL_SCHED_THREAD_JOIN
 */
struct _sched_pthread_join_args {
    pthread_t       thread_id;
    intptr_t      * retval; /* thread return value
                             * (a pointer by POSIX definition) */
};
#endif

#ifndef KERNEL_INTERNAL
__BEGIN_DECLS
/*
int     pthread_atfork(void (*)(void), void (*)(void), void (*)(void));
*/
int     pthread_attr_destroy(pthread_attr_t *);
int     pthread_attr_getstack(const pthread_attr_t * __restrict attr,
            void ** __restrict stackaddr, size_t * __restrict stacksize);
int     pthread_attr_getstacksize(const pthread_attr_t *, size_t *);
/*
int     pthread_attr_getguardsize(const pthread_attr_t *, size_t *);
*/
int     pthread_attr_getstackaddr(const pthread_attr_t *, void **);
/*
int     pthread_attr_getdetachstate(const pthread_attr_t *, int *);
*/
int     pthread_attr_init(pthread_attr_t *);
int     pthread_attr_setstacksize(pthread_attr_t *, size_t);
/*
int     pthread_attr_setguardsize(pthread_attr_t *, size_t);
*/
int     pthread_attr_setstack(pthread_attr_t *, void *, size_t);
int     pthread_attr_setstackaddr(pthread_attr_t *, void *);
int     pthread_attr_setdetachstate(pthread_attr_t *, int);
/*
int     pthread_barrier_destroy(pthread_barrier_t *);
int     pthread_barrier_init(pthread_barrier_t *,
            const pthread_barrierattr_t *, unsigned);
int     pthread_barrier_wait(pthread_barrier_t *);
int     pthread_barrierattr_destroy(pthread_barrierattr_t *);
int     pthread_barrierattr_getpshared(const pthread_barrierattr_t *,
            int *);
int     pthread_barrierattr_init(pthread_barrierattr_t *);
int     pthread_barrierattr_setpshared(pthread_barrierattr_t *, int);
*/

/**
 * Establish cancellation handlers.
 * @{
 */

#define pthread_cleanup_push(rtn, arg) {                        \
    Struct _pthread_cleanup_info * __head, __cleanup_handler =  \
        {.rtn = (rtn), .arg = (arg)};                           \
    __head = pthread_getspecific(_pthread_cleanup_handler_key); \
    __cleanup_handler.next = __head;                            \
    __head = &__cleanup_handler;                                \
    pthread_setspecific(_pthread_cleanup_handler_key, __head)

#define pthread_cleanup_pop(ex)                                 \
    pthread_setspecific(_pthread_cleanup_handler_key,           \
                        __cleanup_handler.next);                \
    if (ex && __cleanup_handler.rtn) {                          \
        __cleanup_handler.rtn(__cleanup_handler.arg); } }

/**
 * @}
 */

/*
int     pthread_condattr_destroy(pthread_condattr_t *);
int     pthread_condattr_getclock(const pthread_condattr_t *,
            clockid_t *);
int     pthread_condattr_getpshared(const pthread_condattr_t *, int *);
int     pthread_condattr_init(pthread_condattr_t *);
int     pthread_condattr_setclock(pthread_condattr_t *, clockid_t);
int     pthread_condattr_setpshared(pthread_condattr_t *, int);
int     pthread_cond_broadcast(pthread_cond_t *);
int     pthread_cond_destroy(pthread_cond_t *);
int     pthread_cond_init(pthread_cond_t *,
            const pthread_condattr_t *);
int     pthread_cond_signal(pthread_cond_t *);
int     pthread_cond_timedwait(pthread_cond_t *,
            pthread_mutex_t *__mutex, const struct timespec *);
int     pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *__mutex);
*/
int     pthread_equal(pthread_t, pthread_t);

void    *pthread_getspecific(pthread_key_t);
/*
int     pthread_getcpuclockid(pthread_t, clockid_t *);
*/

/**
 * Wait for thread termination.
 */
int     pthread_join(pthread_t thread, void ** value_ptr);

/**
 * Create a thread specific data key.
 * @param[out]  key
 * @param       destructor
 * @return EAGAIN, ENOMEM or 0
 */
int     pthread_key_create(pthread_key_t * key, void (*destructor)(void*));
int     pthread_key_delete(pthread_key_t key);
void    __pthread_key_dtors(void) __attribute__((destructor));
int     pthread_mutexattr_destroy(pthread_mutexattr_t *);
int     pthread_mutexattr_getpshared(const pthread_mutexattr_t *,
            int *);
int     pthread_mutexattr_gettype(pthread_mutexattr_t *, int *);
int     pthread_mutexattr_settype(pthread_mutexattr_t *, int);
int     pthread_mutexattr_setpshared(pthread_mutexattr_t *, int);
int     pthread_mutex_destroy(pthread_mutex_t *__mutex);
int     pthread_mutex_timedlock(pthread_mutex_t *__mutex,
            const struct timespec *);
/**
 * Dynamic package initialization.
 */
int     pthread_once(pthread_once_t *, void (*) (void));

/*
int     pthread_rwlock_destroy(pthread_rwlock_t *__rwlock);
int     pthread_rwlock_init(pthread_rwlock_t *__rwlock,
            const pthread_rwlockattr_t *);
int     pthread_rwlock_rdlock(pthread_rwlock_t *__rwlock);
int     pthread_rwlock_timedrdlock(pthread_rwlock_t *__rwlock,
            const struct timespec *);
int     pthread_rwlock_timedwrlock(pthread_rwlock_t *__rwlock,
            const struct timespec *);
int     pthread_rwlock_tryrdlock(pthread_rwlock_t *__rwlock);
int     pthread_rwlock_trywrlock(pthread_rwlock_t *__rwlock);
int     pthread_rwlock_unlock(pthread_rwlock_t *__rwlock);
int     pthread_rwlock_wrlock(pthread_rwlock_t *__rwlock);
int     pthread_rwlockattr_destroy(pthread_rwlockattr_t *);
int     pthread_rwlockattr_getkind_np(const pthread_rwlockattr_t *,
            int *);
int     pthread_rwlockattr_getpshared(const pthread_rwlockattr_t *,
            int *);
int     pthread_rwlockattr_init(pthread_rwlockattr_t *);
int     pthread_rwlockattr_setkind_np(pthread_rwlockattr_t *, int);
int     pthread_rwlockattr_setpshared(pthread_rwlockattr_t *, int);
*/
int     pthread_setspecific(pthread_key_t, const void *);
/*
int     pthread_spin_init(pthread_spinlock_t *__spin, int)
int     pthread_spin_destroy(pthread_spinlock_t *__spin);
int     pthread_spin_lock(pthread_spinlock_t *__spin);
int     pthread_spin_trylock(pthread_spinlock_t *__spin);
int     pthread_spin_unlock(pthread_spinlock_t *__spin);
*/
int     pthread_cancel(pthread_t);
/*
int     pthread_setcancelstate(int, int *);
int     pthread_setcanceltype(int, int *);
void    pthread_testcancel(void);
*/

#if __BSD_VISIBLE
#define pthread_getprio(_thread_id_) \
    getpriority(PRIO_THREAD, _thread_id_)
#define pthread_setprio(_pthread_id_, _prio_) \
    setpriority(PRIO_THREAD, _pthread_id_, _prio_)
#define thrd_yield() thrd_yield();
#endif

#if 0
int     pthread_mutexattr_getprioceiling(pthread_mutexattr_t *,
            int *);
int     pthread_mutexattr_setprioceiling(pthread_mutexattr_t *,
            int);
int     pthread_mutex_getprioceiling(pthread_mutex_t *, int *);
int     pthread_mutex_setprioceiling(pthread_mutex_t *, int, int *);
int     pthread_mutexattr_getprotocol(pthread_mutexattr_t *, int *);
int     pthread_mutexattr_setprotocol(pthread_mutexattr_t *, int);

int     pthread_attr_getinheritsched(const pthread_attr_t *, int *);
int pthread_attr_getschedparam(const pthread_attr_t *,
                               struct sched_param *);
int pthread_attr_getschedpolicy(const pthread_attr_t * restrict attr,
                                int * restrict policy);
int     pthread_attr_getscope(const pthread_attr_t *, int *);
int     pthread_attr_setinheritsched(pthread_attr_t *, int);
int     pthread_attr_setschedparam(pthread_attr_t *,
            const struct sched_param *);
int     pthread_attr_setschedpolicy(pthread_attr_t *, int);
int     pthread_attr_setscope(pthread_attr_t *, int);
int     pthread_getschedparam(pthread_t pthread, int *,
            struct sched_param *);
int     pthread_setschedparam(pthread_t, int,
            const struct sched_param *);
#endif

/**
 * Get calling thread's ID.
 *
 * The pthread_self() function returns the thread ID of the calling thread.
 * @return The thread ID of the calling thread.
 */
pthread_t pthread_self(void);

/**
 * Create a thread and add it to Active Threads and set it to state READY.
 * @param thread[out] returns ID of the new thread.
 * @param attr thread creation attributes.
 * @param start_routine
 * @param arg
 * @return If successful, the pthread_create() function returns zero. Otherwise,
 *         an error number is returned to indicate the error.
 */
int pthread_create(pthread_t * thread, const pthread_attr_t * attr,
        void * (*start_routine)(void *), void * arg);

/**
 * Thread termination.
 * @param retval is the return value of the thread.
 * @return This function does not return to the caller.
 */
void pthread_exit(void * retval);

/**
 * Indicate that storage for the thread thread can be reclaimed when that
 * thread terminates.
 * Calling this function will not terminate the thread if it's not already
 * terminated.
 */
int pthread_detach(pthread_t thread);


/**
 * Initialises the mutex referenced by mutex with attributes specified by attr.
 * If attr is NULL, the default mutex attributes are used.
 * @param mutex is a pointer to the mutex control block.
 * @param attr is a struct containing mutex attributes.
 * @return If successful return zero; Otherwise value other than zero.
 */
int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t * attr);

/**
 * Lock mutex.
 * If the mutex is already locked, the calling thread blocks until the mutex
 * becomes available. This operation returns with the mutex object referenced by
 * mutex in the locked state with the calling thread as its owner.
 * @param mutex is the mutex control block.
 * @return If successful return zero; Otherwise value other than zero.
 */
int pthread_mutex_lock(pthread_mutex_t * mutex);

/**
 * Try to lock mutex and return if can't acquire lock due to it is locked by
 * any thread including the current thread.
 * @param mutex is the mutex control block.
 * @return If successful return zero; Otherwise value other than zero.
 * @exception EBUSY The mutex could not be acquired because it was already locked.
 */
int pthread_mutex_trylock(pthread_mutex_t * mutex);

/**
 * Release the mutex object.
 * @param mutex is the mutex control block.
 * @return If successful return zero; Otherwise value other than zero.
 */
int pthread_mutex_unlock(pthread_mutex_t * mutex);

int pthread_mutexattr_init(pthread_mutexattr_t * attr);


/**
 * Send a signal to a thread.
 * @param thread    is a thread id that shall receive the signal,
 *                  if the value is negative signal is delivered to all
 *                  threads of the current process.
 * @param sig       is the signal to be delivered.
 * @return  Upon successful completion, the function returns a value
 *          of zero. Otherwise, the function will return an error number.
 *          If the function fails, no signal is delivered.
 */
int pthread_kill(pthread_t thread, int sig);

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* PTHREAD_H */

/**
 * @}
 */

/**
 * @}
 */
