/**
 *******************************************************************************
 * @file    ksignal.c
 * @author  Olli Vanhoja
 *
 * @brief   Source file for thread Signal Management in kernel.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * Copyright (c) 1982, 1986, 1989, 1991, 1993
 *        The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *        @(#)kern_sig.c        8.7 (Berkeley) 4/18/94
 *******************************************************************************
 */

#include <errno.h>
#include <sys/_signames.h>
#include <sys/param.h>
#include <sys/priv.h>
#include <sys/sysctl.h>
#include <sys/tree.h>
#include <sys/types.h>
#include <core.h>
#include <kerror.h>
#include <kmalloc.h>
#include <ksched.h>
#include <kstring.h>
#include <libkern.h>
#include <proc.h>
#include <syscall.h>
#include <timers.h>
#include <vm/vm.h>
#include "ksignal.h"

#define KSIG_LOCK_TYPE  MTX_TYPE_TICKET
#define KSIG_LOCK_FLAGS (MTX_OPT_DINT)

static int kern_logsigexit = 1;
SYSCTL_BOOL(_kern, KERN_LOGSIGEXIT, logsigexit, CTLFLAG_RW,
            &kern_logsigexit, 0,
            "Log processes quitting on abnormal signals to syslog(3)");

/*
 * Signal default actions.
 */
static const uint8_t default_sigproptbl[] = {
    SA_IGNORE,          /*!< Not a signal */
    SA_KILL,            /*!< SIGHUP */
    SA_KILL,            /*!< SIGINT */
    SA_KILL|SA_CORE,    /*!< SIGQUIT */
    SA_KILL|SA_CORE,    /*!< SIGILL */
    SA_KILL|SA_CORE,    /*!< SIGTRAP */
    SA_KILL|SA_CORE,    /*!< SIGABRT */
    SA_IGNORE,          /*!< SIGCHLD */
    SA_KILL|SA_CORE,    /*!< SIGFPE */
    SA_KILL,            /*!< SIGKILL */
    SA_KILL|SA_CORE,    /*!< SIGBUS */
    SA_KILL|SA_CORE,    /*!< SIGSEGV */
    SA_IGNORE|SA_CONT,  /*!< SIGCONT */
    SA_KILL,            /*!< SIGPIPE */
    SA_KILL,            /*!< SIGALRM */
    SA_KILL,            /*!< SIGTERM */
    SA_STOP,            /*!< SIGSTOP */
    SA_STOP|SA_TTYSTOP, /*!< SIGTSTP */
    SA_STOP|SA_TTYSTOP, /*!< SIGTTIN */
    SA_STOP|SA_TTYSTOP, /*!< SIGTTOU */
    SA_KILL,            /*!< SIGUSR1 */
    SA_KILL,            /*!< SIGUSR2 */
    SA_KILL|SA_CORE,    /*!< SIGSYS */
    SA_IGNORE,          /*!< SIGURG */
    SA_IGNORE,          /*!< SIGINFO */
    SA_KILL,            /*!< SIGPWR */
    SA_IGNORE,          /*!< SIGCHLDTHRD */
    SA_KILL,            /*!< SIGCANCEL */
    SA_IGNORE,          /*!< 28 */
    SA_IGNORE,          /*!< 29 */
    SA_IGNORE,          /*!< 30 */
    SA_IGNORE,          /*!< _SIGMTX */
};

static int ksignal_queue_sig(struct signals * sigs, int signum,
                             const struct ksignal_param * param);

RB_GENERATE(sigaction_tree, ksigaction, _entry, signum_comp);

/**
 * Signum comparator for rb trees.
 */
int signum_comp(struct ksigaction * a, struct ksigaction * b)
{
    KASSERT((a && b), "a & b must be set");

    return a->ks_signum - b->ks_signum;
}

#ifdef configLOCK_DEBUG
#define ksig_lock(lock) ksig_lock_(lock, _KERROR_WHERESTR)
static int ksig_lock_(ksigmtx_t * lock, char * whr)
#else
static int ksig_lock(ksigmtx_t * lock)
#endif
{
    istate_t s;
    int retval;

#if defined(configLOCK_DEBUG)
#define _ksig_lock_(mtx)    _mtx_lock(&mtx->l, whr)
#define _ksig_trylock_(mtx) _mtx_trylock(&mtx->l, whr)
#else
#define _ksig_lock_(mtx)    mtx_lock(&mtx->l)
#define _ksig_trylock_(mtx) mtx_trylock(&mtx->l)
#endif

    s = get_interrupt_state();
    if (s & PSR_INT_I) {
        retval = _ksig_trylock_(lock);
    } else {
        retval = _ksig_lock_(lock);
    }

#if defined(configLOCK_DEBUG)
    if (retval == 0) {
        lock->l.mtx_ldebug = whr;
    }
#endif


    return retval;
}

static void ksig_unlock(ksigmtx_t * lock)
{
    mtx_unlock(&lock->l);
}

static int ksig_testlock(ksigmtx_t * lock)
{
    return mtx_test(&lock->l);
}

#if defined(configKSIGNAL_DEBUG)
static char * ksignal_str_owner_type(struct signals * sigs)
{
    switch (sigs->s_owner_type) {
    case SIGNALS_OWNER_PROCESS:
        return "process";
    case SIGNALS_OWNER_THREAD:
        return "thread";
    default:
        return "unknown";
    }
}
#endif

const char * ksignal_signum2str(int signum)
{
    return _signal_signum2str(signum);
}

/**
 * Execute thread if signal conditions are met.
 */
static void ksignal_exec_cond(struct thread_info * thread, int signum)
{
    KASSERT(ksig_testlock(&thread->sigs.s_lock), "sigs should be locked\n");

    const int blocked = ksignal_isblocked(&thread->sigs, signum);
    const int swait = sigismember(&thread->sigs.s_wait, signum);

    if (thread_state_get(thread) == THREAD_STATE_INIT)
        return; /* Shouldn't wakeup a thread that is still in init state. */

    if (blocked && swait)
        thread_release(thread->id);
    else if (!blocked)
        thread_ready(thread->id);
}

static void ksignal_free(struct kobj * p)
{
    /* NOP at least for now */
}

