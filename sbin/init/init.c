/**
 *******************************************************************************
 * @file    init.c
 * @author  Olli Vanhoja
 * @brief   First user scope process.
 * @section LICENSE
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

#include <kernel.h>
#include <syscall.h>
#include <kstring.h> /* TODO remove */
#include <libkern.h> /* TODO */
#include <sys/sysctl.h>
#include <unistd.h>
#include <errno.h>
#include "init.h"

dev_t dev_tty0 = DEV_MMTODEV(2, 0);

char banner[] = "\
|'''''||                    \n\
    .|'   ...'||            \n\
   ||   .|...|||  ..  ....  \n\
 .|'    ||    || .' .|...|| \n\
||......|'|...||'|. ||      \n\
             .||. ||.'|...'\n\n\
";

extern int tish(void);

static void run_ikut(void);
static void print_mib_name(int * mib, int len);
static void * test_thread(void *);
static void print_message(const char * message);
static void thread_stat(void);

static char main_stack2[8192];

void * main(void * arg)
{
    int mib_tot[3];
    int mib_free[3];
    int mib_klog[3];
    int len;
    int old_value_tot, old_value_free;
    size_t old_len = sizeof(old_value_tot);
    char buf[80];

    pthread_attr_t attr = {
        .tpriority  = configUSRINIT_PRI,
        .stackAddr  = main_stack2,
        .stackSize  = sizeof(main_stack2)
    };
    pthread_t thread_id;

    print_message("Init v0.0.1\n");

#if 0
    pid_t pid = fork();
    if (pid == -1) {
        print_message("Failed\n");
        while(1);
    } else if (pid == 0) {
        print_message("Hello\n");
        while(1);
    } else {
        print_message("original\n");
    }
#endif

    while (1)
        tish();
    //run_ikut();

    pthread_create(&thread_id, &attr, test_thread, 0);

    len = sysctlnametomib("vm.dynmem_tot", mib_tot, 3);
    sysctlnametomib("vm.dynmem_free", mib_free, 3);
    sysctlnametomib("kern.klogger", mib_klog, 3);

#if 0
    /* Disable logging */
    int new_klog = 0;
    sysctl(mib_klog, len, 0, 0, &new_klog, sizeof(new_klog));
#endif

    while(1) {
        thread_stat();
        if(sysctl(mib_tot, len, &old_value_tot, &old_len, 0, 0)) {
            ksprintf(buf, sizeof(buf), "Error: %u\n",
                    (int)syscall(SYSCALL_SCHED_THREAD_GETERRNO, NULL));
            print_message(buf);
        }
        if(sysctl(mib_free, len, &old_value_free, &old_len, 0, 0)) {
            ksprintf(buf, sizeof(buf), "Error: %u\n",
                    (int)syscall(SYSCALL_SCHED_THREAD_GETERRNO, NULL));
            print_message(buf);
        }
        ksprintf(buf, sizeof(buf), "dynmem allocated: %u/%u\n",
                old_value_tot - old_value_free, old_value_tot);
        print_message(buf);
        sleep(5);
    }
}

static void run_ikut(void)
{
    char buf[80];
    int mib_test[5];
    int mib_next[5];
    size_t len, len_next;
    int  err;
    const int one = 1;

    len = sysctlnametomib("debug.test", mib_test, num_elem(mib_test));

    print_message("     \n"); /* Hack */
    print_mib_name(mib_test, len);

    memcpy(mib_next, mib_test, len * sizeof(int));
    len_next = len;

    while ((len_next = sizeof(mib_next)),
            (err = sysctlgetnext(mib_next, len_next, mib_next, &len_next)) == 0) {
        if (!sysctltstmib(mib_next, mib_test, len)) {
            print_message("End of tests\n");
            break; /* EOF debug.test */
        }

        print_mib_name(mib_next, len_next);
        sysctl(mib_next, len_next, 0, 0, (void *)(&one), sizeof(one));
    }

    ksprintf(buf, sizeof(buf), "errno = %i\n", errno);
    print_message(buf);
}

static void print_mib_name(int * mib, int len)
{
    char buf[80];
    char buf2[80];
    char strname[40];
    char strdesc[40];
    size_t strname_len = sizeof(strname);
    size_t strdesc_len = sizeof(strdesc);

    for (int i = 0; i < len; i++) {
        ksprintf(buf2, sizeof(buf2), "%s.%u", buf, mib[i]);
        memcpy(buf, buf2, sizeof(buf));
    }
    sysctlmibtoname(mib, len, strname, &strname_len);
    sysctlgetdesc(mib, len, strdesc, &strdesc_len);
    strname[sizeof(strname) - 1] = '\0';
    strdesc[sizeof(strdesc) - 1] = '\0';
    ksprintf(buf2, sizeof(buf2), "MIB:%s: %s : %s\n", buf + 1, strname, strdesc);
    print_message(buf2);
}

static void * test_thread(void * arg)
{
    while(1) {
        sleep(10);
        thread_stat();
    }
}

static void print_message(const char * message)
{
    write(2, message, strlenn(message, 80));
}

static void thread_stat(void)
{
    /* Print thread id & cpu mode */
    int id;
    uint32_t mode;
    char buf[80];

    id = (int)syscall(SYSCALL_SCHED_THREAD_GETTID, NULL);
    __asm__ volatile ("mrs     %0, cpsr" : "=r" (mode));
    ksprintf(buf, sizeof(buf), "My id: %u, my mode: %x\n", id, mode);
    print_message(buf);
}
