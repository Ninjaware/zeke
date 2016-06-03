/**
 *******************************************************************************
 * @file    bio.c
 * @author  Olli Vanhoja
 * @brief   IO Buffer Cache.
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

#include <errno.h>
#include <fcntl.h>
#include <idle.h>
#include <kstring.h>
#include <sys/linker_set.h>
#include <sys/tree.h>
#include <sys/types.h>
#include <buf.h>
#include <fs/devfs.h>
#include <kerror.h>
#include <kmalloc.h>

/*
 * Used to protect access caching data structures and synchronizing access
 * to some functions.
 *
 * TODO We'd like to use MTX_TYPE_TICKET here but bio_clean() makes it
 * impossible right now.
 */
static mtx_t cache_lock = MTX_INITIALIZER(MTX_TYPE_SPIN, MTX_OPT_SLEEP |
                                                         MTX_OPT_PRICEIL);
static TAILQ_HEAD(bio_relse_list_head, buf) relse_list =
     TAILQ_HEAD_INITIALIZER(relse_list);

static void _bio_readin(struct buf * bp);
static void _bio_writeout(struct buf * bp);
static void bl_brelse(struct buf * bp);
static int biowait_timo(struct buf * bp, long timeout);
static void bio_clean(uintptr_t freebufs);

SPLAY_GENERATE(bufhd_splay, buf, sentry_, biobuf_compar);

/* Init bio, called by vralloc_init() */
void _bio_init(void)
{
    cache_lock.pri.p_lock = NICE_MIN;
}

/*
 * Comparator for buffer splay trees.
 */
int biobuf_compar(struct buf * a, struct buf * b)
{
    KASSERT(a->b_file.vnode == b->b_file.vnode,
            "vnodes differ in the same tree");

    return (a->b_blkno - b->b_blkno);
}

int bread(vnode_t * vnode, size_t blkno, int size, struct buf ** bpp)
{
    struct buf * bp;

    bp = getblk(vnode, blkno, size, 0);
    if (!bp)
        return -ENOMEM;

    BUF_LOCK(bp);
    _bio_readin(bp);
    BUF_UNLOCK(bp);

    bp->b_bcount = size;
    *bpp = bp;

    return 0;
}

int  breadn(vnode_t * vnode, size_t blkno, int size, size_t rablks[],
            int rasizes[], int nrablks, struct buf ** bpp)
{
    /* TODO */
    return -ENOTSUP;
}

void bio_readin(struct buf * bp)
{
    BUF_LOCK(bp);
    _bio_readin(bp);
    BUF_UNLOCK(bp);
}

static void _bio_readin(struct buf * bp)
{
    file_t * file;
    vnode_t * vnode;
    struct uio uio;

    KASSERT(mtx_test(&bp->lock), "bp should be locked\n");

    /*
     * If we have a separate device file associated with the buffer we should
     * use it.
     */
    if (bp->b_devfile.vnode) {
        file = &bp->b_devfile;
    } else {
        file = &bp->b_file;
    }
    vnode = file->vnode;

    bp->b_flags &= ~B_DONE;

    if (uio_buf2kuio(bp, &uio)) {
        /* TODO Error handling */
        return;
    }
    vnode->vnode_ops->lseek(file, bp->b_blkno, SEEK_SET);
    vnode->vnode_ops->read(file, &uio, bp->b_bcount);

    bp->b_flags |= B_DONE;
}

void bio_writeout(struct buf * bp)
{
    BUF_LOCK(bp);
    _bio_writeout(bp);
    BUF_UNLOCK(bp);
}

/*
 * It's a good idea to have lock on bp before calling this function.
 */
static void _bio_writeout(struct buf * bp)
{
    file_t * file;
    vnode_t * vnode;
    struct uio uio;

    KASSERT(mtx_test(&bp->lock), "bp should be locked\n");

    if (bp->b_flags & B_NOSYNC)
        goto out;

    /*
     * If we have a separate device file associated with the buffer we should
     * use it.
     */
    if (bp->b_devfile.vnode) {
        file = &bp->b_devfile;
    } else {
        file = &bp->b_file;
    }
    vnode = file->vnode;

    if (uio_buf2kuio(bp, &uio)) {
        /* TODO Error handling */
        return;
    }
    vnode->vnode_ops->lseek(file, bp->b_blkno, SEEK_SET);
    vnode->vnode_ops->write(file, &uio, bp->b_bcount);

out:
    bp->b_flags |= B_DONE;
}