void ksignal_signals_ctor(struct signals * sigs, enum signals_owner owner_type)
{
    KSIGNAL_PENDQUEUE_INIT(sigs);
    RB_INIT(&sigs->sa_tree);
    sigemptyset(&sigs->s_block);
    sigemptyset(&sigs->s_wait);
    sigemptyset(&sigs->s_running);
    mtx_init(&sigs->s_lock.l, KSIG_LOCK_TYPE, KSIG_LOCK_FLAGS);
    kobj_init(&sigs->s_obj, ksignal_free);
    sigs->s_owner_type = owner_type;
}

static void ksignal_thread_ctor(struct thread_info * th)
{
    ksignal_signals_ctor(&th->sigs, SIGNALS_OWNER_THREAD);
}
DATA_SET(thread_ctors, ksignal_thread_ctor);

void ksignal_signals_dtor(struct signals * sigs)
{
    kobj_unref(&sigs->s_obj);
}

static void ksignal_thread_dtor(struct thread_info * th)
{
    ksignal_signals_dtor(&th->sigs);
}
DATA_SET(thread_dtors, ksignal_thread_dtor);

void ksignal_signals_fork_reinit(struct signals * sigs)
{
    struct sigaction_tree old_tree = sigs->sa_tree;
    struct ksigaction * sigact_old;

    /*
     * Clear pending signals as required by POSIX.
     */
    KSIGNAL_PENDQUEUE_INIT(sigs);

    /*
     * Clone configured signal actions.
     */
    RB_INIT(&sigs->sa_tree);
    RB_FOREACH(sigact_old, sigaction_tree, &old_tree) {
        struct ksigaction * sigact_new = kmalloc(sizeof(struct ksigaction));

        KASSERT(sigact_new != NULL, "OOM during thread fork\n");

        memcpy(sigact_new, sigact_old, sizeof(struct ksigaction));
        RB_INSERT(sigaction_tree, &sigs->sa_tree, sigact_new);
    }

    /*
     * Reinit mutex lock.
     */
    mtx_init(&sigs->s_lock.l, KSIG_LOCK_TYPE, KSIG_LOCK_FLAGS);
}

static void ksignal_fork_handler(struct thread_info * th)
{
    ksignal_signals_fork_reinit(&th->sigs);
}
DATA_SET(thread_fork_handlers, ksignal_fork_handler);

/**
 * Forward signals pending in proc sigs struct to thread pendqueue.
 * It's usually not a good idea to call this function from other than the owning
 * process. The idea is that the cost of the actual forwarding and delivery
 * should be counted for the receiving side. Also it's lot more safer
 * to only call this function for curproc since then no locking or ref counting
 * is needed for the proc struct.
 */
static void forward_proc_signals(struct proc_info * proc)
{
    struct signals * proc_sigs = &proc->sigs;
    struct ksiginfo * ksiginfo;
    struct ksiginfo * tmp;

    KASSERT(ksig_testlock(&proc_sigs->s_lock), "sigs should be locked\n");

    /* Get next pending signal. */
    KSIGNAL_PENDQUEUE_FOREACH_SAFE(ksiginfo, proc_sigs, tmp) {
        struct thread_info * thread;
        struct thread_info * thread_it = NULL;
        const int signum = ksiginfo->siginfo.si_signo;

        while ((thread = proc_iterate_threads(proc, &thread_it))) {
            int blocked, swait;
            struct signals * thread_sigs = &thread->sigs;

            /*
             * Check if signal is not blocked for the thread.
             */
            if (ksig_lock(&thread_sigs->s_lock)) {
                /* RFE Could we just continue? */
                return; /* Try again later */
            }
            blocked = ksignal_isblocked(thread_sigs, signum);
            swait   = sigismember(&thread_sigs->s_wait, signum);

            if (!(blocked && swait) && blocked) {
                ksig_unlock(&thread_sigs->s_lock);
                continue; /* check next thread */
            }

            /*
             * The signal should be processed by thread.
             */
            KSIGNAL_PENDQUEUE_REMOVE(proc_sigs, ksiginfo);
            KSIGNAL_PENDQUEUE_INSERT_TAIL(thread_sigs, ksiginfo);
            if (thread != current_thread)
                ksignal_exec_cond(thread, ksiginfo->siginfo.si_signo);
            ksig_unlock(&thread_sigs->s_lock);

#if defined(configKSIGNAL_DEBUG)
            KERROR(KERROR_DEBUG, "Signal %s forwarded to thread %d\n",
                   ksignal_signum2str(signum), thread->id);
#endif
            /*
             * We probably can't break and continue signal forwarding here
             * because otherwise we may give one tread signals that can't
             * be handled rightaway (blocking) even there might be another
             * thread capable of handling those.
             */
            return;
        }
    }
}

static void forward_proc_signals_curproc(void)
{
    ksigmtx_t * s_lock = &curproc->sigs.s_lock;

    if (ksig_lock(s_lock))
        return;

    forward_proc_signals(curproc);

    ksig_unlock(s_lock);
}

/**
 * @return  0 = signal handling ready;
 *         -1 = signal can't be handled right now;
 *          1 = signal handling shall continue
 */
static int eval_inkernel_action(struct ksigaction * action)
{
    switch ((int)(action->ks_action.sa_handler)) {
    case (int)(SIG_DFL):
        /*
         * SA_KILL should be handled before queuing.
         */
        if (action->ks_action.sa_flags & SA_KILL) {
            KERROR(KERROR_ERR, "post_scheduling can't handle SA_KILL (yet)");
            return 0;
        }
        return 1;
    case (int)(SIG_IGN):
        return 0;
    case (int)(SIG_ERR):
        /* TODO eval SIG_ERR */
    case (int)(SIG_HOLD):
        return -1;
    default:
        return 1;
    }
}

/**
 * Push 'src' to thread stack.
 * @param thread    thread.
 * @param src       is a pointer to to data to be pushed.
 * @param size      is the size of data pointer by src.
 * @param old_thread_sp[in] returns the old thread stack pointer, can be NULL.
 * @return Error code or zero.
 */
static int thread_stack_push(struct thread_info * thread, const void * src,
                             size_t size, void ** old_thread_sp)
{
    sw_stack_frame_t * sframe;
    void * old_sp;
    void * new_sp;
    int err;

    KASSERT(size > 0, "size should be greater than zero.\n");

    sframe = get_usr_sframe(thread);
    if (!sframe)
        return -EINVAL;

    old_sp = (void *)sframe->sp;
    new_sp = (void *)((uintptr_t)old_sp - memalign(size));
    if (!old_sp)
        return -EFAULT;

    err = copyout(src, (__user void *)new_sp, size);
    if (err)
        return -EFAULT;

    sframe->sp = (uintptr_t)new_sp;
    if (old_thread_sp)
        *old_thread_sp = old_sp;

    return 0;
}

