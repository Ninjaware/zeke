/**
 *******************************************************************************
 * @file    exec.h
 * @author  Olli Vanhoja
 * @brief   Execute a file.
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

#pragma once
#ifndef EXEC_H
#define EXEC_H

#include <sys/linker_set.h>
#include <proc.h>
#include <fs/fs.h>

struct buf;

struct exec_loadfn {
    char name[10];
    int (*test)(file_t * file);
    int (*load)(struct proc_info * proc, file_t * file, uintptr_t * vaddr_base,
                size_t * stack_size);
};

/**
 * Declare a executable loader function.
 * @param fun is the name of the function.
 * @param namestr is a C string containing the name of the executable type.
 */
#define EXEC_LOADER(_test, _load, _namestr)             \
    static struct exec_loadfn _load##_st = {            \
        .name = _namestr, .test = _test, .load = _load  \
    };                                                  \
    DATA_SET(exec_loader, _load##_st)

/**
 * Execute a file.
 * File can be elf binary, she-bang file, etc.
 */
int exec_file(struct exec_loadfn * loader, int fildes, char name[PROC_NAME_LEN],
              struct buf * env_bp, int uargc, uintptr_t uargv, uintptr_t uenvp);

#endif /* EXEC_H */