int bwrite(struct buf * bp)
{
    unsigned flags;
    vnode_t * vnode;

    KASSERT(bp, "bp != NULL\n");

    vnode = bp->b_file.vnode;

    /* Sanity check */
    if (!vnode) {
        BUF_LOCK(bp);
        bp->b_flags |= B_ERROR;
        bp->b_error = -EIO;
        BUF_UNLOCK(bp);

        return -EIO;
    }

    BUF_LOCK(bp);
    flags = bp->b_flags;
    bp->b_flags &= ~(B_DONE | B_ERROR | B_ASYNC | B_DELWRI);
    bp->b_flags |= B_BUSY;
    bp->b_error = 0;
    BUF_UNLOCK(bp);

    /* TODO Use dirty offsets */
    if (flags & B_ASYNC) {
        /* TODO */

        return 0;
    } else {
        BUF_LOCK(bp);
        _bio_writeout(bp);
        bp->b_flags &= ~B_BUSY;
        BUF_UNLOCK(bp);
    }

    return 0;
}

void bawrite(struct buf * bp)
{
    BUF_LOCK(bp);
    bp->b_flags |= B_ASYNC;
    BUF_UNLOCK(bp);
    bwrite(bp);
}

void bdwrite(struct buf * bp)
{
    BUF_LOCK(bp);
    bp->b_flags |= B_DELWRI;
    BUF_UNLOCK(bp);
}

void bio_clrbuf(struct buf * bp)
{
    unsigned flags;

    KASSERT(bp, "bp != NULL\n");

    BUF_LOCK(bp);

    flags = bp->b_flags;

    if (flags & B_DELWRI) {
        _bio_writeout(bp);
    } else if (flags & B_ASYNC) {
        biowait(bp);
    }
    bp->b_flags &= ~(B_DELWRI | B_ERROR);
    bp->b_flags |= B_BUSY;
    BUF_UNLOCK(bp);

    memset((void *)bp->b_data, 0, bp->b_bufsize);

    BUF_LOCK(bp);
    bp->b_flags &= ~B_BUSY;
    BUF_UNLOCK(bp);
}

static struct buf * create_blk(vnode_t * vnode, size_t blkno, size_t size,
                               int slptimeo)
{
    struct buf * bp = geteblk(size);

    if (!bp)
        return NULL;

    bp->b_blkno = blkno;

    /* fd for the file */
    fs_fildes_set(&bp->b_file, vnode, O_RDWR);
    bp->b_file.stream = NULL;

    fs_fildes_set(&bp->b_devfile, vnode, O_RDWR);
    bp->b_devfile.stream = NULL;

    /* fd for the device */
    if (!S_ISBLK(vnode->vn_mode) && !S_ISCHR(vnode->vn_mode)) {
        if (bp->b_file.vnode->sb && bp->b_file.vnode->sb->sb_dev)
            bp->b_devfile.vnode = bp->b_file.vnode->sb->sb_dev;
        else
            panic("file->vnode->sb->sb_dev not set");
    } else {
        bp->b_devfile.vnode = NULL;
    }

    bp->b_flags |= B_DONE;
    bp->b_flags &= ~B_BUSY; /* Unbusy for now */

    VN_LOCK(vnode);

    /* Put to the buffer splay tree of the vnode. */
    if (SPLAY_INSERT(bufhd_splay, &vnode->vn_bpo.sroot, bp))
        panic("Double insert");

    VN_UNLOCK(vnode);

    return bp;
}

struct buf * getblk(vnode_t * vnode, size_t blkno, size_t size, int slptimeo)
{
    struct buf * bp;

    if (!vnode)
        return NULL;

    /* For now we want to synchronize access to this function. */
    mtx_lock(&cache_lock);

    bp = incore(vnode, blkno);
    if (!bp) { /* Not found, create a new buffer. */
        bp = create_blk(vnode, blkno, size, slptimeo);
        if (!bp)
            goto fail;
    }

    /* Found */
retry:
    biowait(bp); /* Wait until I/O has completed. */