/**
 * Pop from thread stack to dest.
 */
static int thread_stack_pop(struct thread_info * thread, void * buf,
                            size_t size)
{
    sw_stack_frame_t * sframe;
    __user void * sp;
    int err;

    KASSERT(size > 0, "size should be greater than zero.\n");

    sframe = get_usr_sframe(thread);
    if (!sframe)
        return -EINVAL;

    sp = (__user void *)sframe->sp;
    if (!sp)
        return -EFAULT;

    err = copyin(sp, buf, size);
    if (err)
        return err;

    sframe->sp += memalign(size);

    return 0;
}

/**
 * Set the next stack frame properly for branching to a signal handler
 * defined by sigaction.
 * @param signum        is the signal number interrupting the normal execution.
 * @param action        describes the signal action and it's used to determine
 *                      the next pc value.
 */
static int push_stack_frame(int signum,
                            const struct ksigaction * restrict action,
                            const siginfo_t * siginfo)
{
    const uintptr_t usigret = curproc->usigret;
    sw_stack_frame_t * tsfp = get_usr_sframe(current_thread);
    void * old_thread_sp; /* this is used to revert signal handling state
                           * and return to normal execution. */

    if (/* Push current stack frame to the user space thread stack. */
        thread_stack_push(current_thread,
                          tsfp,
                          sizeof(sw_stack_frame_t),
                          NULL) ||
        /* Push siginfo struct. */
        thread_stack_push(current_thread,
                          siginfo,
                          sizeof(siginfo_t),
                          &old_thread_sp /* Address of the prev sframe. */)
       ) {
        KERROR(KERROR_ERR, "Failed to push signum %s\n",
               ksignal_signum2str(signum));

        return -EINVAL;
    }

    if (usigret < configEXEC_BASE_LIMIT) {
        KERROR(KERROR_WARN, "usigret addr probably invalid (%x) for proc %i\n",
               usigret, (int)curproc->pid);
    }

    tsfp->pc = (uintptr_t)action->ks_action.sa_sigaction;
    tsfp->r0 = signum;                      /* arg1 = signum */
    tsfp->r1 = tsfp->sp;                    /* arg2 = siginfo */
    tsfp->r2 = 0;                           /* arg3 = TODO context */
    tsfp->r9 = (uintptr_t)old_thread_sp;    /* old stack frame */
    tsfp->lr = usigret;

    return 0;
}

/**
 * Post thread scheduling handler that updates thread stack frame if a signal
 * is pending. After this handler the thread will enter to signal handler
 * instead of returning to normal execution.
 */
static void ksignal_post_scheduling(void)
{
    int signum;
    struct signals * sigs = &current_thread->sigs;
    struct ksigaction action;
    struct ksiginfo * ksiginfo;

    forward_proc_signals_curproc();

    /*
     * Can't handle signals right now if we can't get lock to sigs of
     * the current thread.
     * RFE Can this cause any unexpected returns?
     * TODO What if there is a fatal signal?
     */
    if (ksig_lock(&sigs->s_lock)) {
        return;
    }

    /*
     * Check if thread is in uninterruptible syscall.
     */
    if (thread_flags_is_set(current_thread, SCHED_INSYS_FLAG) &&
        !KSIGFLAG_IS_SET(sigs, KSIGFLAG_INTERRUPTIBLE)) {
        ksig_unlock(&sigs->s_lock);
        return;
    }

    /* Get next pending signal. */
    KSIGNAL_PENDQUEUE_FOREACH(ksiginfo, sigs) {
        int blocked, swait, nxt_state;

        signum = ksiginfo->siginfo.si_signo;
        blocked = ksignal_isblocked(sigs, signum);
        swait = sigismember(&sigs->s_wait, signum);

        /* Signal pending let's check if we should handle it now. */
        ksignal_get_ksigaction(&action, sigs, signum);
        if (sigismember(&sigs->s_running, signum)) {
            /* Already running a handler for that signum */
            sigdelset(&sigs->s_running, signum);
            continue;
        }

        /* Check if the thread is waiting for this signal */
        if (blocked && swait) {
            sigemptyset(&sigs->s_wait);
            current_thread->sigwait_retval = ksiginfo;
            KSIGNAL_PENDQUEUE_REMOVE(sigs, ksiginfo);
            KSIGFLAG_CLEAR(sigs, KSIGFLAG_INTERRUPTIBLE);
            ksig_unlock(&sigs->s_lock);
#if defined(configKSIGNAL_DEBUG)
            KERROR(KERROR_DEBUG, "Detected a sigwait() for %s, returning\n",
                   ksignal_signum2str(signum));
#endif
            return; /* There is a sigwait() for this signum. */
        }

        /* Check if signal is blocked */
        if (blocked) {
            /* This signal is currently blocked and can't be handled. */
            continue;
        }

        nxt_state = eval_inkernel_action(&action);
        if (nxt_state == 0 || action.ks_action.sa_flags & SA_IGNORE) {
            /* Signal handling done */
            KSIGNAL_PENDQUEUE_REMOVE(sigs, ksiginfo);
            KSIGFLAG_CLEAR(sigs, KSIGFLAG_INTERRUPTIBLE);
            ksig_unlock(&sigs->s_lock);
            kfree_lazy(ksiginfo);
#if defined(configKSIGNAL_DEBUG)
            KERROR(KERROR_DEBUG, "Signal %s handled in kernel space\n",
                   ksignal_signum2str(signum));
#endif
            return;
        } else if (nxt_state < 0) {
            /* This signal can't be handled right now */
#if defined(configKSIGNAL_DEBUG)
            KERROR(KERROR_DEBUG, "Postponing handling of signal %s\n",
                   ksignal_signum2str(signum));
#endif
            continue;
        }
        break;
    }
    if (!ksiginfo) {
        ksig_unlock(&sigs->s_lock);
        return; /* All signals blocked or no signals pending */
    }
    /*
     * Else the pending singal should be handled now but in user space, so
     * continue to handle the signal in user space handler.
     */
    KSIGNAL_PENDQUEUE_REMOVE(sigs, ksiginfo);

#if defined(configKSIGNAL_DEBUG)
    KERROR(KERROR_DEBUG, "Pass a signal %s to the user space\n",
           ksignal_signum2str(ksiginfo->siginfo.si_signo));
#endif

    /* Push data and set next stack frame. */
    if (push_stack_frame(signum, &action, &ksiginfo->siginfo)) {
        const struct ksignal_param sigparm = { .si_code = ILL_BADSTK };

        /*
         * Thread has trashed its stack; Nothing we can do but give SIGILL.
         * RFE Should we punish only the thread or the whole process?
         */
#if defined(configKSIGNAL_DEBUG)
         KERROR(KERROR_DEBUG,
                "Thread has trashed its stack, sending a fatal signal\n");
#endif
        ksig_unlock(&sigs->s_lock);
        kfree_lazy(ksiginfo);
        /* RFE Possible deadlock? */
        ksignal_sendsig_fatal(curproc, SIGILL, &sigparm);
        return; /* RFE Is this ok? */
    }

    /*
     * TODO
     * - Check current_thread sigs
     *  -- Change to alt stack if requested
     */

    KSIGFLAG_SET(sigs, KSIGFLAG_SIGHANDLER);
    KSIGFLAG_CLEAR(sigs, KSIGFLAG_INTERRUPTIBLE);
    ksig_unlock(&sigs->s_lock);
    kfree_lazy(ksiginfo);
}
DATA_SET(post_sched_tasks, ksignal_post_scheduling);

