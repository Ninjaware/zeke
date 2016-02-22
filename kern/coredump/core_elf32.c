/**
 *******************************************************************************
 * @file    core_elf32.c
 * @author  Olli Vanhoja
 * @brief   32bit ELF core dumps.
 * @section LICENSE
 * Copyright (c) 2015 - 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <fcntl.h>
#include <machine/endian.h>
#include <stdint.h>
#include <sys/elf32.h>
#include <buf.h>
#include <fs/fs.h>
#include <hal/core.h>
#include <kerror.h>
#include <kmalloc.h>
#include <proc.h>

#define SKIP_REGION(_region) \
    ((_region)->b_flags & B_NOCORE || (_region)->b_data == 0 || \
     (_region)->b_mmu.vaddr == 0)

static off_t write2file(file_t * file, void * p, size_t size)
{
    vnode_t * vn = file->vnode;
    struct uio uio;

    uio_init_kbuf(&uio, p, size);
    return vn->vnode_ops->write(file, &uio, size);
}

static off_t write_elf_header(file_t * file, int phnum)
{
    const size_t elf32_header_size = sizeof(struct elf32_header);
    struct elf32_header hdr = {
        .e_type = ET_CORE,
        .e_machine = EM_ARM, /* TODO get it from somewhere. */
        .e_version = EV_CURRENT,
        .e_phoff = elf32_header_size,
        .e_shoff = 0,
        .e_ehsize = elf32_header_size,
        .e_flags = 0,
        .e_phentsize = sizeof(struct elf32_phdr),
        .e_phnum = phnum,
        .e_shentsize = 0, /* No section headers. */
        .e_shnum = 0,
        .e_shstrndx = 0,
    };

    hdr.e_ident[EI_MAG0] = ELFMAG0;
    hdr.e_ident[EI_MAG1] = ELFMAG1;
    hdr.e_ident[EI_MAG2] = ELFMAG2;
    hdr.e_ident[EI_MAG3] = ELFMAG3;
    hdr.e_ident[EI_VERSION] = EV_CURRENT;
    hdr.e_ident[EI_CLASS] = ELFCLASS32;
    hdr.e_ident[EI_DATA] = ELFDATA_MACH;
    hdr.e_ident[EI_OSABI] = ELFOSABI_NONE;

    /* Write the elf header. */
    return write2file(file, &hdr, elf32_header_size);
}

static uint32_t uap2p_flags(struct buf * bp)
{
    int uap = bp->b_uflags;
    uint32_t p_flags = 0;

    p_flags |= (uap & VM_PROT_READ)     ? PF_R : 0;
    p_flags |= (uap & VM_PROT_WRITE)    ? PF_W : 0;
    p_flags |= (uap & VM_PROT_EXECUTE)  ? PF_X : 0;
    p_flags |= (uap & VM_PROT_COW)      ? PF_ZEKE_COW : 0;

    return p_flags;
}

static size_t put_note_header(void * note, size_t n_descsz, unsigned type)
{
    const char name[] = { 'C', 'O', 'R', 'E' };
    struct elf_note note_s = {
        .n_namesz = sizeof(name),
        .n_descsz = n_descsz,
        .n_type = type,
    };

    memcpy(note, &note_s, sizeof(note_s));
    memcpy((char *)note + sizeof(note_s), name, sizeof(name));

    return sizeof(note_s) + sizeof(name);
}

/**
 * Build a prstatus note.
 * @param proc is a pointer to the process.
 * @param thread is a pointer to the thread.
 * @param note is a pointer to the current note slot.
 * @return Returns the number of bytes written.
 */