    /*
     * Wait until the buffer is released. It is possible that we don't get it
     * locked for us on first try, so we just keep trying until it's not set
     * busy by some other thread.
     */
    while (bp->b_flags & B_BUSY)
        thread_yield(THREAD_YIELD_LAZY);
    BUF_LOCK(bp);
    if (bp->b_flags & B_BUSY) {
        BUF_UNLOCK(bp);
        goto retry;
    }
    bp->b_flags |= B_BUSY;
    /* Remove from the released list. */
    TAILQ_REMOVE(&relse_list, bp, relse_entry_);
    BUF_UNLOCK(bp);

    allocbuf(bp, size); /* Resize if necessary */

    BUF_LOCK(bp);
    bp->b_flags &= ~B_ERROR;
    bp->b_error = 0;
    BUF_UNLOCK(bp);

fail:
    mtx_unlock(&cache_lock);

    return bp;
}

struct buf * incore(vnode_t * vnode, size_t blkno)
{
    struct bufhd * bf;
    struct buf * bp;
    struct buf find;

    if (!vnode)
        return NULL;

    bf = &vnode->vn_bpo;

    if (SPLAY_EMPTY(&bf->sroot))
        return NULL;

    find.b_file.vnode = vnode;
    find.b_blkno = blkno;
    bp = SPLAY_FIND(bufhd_splay, &bf->sroot, &find);

    return bp;
}

static void bl_brelse(struct buf * bp)
{
    KASSERT(mtx_test(&bp->lock), "Lock is required.");

    bp->b_flags &= ~B_BUSY;

    mtx_lock(&cache_lock);
    TAILQ_INSERT_TAIL(&relse_list, bp, relse_entry_);
    mtx_unlock(&cache_lock);
}

void brelse(struct buf * bp)
{
    BUF_LOCK(bp);
    bl_brelse(bp);
    BUF_UNLOCK(bp);
}

void biodone(struct buf * bp)
{
    BUF_LOCK(bp);

    KASSERT(bp->b_flags & B_DONE, "dup biodone");

    bp->b_flags |= B_DONE;

    if (bp->b_flags & B_ASYNC)
        bl_brelse(bp);

    BUF_UNLOCK(bp);
}

static int biowait_timo(struct buf * bp, long timeout)
{
    /* TODO timeout */

    while (!(bp->b_flags & B_DONE));

    return bp->b_error;
}

int biowait(struct buf * bp)
{
    return biowait_timo(bp, 0);
}

/**
 * Cleanup released buffers.
 * @param freebufs  tells if released buffers should be freed after write out.
 */
static void bio_clean(uintptr_t freebufs)
{
    struct buf * bp;
    struct buf * bp_tmp;

    if (mtx_trylock(&cache_lock))
        return; /* Don't enter if we don't get exclusive access. */

    TAILQ_FOREACH_SAFE(bp, &relse_list, relse_entry_, bp_tmp) {
        file_t * file;

        /* Skip if already locked or BUSY */
        if (mtx_trylock(&bp->lock) || (bp->b_flags & B_BUSY))
            continue;

        file = &bp->b_file;

        /* Write out if delayed write was set. */
        if (bp->b_flags & B_DELWRI) {
            bp->b_flags |= B_BUSY;
            bp->b_flags &= ~B_ASYNC;

            _bio_writeout(bp);
        }

        if (freebufs &&
            !(bp->b_flags & B_LOCKED) &&
            !VN_TRYLOCK(file->vnode)) {
            SPLAY_REMOVE(bufhd_splay, &file->vnode->vn_bpo.sroot, bp);
            TAILQ_REMOVE(&relse_list, bp, relse_entry_);
            vrfree(bp);
            VN_UNLOCK(file->vnode);
        } else {
            bp->b_flags &= ~B_BUSY;
            BUF_UNLOCK(bp);
        }
    }

    mtx_unlock(&cache_lock);
}
/*
 * Idle task for cleaning up buffers.
 */
IDLE_TASK(bio_clean, 0);

int bio_geterror(struct buf * bp)
{
    int error = 0;

    BUF_LOCK(bp);

    if (bp->b_flags & B_ERROR) {
        if (bp->b_error != 0)
            error = bp->b_error;
        else
            error = -EIO;
    }

    BUF_UNLOCK(bp);

    return error;
}