int ksignal_sendsig(struct signals * sigs, int signum,
                    const struct ksignal_param * param)
{
    int retval;

    if ((retval = kobj_ref(&sigs->s_obj)) || ksig_lock(&sigs->s_lock)) {
        if (!retval)
            kobj_unref(&sigs->s_obj);
        return -EAGAIN;
    }
    retval = ksignal_queue_sig(sigs, signum, param);
    ksig_unlock(&sigs->s_lock);
    kobj_unref(&sigs->s_obj);

    return retval;
}

static int ksignal_queue_sig(struct signals * sigs, int signum,
                             const struct ksignal_param * param)
{
    int retval = 0;
    struct ksigaction action;
    struct ksiginfo * ksiginfo;
    struct thread_info * thread;

    KASSERT(ksig_testlock(&sigs->s_lock), "sigs should be locked\n");

#if defined(configKSIGNAL_DEBUG)
    KERROR(KERROR_DEBUG, "Queuing a signum %s to sigs: %p (%s)\n",
           ksignal_signum2str(signum), sigs, ksignal_str_owner_type(sigs));
#endif

    if (signum <= 0 || signum > _SIG_MAXSIG) {
#if defined(configKSIGNAL_DEBUG)
        KERROR(KERROR_DEBUG, "Invalid signum\n");
#endif
        return -EINVAL;
    }

    retval = sigismember(&sigs->s_running, signum);
    if (retval) {
        /* Already running a handler. */
#if defined(configKSIGNAL_DEBUG)
        KERROR(KERROR_DEBUG, "\tAlready running a handler for this signal\n");
#endif
        return 0;
    }

    /* Get action struct for this signal. */
    ksignal_get_ksigaction(&action, sigs, signum);

    /* Ignored? */
    if (action.ks_action.sa_handler == SIG_IGN) {
#if defined(configKSIGNAL_DEBUG)
        KERROR(KERROR_DEBUG, "\tSignal ignored\n");
#endif
        return 0;
    }

    /*
     * Get the associated thread.
     */
    switch (sigs->s_owner_type) {
    case SIGNALS_OWNER_PROCESS:
        thread = container_of(sigs, struct proc_info, sigs)->main_thread;
        break;
    case SIGNALS_OWNER_THREAD:
        thread = container_of(sigs, struct thread_info, sigs);
        break;
    default:
        panic("Invalid sigs owner type");
    }
    KASSERT(thread != NULL, "thread must be set");

    /*
     * Build ksiginfo.
     */
    ksiginfo = kmalloc(sizeof(struct ksiginfo));
    if (!ksiginfo)
        return -ENOMEM;
    *ksiginfo = (struct ksiginfo){
        .siginfo.si_signo = signum,
        .siginfo.si_code = param->si_code,
        .siginfo.si_errno = param->si_errno,
        .siginfo.si_tid = current_thread->id,
        .siginfo.si_pid = curproc->pid,
        .siginfo.si_uid = curproc->cred.uid,
        .siginfo.si_addr = param->si_addr,
        .siginfo.si_status = param->si_status,
        .siginfo.si_value = param->si_value,
    };

    /*
     * SA_KILL is handled here because post_scheduling handler can't change
     * next thread.
     */
    if ((action.ks_action.sa_handler == SIG_DFL) &&
            (action.ks_action.sa_flags & SA_KILL) &&
            !sigismember(&sigs->s_wait, signum)) {
        struct proc_info * proc_owner;

#if defined(configKSIGNAL_DEBUG)
        KERROR(KERROR_DEBUG, "Thread %u will be terminated by signum %s\n",
               thread->id, ksignal_signum2str(signum));
#endif

        ksiginfo->siginfo.si_code = CLD_KILLED;
        thread->exit_ksiginfo = ksiginfo;

        /*
         * RFE Should we kill the process regardles of which thread it was?
         * RFE Should we block all threads?
         */
        proc_owner = proc_ref(thread->pid_owner, PROC_NOT_LOCKED);
        proc_unref(proc_owner); /* Won't be freed anyway. */
        if (proc_owner && (action.ks_action.sa_flags & SA_CORE) &&
            proc_owner->main_thread == thread) {
#if defined(configCORE_DUMPS)
            if (core_dump_by_curproc(proc_owner) == 0)
                ksiginfo->siginfo.si_code = CLD_DUMPED;
#endif
        }

        /*
         * If the thread is in a system call we should wait until it's exiting
         * to make sure we don't left any locks or extra refcounts.
         */
        if (thread_flags_is_set(current_thread, SCHED_INSYS_FLAG)) {
            if (sigs != &thread->sigs) {
                while (ksig_lock(&thread->sigs.s_lock));
                KSIGFLAG_SET(&thread->sigs, KSIGFLAG_SA_KILL);
                ksig_unlock(&thread->sigs.s_lock);
            } else {
                KSIGFLAG_SET(sigs, KSIGFLAG_SA_KILL);
            }
        } else {
            /*
             * Otherwise the thread is in user mode and we can just terminate
             * it immediately.
             */
            thread_terminate(thread->id);
        }
    } else { /* push to pending signals list. */
        if (action.ks_action.sa_flags & SA_RESTART) {
            KERROR(KERROR_ERR, "SA_RESTART is not yet supported\n");
        }

        /* Not ignored so we can set the signal to pending state. */
        KSIGNAL_PENDQUEUE_INSERT_TAIL(sigs, ksiginfo);

        if (thread != current_thread) {
            if (sigs != &thread->sigs) {
                while (ksig_lock(&thread->sigs.s_lock));
                ksignal_exec_cond(thread, signum);
                ksig_unlock(&thread->sigs.s_lock);
            } else { /* Sigs already locked */
                ksignal_exec_cond(thread, signum);
            }
        }
    }

    return 0;
}

