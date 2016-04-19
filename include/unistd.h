/**
 *******************************************************************************
 * @file    unistd.h
 * @author  Olli Vanhoja
 * @brief   Standard symbolic constants and types.
 * @section LICENSE
 * Copyright (c) 2013 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/**
 * @addtogroup LIBC
 * @{
 */

/**
 * @addtogroup unistd
 * @{
 */

#ifndef _UNISTD_H
#define _UNISTD_H

#include <stdint.h> /* We only need intptr_t but that's not possible */
#include <sys/types/_gid_t.h>
#include <sys/types/_null.h>
#include <sys/types/_off_t.h>
#include <sys/types/_pid_t.h>
#include <sys/types/_size_t.h>
#include <sys/types/_ssize_t.h>
#include <sys/types/_uid_t.h>

/* Options and option groups */
#define _POSIX_ADVISORY_INFO            -1
#define _POSIX_MAPPED_FILES             200809L
#define _POSIX_PRIORITY_SCHEDULING      200809L
#define _POSIX_SHELL                    1
#define _POSIX_THREAD_SAFE_FUNCTIONS    200809L
#define _POSIX_SPORADIC_SERVER          -1
#define _POSIX_THREAD_ATTR_STACKADDR    200809L
#define _POSIX_THREAD_ATTR_STACKSIZE    200809L
#define _POSIX_THREADS                  200809L
#define _POSIX2_C_BIND                  200809L
#define _POSIX2_C_DEV                   -1

/*
 * Access test flags.
 * For compatibility with oflags these are defined to have same values so they
 * are interchangeable, except F_OK which obviously doesn't exist for oflags.
 */
#define F_OK 0x0001 /*!< Test for existence of file. */
#define R_OK 0x1000 /*!< Test for read permission. */
#define W_OK 0x2000 /*!< Test for write permission. */
#define X_OK 0x4000 /*!< Test for execute (search) permission. */

/* Block device seek origin types */
#ifndef SEEK_SET
#define SEEK_SET        0 /*!< Beginning of file. */
#define SEEK_CUR        1 /*!< Current position */
#define SEEK_END        2 /*!< End of file. */
#endif

#define STDIN_FILENO    0 /*!< File number of stdin. */
#define STDOUT_FILENO   1 /*!< File number of stdout. */
#define STDERR_FILENO   2 /*!< File number of stderr. */

