/**
 *******************************************************************************
 * @file    kerror.c
 * @author  Olli Vanhoja
 * @brief   Kernel error logging.
 * @section LICENSE
 * Copyright (c) 2014, 2015 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

#include <errno.h>
#include <sys/linker_set.h>
#include <sys/sysctl.h>
#include <fs/fs.h>
#include <fs/fs_util.h>
#include <hal/core.h>
#include <hal/uart.h>
#include <kerror.h>
#include <kinit.h>
#include <kstring.h>
#include <libkern.h>

#ifdef configKLOGGER
const char * const _kernel_panic_msg = "Oops, Kernel panic\n";
#endif

#define NR_PRINTBUFS 8

static char kerror_printbuf_str[configKERROR_MAXLEN * NR_PRINTBUFS];
static isema_t kerror_printbuf_sema[NR_PRINTBUFS] =
    ISEMA_INITIALIZER(NR_PRINTBUFS);

static ssize_t kerror_fdwrite(file_t * file, struct uio * uio, size_t count);

vnode_ops_t kerror_vops = {
    .write = kerror_fdwrite,
};

vnode_t kerror_vnode = {
    .vn_num = 0,
    .vn_refcount = ATOMIC_INIT(1),
    .vn_len = SIZE_MAX,
    .vnode_ops = &kerror_vops
};

SET_DECLARE(klogger_set, struct kerror_klogger);

extern void kerror_buf_puts(const char * str);
void (*kputs)(const char *) = &kerror_buf_puts; /* Boot value */
static size_t curr_klogger_id = KERROR_BUF;     /* Boot value */
static char klogger_level = KERROR_INFO;

static int klogger_change(size_t new_id, size_t old_id);

int __kinit__ kerror_init(void)
{
    SUBSYS_INIT("kerror logger");

    int err;

    klogger_level =
#if defined(configDEBUG_CRIT)
        KERROR_CRIT;
#elif defined(configDEBUG_ERR)
        KERROR_ERR;
#elif defined(configDEBUG_WARN)
        KERROR_WARN;
#elif defined(configDEBUG_INFO)
        KERROR_INFO;
#elif defined(configDEBUG_DEBUG)
        KERROR_DEBUG;
#else
        KERROR_INFO;
#endif

    fs_inherit_vnops(&kerror_vops, &nofs_vnode_ops);

    /*
     * We can now change from klogger buffer to the actual logger selected at
     * compilation time.
     */
    err = klogger_change(configDEF_KLOGGER, curr_klogger_id);
    if (err)
        return err;

    return 0;
}

int _kerror_log_level_ge(char level)
{
    return klogger_level >= level;
}

size_t _kerror_acquire_buf(char ** buf)
{
    size_t i;

    i = isema_acquire(kerror_printbuf_sema, num_elem(kerror_printbuf_sema));
    *buf = &kerror_printbuf_str[i * configKERROR_MAXLEN];

    return i;
}

void _kerror_release_buf(size_t index)
{
    isema_release(kerror_printbuf_sema, index);
}

void _kerror_panic(const char * where, const char * msg)
{
    char * buf;

    disable_interrupt();
    _kerror_acquire_buf(&buf);
    ksprintf(buf, configKERROR_MAXLEN, "Oops, Kernel panic\n%s %s\n",
             where, msg);
    kputs(buf);
    panic_halt();
}

/**
 * Kernel fake fd write function to print kerror messages from usr mode threads.
 */
static ssize_t kerror_fdwrite(file_t * file, struct uio * uio, size_t count)
{
    void * buf;
    int err;

    err = uio_get_kaddr(uio, &buf);
    if (err)
        return err;
    kputs(buf);

    return count;
}

static void nolog_puts(const char * str)
{
}

static const struct kerror_klogger klogger_nolog = {
    .id     = KERROR_NOLOG,
    .init   = NULL,
    .puts   = &nolog_puts,
    .read   = NULL,
    .flush  = NULL
};
DATA_SET(klogger_set, klogger_nolog);

static struct kerror_klogger * get_klogger(size_t id)
{
    struct kerror_klogger ** klogger;

    SET_FOREACH(klogger, klogger_set) {
        if ((*klogger)->id == id)
            return *klogger;
    }

    return 0;
}

static int klogger_change(size_t new_id, size_t old_id)
{
    struct kerror_klogger * new = get_klogger(new_id);
    struct kerror_klogger * old = get_klogger(old_id);

    if (!new || !old)
        return -EINVAL;

    if (new->init)
        new->init();

    kputs = new->puts;

    if (old->flush)
        old->flush();

    curr_klogger_id = new_id;

    return 0;
}

SYSCTL_DECL(_kern_klogger);
SYSCTL_NODE(_kern, OID_AUTO, klogger, CTLFLAG_RD, 0,
            "Kernel logger");

/**
 * sysctl function to read current klogger and change it.
 */
static int sysctl_kern_klogger_type(SYSCTL_HANDLER_ARGS)
{
    int error;
    size_t new_klogger = curr_klogger_id;
    size_t old_klogger = curr_klogger_id;

    error = sysctl_handle_int(oidp, &new_klogger, sizeof(new_klogger), req);
    if (!error && req->newptr) {
        error = klogger_change(new_klogger, old_klogger);
    }

    return error;
}
SYSCTL_PROC(_kern_klogger, OID_AUTO, type, CTLTYPE_INT | CTLFLAG_RW,
            NULL, 0, sysctl_kern_klogger_type, "I", "Kernel logger type.");

static int sysctl_kern_klogger_level(SYSCTL_HANDLER_ARGS)
{
    int error;
    int new_level = klogger_level - '0';

    error = sysctl_handle_int(oidp, &new_level, sizeof(int), req);
    if (!error && req->newptr) {
        if (new_level >= 0 && new_level <= 4) {
            klogger_level = '0' + (char)new_level;
        } else {
            error = -EINVAL;
        }
    }

    return error;
}
SYSCTL_PROC(_kern_klogger, OID_AUTO, level, CTLTYPE_INT | CTLFLAG_RW,
            NULL, 0, sysctl_kern_klogger_level, "I", "Kernel logger level.");