void ksignal_sendsig_fatal(struct proc_info * p, int signum,
                           const struct ksignal_param * param)
{
    struct signals * sigs = &p->sigs;
    struct ksigaction act;
    int err;

    if (kobj_ref(&sigs->s_obj)) {
        KERROR(KERROR_ERR,
               "%s: Failed to send a fatal signal %s to pid: %d\n",
               __func__, ksignal_signum2str(signum), p->pid);
        return;
    }
    while (ksig_lock(&sigs->s_lock));

    /* Change signal action to default to make this signal fatal. */
    err = ksignal_reset_ksigaction(sigs, signum);
    if (err) {
        KERROR(KERROR_ERR,
               "%s: Failed to reset sigaction (pid: %d, signum: %s, err: %d)\n",
               __func__, p->pid, ksignal_signum2str(signum), err);
        goto out;
    }

    ksignal_get_ksigaction(&act, sigs, signum);
    if (!(act.ks_action.sa_flags & SA_KILL)) {
        KERROR(KERROR_WARN, "%d requested a fatal signal for %d"
                 "but dfl action for signum %s is not SA_KILL\n",
                 curproc->pid, p->pid, ksignal_signum2str(signum));
    }

    err = ksignal_queue_sig(sigs, signum, param);

    ksig_unlock(&sigs->s_lock);

    if (err) {
        KERROR(KERROR_ERR,
               "%s: Failed to send a fatal signal (pid: %d, signum: %s, err: %d)\n",
               __func__, p->pid, ksignal_signum2str(signum), err);
    }

out:
    kobj_unref(&sigs->s_obj);
}

int ksignal_sigwait(siginfo_t * retval, const sigset_t * restrict set)
{
    struct signals * sigs = &current_thread->sigs;
    ksigmtx_t * s_lock = &sigs->s_lock;
    struct ksiginfo * ksiginfo;

    KASSERT(retval, "retval must be set");

    while (ksig_lock(s_lock));
    memcpy(&sigs->s_wait, set, sizeof(sigs->s_wait));
    ksig_unlock(s_lock);

    forward_proc_signals_curproc();

    while (ksig_lock(s_lock));

    /* Iterate through pending signals */
    KSIGNAL_PENDQUEUE_FOREACH(ksiginfo, sigs) {
        if (sigismember(set, ksiginfo->siginfo.si_signo)) {
            current_thread->sigwait_retval = ksiginfo;
            KSIGNAL_PENDQUEUE_REMOVE(sigs, ksiginfo);
            ksig_unlock(s_lock);
            goto out;
        }
    }

    KSIGFLAG_SET(sigs, KSIGFLAG_INTERRUPTIBLE);
    ksig_unlock(s_lock);
    thread_wait(); /* Wait for a wakeup */
    KSIGFLAG_CLEAR(sigs, KSIGFLAG_INTERRUPTIBLE);

out:
    while (ksig_lock(s_lock));
    sigemptyset(&sigs->s_wait);
    /* TODO Sometimes sigwait_retval is not set? */
    if (current_thread->sigwait_retval)
        *retval = current_thread->sigwait_retval->siginfo;
    ksig_unlock(s_lock);
    kfree(current_thread->sigwait_retval);
    current_thread->sigwait_retval = NULL;

    return 0;
}

int ksignal_sigtimedwait(siginfo_t * retval, const sigset_t * restrict set,
                         const struct timespec * restrict timeout)
{
    int timer_id, err;
    siginfo_t sigret = { .si_signo = -1 };

    /*
     * TODO If timeout == 0 and there is no signals pending we should
     * immediately exit with an error.
     */

    timer_id = thread_alarm(timeout->tv_sec * 1000 +
                            timeout->tv_nsec / 1000000);
    if (timer_id < 0)
        return timer_id;

    err = ksignal_sigwait(&sigret, set);
    thread_alarm_rele(timer_id);

    if (err)
        return err;
    if (sigret.si_signo == -1)
        return -EAGAIN;
    *retval = sigret;
    return 0;
}

int ksignal_sigsleep(const struct timespec * restrict timeout)
{
    struct signals * sigs = &current_thread->sigs;
    ksigmtx_t * s_lock = &sigs->s_lock;
    struct ksiginfo * ksiginfo;
    int64_t usec, unslept;
    int timer_id;

    forward_proc_signals_curproc();

    while (ksig_lock(s_lock));

    /*
     * Iterate through pending signals and check if there is any actions
     * defined, possible thread termination is handled elsewhere.
     */
    KSIGNAL_PENDQUEUE_FOREACH(ksiginfo, sigs) {
        int signum = ksiginfo->siginfo.si_signo;

        if (!sigismember(&sigs->s_block, signum)) {
            struct ksigaction action;
            void (*sa_handler)(int);

            ksignal_get_ksigaction(&action, sigs, signum);
            sa_handler = action.ks_action.sa_handler;

            /*
             * _SIGMTX must be a special case here because it's not something
             * the user can have a control on and we may have one or more in
             * queue.
             * RFE Not sure if _SIGMTX requires some other special attention
             * still?
             */
            if (sa_handler != SIG_IGN && sa_handler != SIG_DFL &&
                    signum != _SIGMTX) {
                ksig_unlock(s_lock);
                return timeout->tv_sec;
            }
        }
    }

    usec = timeout->tv_sec * 1000000 + timeout->tv_nsec / 1000;
    timer_id = thread_alarm(usec / 1000);
    if (timer_id < 0)
        return timer_id;

    /* This syscall callable function is now interruptible */
    KSIGFLAG_SET(sigs, KSIGFLAG_INTERRUPTIBLE);
    ksig_unlock(s_lock);

    thread_wait();
    timers_stop(timer_id);
    unslept = usec - timers_get_split(timer_id);
    thread_alarm_rele(timer_id);

    unslept = (unslept > 0) ? unslept / 1000000 : 0;
    return unslept;
}

