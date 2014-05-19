/**
 *******************************************************************************
 * @file    pthread.h
 * @author  Olli Vanhoja
 * @brief   Threads.
 * @section LICENSE
 * Copyright (c) 2013 Joni Hauhia <joni.hauhia@cs.helsinki.fi>
 * Copyright (c) 2013, 2014 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup LIBC
  * @{
  */

#pragma once
#ifndef PTHREAD_H
#define PTHREAD_H

#include <sys/cdefs.h>
#include <sys/types_pthread.h>

/** @addtogroup Threads
  * @{
  */

/* TODO Missing most of the standard declarations */
/* TODO errnos */

__BEGIN_DECLS
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


/* TODO Legacy */

/// Terminate execution of a thread and remove it from Active Threads.
/// \param[in]     thread_id   thread ID obtained by \ref osThreadCreate or \ref osThreadGetId.
/// \return status code that indicates the execution status of the function.
int osThreadTerminate(pthread_t thread_id);
__END_DECLS

#endif /* PTHREAD_H */

/**
 * @}
 */

/**
 * @}
 */