/* Sysconf variables */
#define _SC_AIO_LISTIO_MAX               1
#define _SC_AIO_MAX                      2
#define _SC_AIO_PRIO_DELTA_MAX           3
#define _SC_ARG_MAX                      4
#define _SC_ATEXIT_MAX                   5
#define _SC_BC_BASE_MAX                  6
#define _SC_BC_DIM_MAX                   7
#define _SC_BC_SCALE_MAX                 8
#define _SC_BC_STRING_MAX                9
#define _SC_CHILD_MAX                   10
#define _SC_CLK_TCK                     11
#define _SC_COLL_WEIGHTS_MAX            12
#define _SC_DELAYTIMER_MAX              13
#define _SC_EXPR_NEST_MAX               14
#define _SC_HOST_NAME_MAX               15
#define _SC_IOV_MAX                     16
#define _SC_LINE_MAX                    17
#define _SC_LOGIN_NAME_MAX              18
#define _SC_NGROUPS_MAX                 19
#define _SC_GETGR_R_SIZE_MAX            20
#define _SC_GETPW_R_SIZE_MAX            21
#define _SC_MQ_OPEN_MAX                 22
#define _SC_MQ_PRIO_MAX                 23
#define _SC_OPEN_MAX                    24
#define _SC_ADVISORY_INFO               25
#define _SC_BARRIERS                    26
#define _SC_ASYNCHRONOUS_IO             27
#define _SC_CLOCK_SELECTION             28
#define _SC_CPUTIME                     29
#define _SC_FSYNC                       30
#define _SC_IPV6                        31
#define _SC_JOB_CONTROL                 32
#define _SC_MAPPED_FILES                33
#define _SC_MEMLOCK                     34
#define _SC_MEMLOCK_RANGE               35
#define _SC_MEMORY_PROTECTION           36
#define _SC_MESSAGE_PASSING             37
#define _SC_MONOTONIC_CLOCK             38
#define _SC_PRIORITIZED_IO              39
#define _SC_PRIORITY_SCHEDULING         40
#define _SC_RAW_SOCKETS                 41
#define _SC_READER_WRITER_LOCKS         42
#define _SC_REALTIME_SIGNALS            43
#define _SC_REGEXP                      44
#define _SC_SAVED_IDS                   45
#define _SC_SEMAPHORES                  46
#define _SC_SHARED_MEMORY_OBJECTS       47
#define _SC_SHELL                       48
#define _SC_SPAWN                       49
#define _SC_SPIN_LOCKS                  50
#define _SC_SPORADIC_SERVER             51
#define _SC_SS_REPL_MAX                 52
#define _SC_SYNCHRONIZED_IO             53
#define _SC_THREAD_ATTR_STACKADDR       54
#define _SC_THREAD_ATTR_STACKSIZE       55
#define _SC_THREAD_CPUTIME              56
#define _SC_THREAD_PRIO_INHERIT         57
#define _SC_THREAD_PRIO_PROTECT         58
#define _SC_THREAD_PRIORITY_SCHEDULING  59
#define _SC_THREAD_PROCESS_SHARED       60
#define _SC_THREAD_ROBUST_PRIO_INHERIT  61
#define _SC_THREAD_ROBUST_PRIO_PROTECT  62
#define _SC_THREAD_SAFE_FUNCTIONS       63
#define _SC_THREAD_SPORADIC_SERVER      64
#define _SC_THREADS                     65
#define _SC_TIMEOUTS                    66
#define _SC_TIMERS                      67
#define _SC_TRACE                       68
#define _SC_TRACE_EVENT_FILTER          69
#define _SC_TRACE_EVENT_NAME_MAX        70
#define _SC_TRACE_INHERIT               71
#define _SC_TRACE_LOG                   72
#define _SC_TRACE_NAME_MAX              73
#define _SC_TRACE_SYS_MAX               74
#define _SC_TRACE_USER_EVENT_MAX        75
#define _SC_TYPED_MEMORY_OBJECTS        76
#define _SC_VERSION                     77
#define _SC_V7_ILP32_OFF32              78
#define _SC_V7_ILP32_OFFBIG             79
#define _SC_V7_LP64_OFF64               80
#define _SC_V7_LPBIG_OFFBIG             81
#define _SC_PAGE_SIZE                   82
#define _SC_PAGESIZE                    83
/* End of sysconf variables */

#if defined(__SYSCALL_DEFS__) || defined(KERNEL_INTERNAL)
/**
 * Arguments struct for SYSCALL_FS_READ and SYSCALL_FS_WRITE
 */
struct _fs_readwrite_args {
    int fildes;
    void * buf;
    size_t nbytes;
};

/** Arguments struct for SYSCALL_FS_LSEEK */
struct _fs_lseek_args {
    int fd;
    off_t offset; /* input and return value */
    int whence;
};

/** Arguments for SYSCALL_FS_ACCESS */
struct _fs_access_args {
    int fd;
    const char * path;
    size_t path_len;
    int amode;
    int flag;
};

/** Arguments for SYSCALL_FS_CHOWN */
struct _fs_chown_args {
    int fd;
    uid_t owner;
    gid_t group;
};

/** Arguments for SYSCALL_FS_LINK */
struct _fs_link_args {
    int fd1;
    const char * path1;
    size_t path1_len;
    int fd2;
    const char * path2;
    size_t path2_len;
    int flag;
};

/** Arguments for SYSCALL_FS_UNLINK */
struct _fs_unlink_args {
    int fd;             /*!< File descriptor number. */
    const char * path;
    size_t path_len;
    int flag;
};

/** Arguments for SYSCALL_PROC_CHDIR */
struct _proc_chdir_args {
    int fd; /* if AT_FDARG */
    const char * name;
    size_t name_len; /*!< in bytes */
    int atflags;
};

/** Arguments for SYSCALL_PROC_SETPGID */
struct _proc_setpgid_args {
    pid_t pid;
    pid_t pg_id;
};

/** Arguments for SYSCALL_PROC_GETGROUPS */
struct _proc_getgroups_args {
    gid_t * grouplist;
    size_t size; /*!< Size of grouplist in bytes. */
};

