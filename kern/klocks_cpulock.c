/**
 *******************************************************************************
 * @file    klocks_cpulock.h
 * @author  Olli Vanhoja
 * @brief   Kernel space locks.
 * @section LICENSE
 * Copyright (c) 2014 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <kmalloc.h>
#include <ksched.h>
#include <klocks.h>

cpulock_t * cpulock_create(void)
{
    cpulock_t * lock;

    lock = kzalloc(sizeof(cpulock_t) + KSCHED_CPU_COUNT * sizeof(mtx_t));
    if (!lock)
        return NULL;

    for (int i = 0; i < KSCHED_CPU_COUNT; i++) {
        mtx_init(&lock->mtx[i], MTX_TYPE_TICKET, MTX_OPT_DEFAULT);
    }

    return lock;
}

void cpulock_destroy(cpulock_t * lock)
{
    kfree(lock);
}

int cpulock_lock(cpulock_t * lock)
{
    return mtx_lock(&lock->mtx[get_cpu_index()]);
}

void cpulock_unlock(cpulock_t * lock)
{
    mtx_unlock(&lock->mtx[get_cpu_index()]);
}