static size_t thread_prstatus(const struct proc_info * proc,
                              struct thread_info * thread,
                              void * note)
{
    size_t bytes = 0;
    prstatus_t prstatus = {
        .pr_cursig = 0,
        .pr_pid = thread->id, /* No separate thread IDs in Linux. */
        .pr_ppid = (proc->inh.parent) ? proc->inh.parent->pid : 0,
        .pr_pgrp = proc->pgrp->pg_id,
        .pr_sid = proc->pgrp->pg_session->s_leader,
        /* TODO set times */
        .pr_fpvalid = (IS_HFP_PLAT) ? 1 : 0,
    };
    const sw_stack_frame_t * sf = NULL;

    /*
     * Restore the last stack frame and signal status.
     */
    if (thread->exit_ksiginfo) {
        siginfo_t * siginfo = &thread->exit_ksiginfo->siginfo;
        struct elf_siginfo einfo = {
            .si_signo = siginfo->si_signo,
            .si_code = siginfo->si_code,
            .si_errno = siginfo->si_errno,
        };

        prstatus.pr_cursig = siginfo->si_signo;
        prstatus.pr_info = einfo;

    }

    sf = get_usr_sframe(thread);
    if (!sf) {
        KERROR(KERROR_INFO,
               "get_usr_sframe() failed, assuming SCHED_SFRAME_ABO\n");
        /* FIXME HW dependant */
        sf = &thread->sframe.s[SCHED_SFRAME_ABO];
    }
    elf_gregset_t gregs = {
        sf->r0,
        sf->r1,
        sf->r2,
        sf->r3,
        sf->r4,
        sf->r5,
        sf->r6,
        sf->r7,
        sf->r8,
        sf->r9,
        sf->r10,
        sf->r11,
        sf->r12,
        sf->sp,
        sf->lr,
        sf->pc,
        sf->psr,
        -1,
    };
    memcpy(&prstatus.pr_reg, gregs, sizeof(elf_gregset_t));

    bytes = put_note_header(note, sizeof(prstatus), NT_PRSTATUS);
    memcpy((uint8_t *)note + bytes, &prstatus, sizeof(prstatus));
    bytes += sizeof(prstatus);

    return bytes;
}

/**
 * Iterate through threads and construct prstatus struct for each thread.
 */
static size_t build_note_prstatus(const struct proc_info * proc, void * note)
{
    struct thread_info * thread;
    struct thread_info * thread_it = NULL;
    size_t bytes = 0;

    while ((thread = proc_iterate_threads(proc, &thread_it))) {
        size_t off;

        off = thread_prstatus(proc, thread, note);
        note = (char *)note + off;
        bytes += off;
    }

    return bytes;
}

/**
 * GDB compatible process status info.
 */
static size_t build_note_prpsinfo(const struct proc_info * proc, void * note)
{
    size_t bytes = 0;
    prpsinfo_t prpsinfo = {
        .pr_state = proc->state,
        .pr_sname = "IRRWSZD"[proc->state],
        .pr_zomb = (proc->state == PROC_STATE_ZOMBIE),
        .pr_nice = proc->priority,
        .pr_flag = 0, /* We don't have process flags. */
        .pr_uid = proc->cred.uid, /* RFE or e? */
        .pr_gid = proc->cred.gid,
        .pr_pid = proc->pid,
        .pr_ppid = (proc->inh.parent) ? proc->inh.parent->pid : 0,
        .pr_pgrp = proc->pgrp->pg_id,
        .pr_sid = proc->pgrp->pg_session->s_leader,
    };

    strlcpy(prpsinfo.pr_fname, proc->name, sizeof(prpsinfo.pr_fname));
    /*
     * It's impossible to reliable get any args but at least we can provide
     * the process name for GDB here.
     */
    strlcpy(prpsinfo.pr_psargs, proc->name, sizeof(prpsinfo.pr_psargs));

    bytes = put_note_header(note, sizeof(prpsinfo), NT_PRPSINFO);
    memcpy((uint8_t *)note + bytes, &prpsinfo, sizeof(prpsinfo));
    bytes += sizeof(prpsinfo);

    return bytes;
}

static size_t build_note_siginfo(const struct proc_info * proc, void * note)
{
    size_t bytes = 0;

    if (proc->main_thread && proc->main_thread->exit_ksiginfo) {
        siginfo_t siginfo = proc->main_thread->exit_ksiginfo->siginfo;

        bytes = put_note_header(note, sizeof(siginfo), NT_SIGINFO);
        memcpy((uint8_t *)note + bytes, &siginfo, sizeof(siginfo));
        bytes += sizeof(siginfo);
    }

    return bytes;
}

/**
 * Dump process credentials.
 */
static size_t build_note_prcred(const struct proc_info * proc, void * note)
{
    size_t bytes;

    bytes = put_note_header(note, sizeof(struct cred), NT_PRCRED);
    memcpy((uint8_t *)note + bytes, &proc->cred, sizeof(struct cred));
    bytes += sizeof(struct cred);

    return bytes;
}