int ksignal_isblocked(struct signals * sigs, int signum)
{
    KASSERT(ksig_testlock(&sigs->s_lock), "sigs should be locked\n");

    /*
     * TODO IEEE Std 1003.1, 2004 Edition
     * When a signal is caught by a signal-catching function installed by
     * sigaction(), a new signal mask is calculated and installed for
     * the duration of the signal-catching function (or until a call to either
     * sigprocmask() or sigsuspend() is made). This mask is formed by taking
     * the union of the current signal mask and the value of the sa_mask for
     * the signal being delivered [XSI] unless SA_NODEFER or SA_RESETHAND
     * is set, and then including the signal being delivered. If and when
     * the user's signal handler returns normally, the original signal
     * mask is restored.
     */
    if (sigismember(&sigs->s_block, signum))
        return 1;
    return 0;
}

int ksignal_sigsmask(struct signals * sigs, int how,
                     const sigset_t * restrict set,
                     sigset_t * restrict oldset)
{
    sigset_t tmpset;
    sigset_t * cursigset;
    int err, retval = 0;

    if ((err = kobj_ref(&sigs->s_obj)) || ksig_lock(&sigs->s_lock)) {
        if (!err)
            kobj_unref(&sigs->s_obj);
        return -EAGAIN;
    }

    cursigset = &sigs->s_block;

    if (oldset)
        memcpy(oldset, cursigset, sizeof(sigset_t));

    if (!set)
        goto out;

    /* Change ops. */
    switch (how) {
    case SIG_BLOCK:
        /*
         * The resulting set is the union of the current set and the signal set
         * pointed by 'set'
         */
        sigunion(cursigset, cursigset, set);
        break;
    case SIG_SETMASK:
        /*
         * The resulting set is the signal set pointed by 'set'.
         */
        memcpy(cursigset, set, sizeof(sigset_t));
        break;
    case SIG_UNBLOCK:
        /*
         * The resulting set is the intersection of the current set and
         * the complement of the signal set pointed by 'set'.
         */
        memcpy(&tmpset, set, sizeof(sigset_t));
        sigintersect(cursigset, cursigset, sigcompl(&tmpset, &tmpset));
        break;
    default:
        /*
         * Invalid 'how' value.
         */
        retval = -EINVAL;
    }

out:
    ksig_unlock(&sigs->s_lock);
    kobj_unref(&sigs->s_obj);

    return 0;
}

void ksignal_get_ksigaction(struct ksigaction * action,
                            struct signals * sigs, int signum)
{
    struct ksigaction find = { .ks_signum = signum };
    struct ksigaction * p_action;

    KASSERT(action, "Action should be set\n");
    KASSERT(signum >= 0, "Signum should be positive\n");
    KASSERT(ksig_testlock(&sigs->s_lock), "sigs should be locked\n");

    if (!RB_EMPTY(&sigs->sa_tree) &&
        (p_action = RB_FIND(sigaction_tree, &sigs->sa_tree, &find))) {
        memcpy(action, p_action, sizeof(struct ksigaction));
        return;
    }

    action->ks_signum = signum;
    sigemptyset(&action->ks_action.sa_mask);
    action->ks_action.sa_flags = (signum < (int)num_elem(default_sigproptbl)) ?
        default_sigproptbl[signum] : SA_IGNORE;
    action->ks_action.sa_handler = SIG_DFL;
}

int ksignal_reset_ksigaction(struct signals * sigs, int signum)
{
    struct ksigaction filt = { .ks_signum = signum };
    struct ksigaction * p_action;

    if (signum < 0 || signum >= (int)num_elem(default_sigproptbl)) {
        return -EINVAL;
    }

    KASSERT(ksig_testlock(&sigs->s_lock), "sigs should be locked\n");

    if (!RB_EMPTY(&sigs->sa_tree) &&
        (p_action = RB_FIND(sigaction_tree, &sigs->sa_tree, &filt))) {
        if (RB_REMOVE(sigaction_tree, &sigs->sa_tree, p_action)) {
            kfree(p_action);
        } else {
            panic("Can't remove an entry from sigaction_tree\n");
        }
    }

    return 0;
}

/**
 * Set signal action struct.
 * @note Always copied, so action struct can be allocated from stack.
 */
int ksignal_set_ksigaction(struct signals * sigs, struct ksigaction * action)
{
    int signum;
    struct ksigaction * p_action;
    struct sigaction * sigact = NULL;

    KASSERT(ksig_testlock(&sigs->s_lock), "sigs should be locked\n");

    if (!action)
        return -EINVAL;
    signum = action->ks_signum;

    if (!(action->ks_signum > 0 && action->ks_signum < _SIG_MAX_))
        return -EINVAL;

    if (!RB_EMPTY(&sigs->sa_tree) &&
        (p_action = RB_FIND(sigaction_tree, &sigs->sa_tree, action))) {

        memcpy(p_action, action, sizeof(struct ksigaction) -
                sizeof(RB_ENTRY(ksigaction)));
    } else {
        p_action = kmalloc(sizeof(struct ksigaction));
        memcpy(p_action, action, sizeof(struct ksigaction) -
                sizeof(RB_ENTRY(ksigaction)));

        if (RB_INSERT(sigaction_tree, &sigs->sa_tree, p_action)) {
            panic("ksignal_set_ksigaction() failed to insert.\n");
        }
    }

    /* Check if this action can be actually removed */
    sigact = &p_action->ks_action;
    if (sigisemptyset(&sigact->sa_mask) &&
        (sigact->sa_flags == (signum < (int)sizeof(default_sigproptbl)) ?
            default_sigproptbl[signum] : SA_IGNORE) &&
        (sigact->sa_handler == SIG_DFL)) {
        if (RB_REMOVE(sigaction_tree, &sigs->sa_tree, p_action)) {
            kfree(p_action);
        } else {
            panic("Can't remove an entry from sigaction_tree\n");
        }
    }

    return 0;
}