/** Arguments for SYSCALL_PROC_SETGROUPS */
struct _proc_setgroups_args {
    const gid_t * grouplist;
    size_t size; /*!< Size of grouplist in bytes. */
};

/** Arguments for SYSCALL_IPC_PIPE */
struct _ipc_pipe_args {
    int fildes[2]; /*!< returned file descriptors. */
    size_t len; /*!< Preferred minumum size of the pipe buf. */
};

/** Arguments for SYSCALL_EXEC_EXEC */
struct _exec_args {
    int fd;
    char * const * argv;
    size_t nargv;
    char * const * env;
    size_t nenv;
};

/**
 * Arguments struct for SYSCALL_PROC_GETBREAK
 */
struct _proc_getbreak_args {
    void * start;
    void * stop;
};
#endif


#ifndef KERNEL_INTERNAL
__BEGIN_DECLS
/**
 * Set the break value.
 * @param addr is the new break addr.
 * @return  Returns 0 on succesful completion;
 *          Returns -1 and sets errno on error.
 * @throws  ENOMEM = The requested change would be impossible,
 *          EAGAIN = Temporary failure.
 */
int brk(void * addr);

/**
 * Increment or decrement the break value.
 * @param incr is the amount of allocated space in bytes. Negative value
 * decrements the break value.
 * @return  Returns the prior break value on succesful operation;
 *          Returns (void *)-1 if failed.
 * @throws  ENOMEM = The requested change would be impossible,
 *          EAGAIN = Temporary failure.
 */
void * sbrk(intptr_t incr);

/**
 * Get configurable system variables.
 * @note This is a user space wrapper to sysctl interface and provided
 * to comply with POSIX standard. More complete interface is avaiable by using
 * sysctl.
 */
long sysconf(int name);

/**
 * @addtogroup crypt
 * Trapdoor encryption.
 * @{
 */

char * crypt(const char *key, const char * salt);
const char * crypt_get_format(void);
int crypt_set_format(const char * string);

/**
 * @}
 */

/**
 * Change working directory.
 */
int chdir(const char * path);

pid_t fork(void);

/*
 * Note only exevp() and axeclp() can properly handle scripts with and without
 * shebang. This is also a requirement set by POSIX.
 */

extern char **environ;
int execl(const char *path, const char *arg0, ... /*, (char *)0 */);
int execle(const char *path, const char *arg0, ...
           /*,(char *)0, char *const envp[]*/);
int execlp(const char *file, const char *arg0, ... /*, (char *)0 */);
int execv(const char * path, char * const argv[]);
int execve(const char * path, char * const argv[], char * const envp[]);
int execvp(const char * name, char * const argv[]);
#if 0
int fexecve(int fd, char *const argv[], char *const envp[]);
#endif

/**
 * Terminate the process.
 */
void _exit(int status);

uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
int setuid(uid_t uid);
int seteuid(uid_t uid);
int setgid(gid_t gid);
int setegid(gid_t gid);
#if 0
int setreuid(uid_t, uid_t);
int setregid(gid_t, gid_t);
#endif

/**
 * @addtogroup getgroups getgroups setgroups
 * @{
 */

int getgroups(int gidsetsize, gid_t grouplist[]);
int setgroups(int gidsetsize, const gid_t grouplist[]);

/**
 * @}
 */

pid_t getsid(pid_t pid);
pid_t setsid(void);
pid_t getpgrp(void);
int set_pggid(pid_t pid, pid_t pgid);

/**
 * @addtogroup getlogin getlogin setlogin
 * @{
 */

/**
 * Get the login name associated with the current session.
 */
char * getlogin(void);
int getlogin_r(char * name, size_t namesize);

/**
 * Set the login name associated with the current session.
 * This call is restricted to the super-use.
 */
int setlogin(const char * name);

/**
 * @}
 */

/**
 * Get the Process ID.
 * @return Returns the Process ID of the current process.
 */
pid_t getpid(void);

/**
 * Get the parent Process ID.
 * @return Returns the Process ID of the parent process.
 */
pid_t getppid(void);

