/**
 *******************************************************************************
 * @file    pthread.h
 * @author  Olli Vanhoja
 * @brief   Threads.
 * @section LICENSE
 * Copyright (c) 2020 Olli Vanhoja <olli.vanhoja@alumni.helsinki.fi>
 * Copyright (c) 2013 - 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 *******************************************************************************
 */

#ifndef TYPES_PTHREAD_H
#define TYPES_PTHREAD_H

#include <stddef.h>
#include <stdint.h>
#include <sched.h>
#include <sys/_sigset.h>
#include <sys/types/_pthread_t.h>

#define _ZEKE_THREAD_NAME_SIZE 16

/* TODO Missing types:
 * - pthread_barrier_t
 * - pthread_barrierattr_t
 * - pthread_cond_t
 * - pthread_condattr_t
 * - pthread_mutex_t
 * - pthread_mutexattr_t
 *   pthread_rwlock_t
 * - pthread_rwlockattr_t
 * - pthread_spinlock_t
 */

/**
 * Entry point of a thread.
 */
typedef void * (*start_routine)(void *);

/**
 * Thread Definition structure contains startup information of a thread.
 */
typedef struct pthread_attr {
    struct sched_param param;
    void *          stack_addr; /*!< Stack address */
    size_t          stack_size; /*!< Size of stack reserved for the thread. */
    unsigned        flags;
    char            name[_ZEKE_THREAD_NAME_SIZE];
} pthread_attr_t;

typedef struct pthread_condattr {
    int dummy;
} pthread_condattr_t;

/**
 * Mutex Definition structure contains setup information for a mutex.
 */
typedef struct pthread_mutexattr {
    int pshared;
    int kind;
} pthread_mutexattr_t;

typedef struct pthread_mutex {
    int lock;       /*!< Exclusive access to mutex state:
                     * - 0: unlocked/free
                     * - 1: locked - no other waiters
                     * - -1: locked - with possible other waiters
                     */

  int recursion;    /*!< Number of unlocks a thread needs to perform
                     *   before the lock is released (recursive mutexes only)
                     */
  int kind;         /*!< Mutex type */
  pthread_t owner;  /*!< Thread owning the mutex */
} pthread_mutex_t;

/*
 * Once definitions.
 */
struct pthread_once {
    int             state;
    pthread_mutex_t mutex;
};

#endif /* TYPES_PTHREAD_H */
