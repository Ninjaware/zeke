/**
 *******************************************************************************
 * @file hw_timers.h
 * @author Olli Vanhoja
 * @brief HW timer services.
 * @section LICENSE
 * Copyright (c) 2014 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#pragma once
#ifndef HW_TIMERS_H
#define HW_TIMERS_H

#include <stddef.h>
#include <stdint.h>

/**
 * Get us timestamp.
 * @return Returns us timestamp.
 */
uint64_t get_utime(void);

void udelay(uint32_t delay);

#define TIMEOUT_WAIT(stop_if_true, usec)                \
do {                                                    \
    uint64_t i = (usec), __start_time = get_utime();    \
    do {                                                \
        if((stop_if_true))                              \
            break;                                      \
    } while((get_utime() - __start_time) >= i);         \
} while(0)

typedef void timer_task_t(void);

#define TIMER_TASK(fun) \
    DATA_SET(timer_tasks, fun)

/**
 * Run timer tasks.
 * Shall be called by the HW specific timer handler.
 */
void hw_timers_run(void);

#endif /* HW_TIMERS_H */