/**
 * Add the full siginfo delivered to the main_thread if applicable.
 */
static size_t build_notes(struct proc_info * proc, void ** notes_out)
{
    void * notes;
    uint8_t * note;
    size_t off = 0;
    size_t (*note_builder[])(const struct proc_info * proc, void * note) = {
         build_note_prstatus,
         build_note_prpsinfo,
         build_note_siginfo,
         build_note_prcred,
    };

    /* TODO
     * - tls registers
     */

    /*
     * RFE How to determine how much memory is needed?
     */
    notes = kzalloc(2048);
    note = (uint8_t *)notes;
    for (size_t i = 0; i < num_elem(note_builder); i++) {
        off += note_builder[i](proc, note);
        note = (uint8_t *)notes + off;
    }

    *notes_out = notes;
    return off;
}

static int create_pheaders(const struct vm_mm_struct * mm, size_t notes_size,
                           struct elf32_phdr ** phdr_arr_out)
{
    struct elf32_phdr * phdr_arr;
    int i, hi, nr_core_regions, phnum;
    size_t phsize;
    off_t offset;

    /*
     * Count dumpable memory regions.
     */
    nr_core_regions = 0;
    for (i = 0; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];

        if (!SKIP_REGION(region))
            nr_core_regions++;
    }

    phnum = 1 + nr_core_regions;
    phsize = phnum * sizeof(struct elf32_phdr);
    phdr_arr = kzalloc(phsize);
    if (!phdr_arr)
        return -ENOMEM;

    /* Assume that program headers start right after the elf header. */
    offset = sizeof(struct elf32_header) + phsize;

    /* NOTE section. */
    phdr_arr[0] = (struct elf32_phdr){
        .p_type = PT_NOTE,
        .p_offset = offset,
        .p_vaddr = 0,
        .p_paddr = 0,
        .p_filesz = notes_size,
        .p_memsz = 0,
        .p_flags = 0,
        .p_align = 0,
    };
    offset += notes_size;

    /*
     * Memory region headers.
     */
    for (i = 0, hi = 1; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];
        struct elf32_phdr * phdr = phdr_arr + hi;

        if (SKIP_REGION(region))
            continue;

        *phdr = (struct elf32_phdr){
            .p_type = PT_LOAD,
            .p_offset = offset,
            .p_vaddr = region->b_mmu.vaddr,
            .p_paddr = region->b_mmu.paddr, /* Linux sets to 0. */
            .p_filesz = region->b_bufsize,
            .p_memsz = region->b_bufsize,
            .p_flags = uap2p_flags(region),
            .p_align = MMU_PGSIZE_COARSE,
        };
        offset += region->b_bufsize;
        hi++;
    }

    *phdr_arr_out = phdr_arr;
    return phnum;
}

static off_t dump_regions(file_t * file, const struct vm_mm_struct * mm)
{
    off_t err, off = 0;

    for (int i = 0; i < mm->nr_regions; i++) {
        struct buf * region = (*mm->regions)[i];

        if (SKIP_REGION(region))
            continue;

        err = write2file(file, (void *)region->b_data, region->b_bufsize);
        if (err != region->b_bufsize)
            return err;
        off += off;
    }

    return off;
}

int core_dump2file(struct proc_info * proc, file_t * file)
{
    vnode_t * vn = file->vnode;
    struct vm_mm_struct * mm;
    kmalloc_autofree struct elf32_phdr * phdr = NULL;
    kmalloc_autofree void * notes = NULL;
    int phnum, retval;
    off_t err;
    size_t phsize, notes_size;

    if (vn->vnode_ops->lseek(file, 0, SEEK_SET) < 0) {
        return -EINVAL;
    }

    mm = &proc->mm;
    mtx_lock(&mm->regions_lock);

    notes_size = build_notes(proc, &notes);
    phnum = create_pheaders(mm, notes_size, &phdr);
    phsize = phnum * sizeof(struct elf32_phdr);

    if ((err = write_elf_header(file, phnum)) < 0 ||
        (err = write2file(file, phdr, phsize)) < 0 ||
        (err = write2file(file, notes, notes_size)) < 0 ||
        (err = dump_regions(file, mm)) < 0) {
        retval = err;
        goto out;
    }

    retval = 0;
out:
    mtx_unlock(&mm->regions_lock);
    return retval;
}