int ksignal_syscall_exit(int retval)
{
    struct signals * sigs = &current_thread->sigs;

    while (ksig_lock(&sigs->s_lock));

    KSIGFLAG_CLEAR(sigs, KSIGFLAG_INTERRUPTIBLE);

    if (KSIGFLAG_IS_SET(sigs, KSIGFLAG_SA_KILL)) {
        ksig_unlock(&sigs->s_lock);
        thread_die(current_thread->retval);
        /* Won't return */
    }

    if (KSIGFLAG_IS_SET(sigs, KSIGFLAG_SIGHANDLER)) {
        /*
         * The syscall was interrupted by a signal that will cause a
         * branch to a signal handler before returning to the caller.
         */
        sw_stack_frame_t * sframe = get_usr_sframe(current_thread);
        sw_stack_frame_t caller;

        KASSERT(sframe != NULL, "Must have exitting sframe");

        /* Set return value for the syscall. */
        copyin((__user sw_stack_frame_t *)sframe->r9, &caller, sizeof(caller));
        caller.r0 = retval;
        copyout(&caller, (__user sw_stack_frame_t *)sframe->r9, sizeof(caller));

        /* Set first argument for the signal handler. */
        retval = sframe->r0;

        KSIGFLAG_CLEAR(sigs, KSIGFLAG_SIGHANDLER);
    }

    ksig_unlock(&sigs->s_lock);
    return retval;
}

static int is_valid_usignum(int signum)
{
    return (0 < signum && signum < _SIG_UMAX_);
}

/* System calls ***************************************************************/

/**
 * Send a signal to a process or a group of processes.
 */
static int sys_signal_pkill(__user void * user_args)
{
    struct _pkill_args args;
    struct proc_info * proc;
    struct signals * sigs;
    int err, retval = -1;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    /* TODO if pid == 0 send signal to all procs */

    proc = proc_ref(args.pid, PROC_NOT_LOCKED);
    if (!proc) {
        set_errno(ESRCH);
        return -1;
    }

    /*
     * Check if process is privileged to signal other users.
     */
    if (priv_check_cred(&curproc->cred, &proc->cred, PRIV_SIGNAL_OTHER)) {
        set_errno(EPERM);
        retval = -1;
        goto out;
    }

    /*
     * The null signal can be used to check the validity of pid.
     * IEEE Std 1003.1, 2013 Edition
     *
     * If sig == 0 we can return immediately.
     */
    if (args.sig == 0) {
        retval = 0;
        goto out;
    }

    if (!is_valid_usignum(args.sig)) {
        set_errno(EINVAL);
        goto out;
    }

    sigs = &proc->sigs;
    if ((err = kobj_ref(&sigs->s_obj)) || ksig_lock(&sigs->s_lock)) {
        if (!err)
            kobj_unref(&sigs->s_obj);
        set_errno(EAGAIN);
        goto out;
    }

    /* RFE Check errors? */
    ksignal_queue_sig(sigs, args.sig,
                      &(struct ksignal_param){ .si_code = SI_USER });

    ksig_unlock(&sigs->s_lock);
    kobj_unref(&sigs->s_obj);

    /*
     * It's a good idea to forward signals now if we sent a signal to ourself.
     */
    if (args.pid == curproc->pid) {
        forward_proc_signals_curproc();
    }

    retval = 0;
out:
    proc_unref(proc);
    return retval;
}

/**
 * Send a signal to a thread or threads.
 */
static int sys_signal_tkill(__user void * user_args)
{
    struct _tkill_args args;
    struct thread_info * thread;
    struct proc_info * proc;
    struct signals * sigs;
    int err, retval = -1;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    /* TODO if thread_id == 0 then send to all (child/group?) threads */

    thread = thread_lookup(args.thread_id);
    if (!thread) {
        set_errno(ESRCH);
        return -1;
    }

    proc = proc_ref(thread->pid_owner, PROC_NOT_LOCKED);
    if (!proc) {
        set_errno(ESRCH);
        return -1;
    }

    /*
     * Check if process is privileged to signal other users.
     */
    if (priv_check_cred(&curproc->cred, &proc->cred, PRIV_SIGNAL_OTHER)) {
        set_errno(EPERM);
        goto out;
    }

    /*
     * The null signal can be used to check the validity of pid. (thread id)
     * IEEE Std 1003.1, 2013 Edition
     *
     * If sig == 0 we can return immediately.
     */
    if (args.sig == 0) {
        retval = 0;
        goto out;
    }

    if (!is_valid_usignum(args.sig)) {
        set_errno(EINVAL);
        goto out;
    }

    sigs = &thread->sigs;
    if ((err = kobj_ref(&sigs->s_obj)) || ksig_lock(&sigs->s_lock)) {
        if (!err)
            kobj_unref(&sigs->s_obj);
        set_errno(EAGAIN);
        goto out;
    }

    err = ksignal_queue_sig(sigs, args.sig,
                            &(struct ksignal_param){ .si_code = SI_USER });
    ksig_unlock(&sigs->s_lock);
    kobj_unref(&sigs->s_obj);
    if (err) {
        set_errno(-err);
        goto out;
    }

    retval = 0;
out:
    proc_unref(proc);
    return retval;
}