/**
 * @addtogroup access
 * Check accessibility of a file.
 *
 * The access() system call check the accessibility of the file named by
 * the path argument for access permissions indicated by the amode argument.
 * The value of amode is either the bitwise-inclusive OR of the access
 * permissions to be checked (R_OK for read permission, W_OK for write
 * permission, and X_OK for execute/search permission), or the
 * existence test (F_OK).
 *
 * The faccessat() system call is equivalent to access() but in case of relative
 * path is given the access is determined relatively to the directory associated
 * with file descriptor fd.
 * @{
 */

/**
 * Checks whether the process would be allowed to read, write or test for
 * existence of the file.
 * @param path  is a path to the file.
 * @param amode is the access mode tested.
 * @return 0 if file can be accessed; Otherwise -1 and errno.
 */
int access(const char * path, int amode);
int faccessat(int fd, const char * path, int amode, int flag);

/**
 * @}
 */

int chown(const char * path, uid_t owner, gid_t group);
int fchownat(int fd, const char * path, uid_t owner, gid_t group,
             int flag);
int fchown(int fildes, uid_t owner, gid_t group);

int lchown(const char *path, uid_t owner, gid_t group);

ssize_t pread(int fildes, void * buf, size_t nbytes, off_t offset);

/**
 * Read from a file descriptor.
 */
ssize_t read(int fildes, void * buf, size_t nbytes);

ssize_t pwrite(int fildes, const void *buf, size_t nbytes, off_t offset);

/**
 * Write to a file descriptor.
 */
ssize_t write(int fildes, const void * buf, size_t nbyte);

/**
 * Reposition read/write file offset.
 * The lseek function repositions the offset of the file descriptor fildes
 * to the argument offset according to the directive whence as follows:
 *
 * - SEEK_SET The offset is set to offset bytes.
 * - SEEK_CUR The offset is set to its current location plus offset bytes.
 * - SEEK_END The offset is set to the size of the file plus offset bytes.
 *
 * Some devices may or may not support seeking and there is no general agreement
 * on which devices should support seeking. Many unices will return the number
 * of characters written to a tty if SEEK_SET is used to it. Zeke follows this
 * convention.
 *
 * @param fildes is the file descriptor number.
 * @param offset is the offset from whence to seek.
 * @param whence is the directive specifying start location for seeking.
 * @returns A successful seek returns the resulting offset from the begining of
 *          the file; Otherwise (off_t)-1 and errno is returned.
 */
off_t lseek(int fildes, off_t offset, int whence);

/**
 * Close a file descriptor.
 * @param fildes is the file descriptor number.
 */
int close(int fildes);

/**
 * Create a new link to a existing file.
 */
int link(const char * path1, const char * path2);

/**
 * Create a new link relative to file descriptors.
 */
int linkat(int fd1, const char * path1,
           int fd2, const char * path2, int flag);

/**
 * Remove a directory entry.
 */
int unlink(const char * path);

/**
 * Remove a directory entry relative to directory file descriptor.
 */
int unlinkat(int fd, const char * path, int flag);

/**
 * Remove a directory.
 */
int rmdir(const char * path);

/**
 * Get the pathname of the current working directory.
 */
char * getcwd(char * pathname, size_t size);

/**
 * Return a new file descriptor that is the lowest number available.
 * The new file descriptor will refer to the same open file as the original file
 * descriptor.
 */
int dup(int fildes);
int dup2(int fildes, int fildes2);

int pipe(int fildes[2]);

unsigned sleep(unsigned seconds);

int gethostname(char * name, size_t namelen);
int sethostname(char * name, size_t namelen);

/**
 * @addtogroup getopt
 * Parse command-line options
 * @{
 */

extern char * optarg;

/**
 * if error message should be printed.
 */
extern int opterr;

/**
 * index into parent argv vector.
 */
extern int optind;

/**
 * character checked for validity.
 */
extern int optopt;

/**
 * reset getopt.
 */
extern int optreset;

/**
 * Parse command-line options.
 */
int getopt(int nargc, char * const nargv[], const char * ostr);

/**
 * Test for a terminal device.
 */
int isatty(int fd);

/**
 * @}
 */

__END_DECLS
#endif /* !KERNEL_INTERNAL */

#endif /* _UNISTD_H */

/**
 * @}
 */

/**
 * @}
 */