static int sys_signal_signal(__user void * user_args)
{
    struct _signal_signal_args args;
    struct ksigaction action;
    void * old_handler;
    struct signals * sigs;
    int err;

    if (priv_check(&curproc->cred, PRIV_SIGNAL_ACTION)) {
        set_errno(ENOTSUP);
        return -1;
    }

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if (!is_valid_usignum(args.signum)) {
        set_errno(EINVAL);
        return -1;
    }

    /*
     * Since signal() is not clearly defined to work for multi-threaded
     * processes, we just use sigs struct of the current_thread and hope
     * that's what the caller wanted to alter.
     */
    sigs = &current_thread->sigs;
    if (ksig_lock(&sigs->s_lock)) {
        set_errno(EAGAIN);
        return -1;
    }

    /* Get current sigaction */
    ksignal_get_ksigaction(&action, sigs, args.signum);

    /* Swap handler pointers */
    old_handler = action.ks_action.sa_handler;
    action.ks_action.sa_handler = args.handler;
    args.handler = old_handler;

    /* Set new handler and unlock sigs */
    err = ksignal_set_ksigaction(sigs, &action);
    ksig_unlock(&sigs->s_lock);
    if (err) {
        set_errno(-err);
        return -1;
    }

    err = copyout(&args, user_args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_signal_action(__user void * user_args)
{
    struct _signal_action_args args;
    struct signals * sigs;
    struct ksigaction old_ksigaction;
    int err;

    /*
     * RFE How should a process get a priviledge to set signal handlers needed
     * by libc?
     */
    if (priv_check(&curproc->cred, PRIV_SIGNAL_ACTION)) {
        set_errno(ENOTSUP);
        return -1;
    }

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    if (!is_valid_usignum(args.signum)) {
        set_errno(EINVAL);
        return -1;
    }

    sigs = &current_thread->sigs;
    if (ksig_lock(&sigs->s_lock)) {
        set_errno(EAGAIN);
        return -1;
    }
    ksignal_get_ksigaction(&old_ksigaction, sigs, args.signum);
    memcpy(&args.old_action, &old_ksigaction.ks_action,
           sizeof(args.old_action));
    err = ksignal_set_ksigaction(
            sigs,
            &(struct ksigaction){
                .ks_signum = args.signum,
                .ks_action = args.new_action
            });
    ksig_unlock(&sigs->s_lock);
    if (err) {
        set_errno(-err);
        return -1;
    }

    err = copyout(&args, user_args, sizeof(args));
    if (err) {
        set_errno(EFAULT);
        return -1;
    }

    return 0;
}

static int sys_signal_altstack(__user void * user_args)
{
    /*
     * TODO Implement altstack syscall that can be used to set alternative
     *      user stack for signal handlers.
     */
    set_errno(ENOTSUP);
    return -1;
}

/**
 * Examine and change blocked signals of the thread or the current process.
 */
static int sys_signal_sigmask(__user void * user_args)
{
    struct _signal_sigmask_args args;
    sigset_t set;
    sigset_t * setp = NULL;
    sigset_t oldset;
    struct signals * sigs;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(-err);
        return -1;
    }

    if (args.set) {
        err = copyin((__user sigset_t *)args.set, &set, sizeof(sigset_t));
        if (err) {
            set_errno(-err);
            return -1;
        }
        setp = &set;
    }

    /*
     * Select current sigs.
     */
    if (args.threadmask)
        sigs = &current_thread->sigs;
    else
        sigs = &curproc->sigs;

    err = ksignal_sigsmask(sigs, args.how, setp, &oldset);
    if (err) {
        set_errno(-err);
        return -1;
    }

    if (args.oset) {
        /* Copy the current set to usr oset. */
        err = copyout(&oldset, (__user void *)args.oset,
                      sizeof(struct _signal_sigmask_args));
        if (err) {
            set_errno(-err);
            return -1;
        }
    }

    return 0;
}

static int sys_signal_sigwait(__user void * user_args)
{
    struct _signal_sigwait_args args;
    sigset_t set;
    siginfo_t retval;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(-err);
        return -1;
    }
    err = copyin((__user sigset_t *)args.set, &set, sizeof(set));
    if (err) {
        set_errno(-err);
        return -1;
    }

    err = ksignal_sigwait(&retval, &set);
    if (err) {
        set_errno(-err);
        return -1;
    }

    err = copyout(&retval.si_signo, (__user int *)args.sig, sizeof(int));
    if (err) {
        set_errno(EINVAL);
        return -1;
    }

    return 0;
}

static int sys_signal_sigwaitinfo(__user void * user_args)
{
    struct _signal_sigwaitinfo_args args;
    sigset_t set;
    siginfo_t retval;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(-err);
        return -1;
    }
    err = copyin((__user sigset_t *)args.set, &set, sizeof(set));
    if (err) {
        set_errno(-err);
        return -1;
    }

    if (args.twsec == -1) { /* sigwaitinfo */
        err = ksignal_sigwait(&retval, &set);
    } else { /* sigtimedwait */
        struct timespec timeout = {
            .tv_sec = args.twsec,
            .tv_nsec = args.twnsec,
        };

        err = ksignal_sigtimedwait(&retval, &set, &timeout);
    }
    if (err) {
        set_errno(-err);
        return -1;
    }

    err = copyout(&retval, (__user siginfo_t *)args.info, sizeof(retval));
    if (err) {
        set_errno(EINVAL);
        return -1;
    }

    return 0;
}

static int sys_signal_sigsleep(__user void * user_args)
{
    struct _signal_sigsleep_args args;
    struct timespec timeout;
    int err;

    err = copyin(user_args, &args, sizeof(args));
    if (err) {
        set_errno(-err);
        return -1;
    }

    timeout = (struct timespec){
        .tv_sec = args.tsec,
        .tv_nsec = args.tnsec,
    };

    return ksignal_sigsleep(&timeout);
}

static int sys_signal_set_return(__user void * user_args)
{
    curproc->usigret = (uintptr_t)user_args;

    return 0;
}

static int sys_signal_return(__user void * user_args)
{
    /* FIXME HW dependent. */
    sw_stack_frame_t * sframe = &current_thread->sframe.s[SCHED_SFRAME_SVC];
    sw_stack_frame_t next;
    uintptr_t sp;
    int err;

    /*
     * TODO
     * Return from signal handler
     * - revert stack frame and alt stack
     */

    sframe->sp = sframe->r9;
    err = thread_stack_pop(current_thread,
                           &next,
                           sizeof(const sw_stack_frame_t));
    if (err) {
        const struct ksignal_param sigparm = { .si_code = ILL_BADSTK };

        /*
         * RFE Should we punish only the thread or whole process?
         */
        ksignal_sendsig_fatal(curproc, SIGILL, &sigparm);
        while (1) {
            thread_wait();
            /* Should not return to here */
        }
    }
    sp = sframe->sp;
    memcpy(sframe, &next, sizeof(const sw_stack_frame_t));
    sframe->sp = sp;

    /*
     * We return for now but the actual return from this system call will
     * happen to the place that was originally interrupted by a signal.
     */
    return sframe->r0;
}

static const syscall_handler_t ksignal_sysfnmap[] = {
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_PKILL,      sys_signal_pkill),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_TKILL,      sys_signal_tkill),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_SIGNAL,     sys_signal_signal),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_ACTION,     sys_signal_action),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_ALTSTACK,   sys_signal_altstack),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_SIGMASK,    sys_signal_sigmask),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_SIGWAIT,    sys_signal_sigwait),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_SIGWAITNFO, sys_signal_sigwaitinfo),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_SIGSLEEP,   sys_signal_sigsleep),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_SETRETURN,  sys_signal_set_return),
    ARRDECL_SYSCALL_HNDL(SYSCALL_SIGNAL_RETURN,     sys_signal_return),
};
SYSCALL_HANDLERDEF(ksignal_syscall, ksignal_sysfnmap);
