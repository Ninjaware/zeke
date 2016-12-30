/**
 *******************************************************************************
 * @file    bcm2835_prop.c
 * @author  Olli Vanhoja
 * @brief   BCM2835 Property interface.
 * @section LICENSE
 * Copyright (c) 2015, 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
#include <machine/atomic.h>
#include <buf.h>
#include <hal/hw_timers.h>
#include <kerror.h>
#include <kinit.h>
#include "bcm2835_mailbox.h"
#include "bcm2835_mmio.h"
#include "bcm2835_prop.h"

/*
 * The mailbuffer is divided to 8 sections each 512 bytes to allow assynchronous
 * calls. This size should be re-evaluated if we hit a case where it's too
 * small.
 */
#define MB_SECSIZE 512
static isema_t mb_res[8] = ISEMA_INITIALIZER(8);

static struct buf * mbuf;

int __kinit__ bcm2835_prop_init(void)
{
    SUBSYS_INIT("BCM2835_prop");

    mbuf = geteblk_special(MMU_PGSIZE_COARSE, MMU_CTRL_MEMTYPE_SO);
    if (!mbuf || mbuf->b_data == 0) {
        KERROR(KERROR_ERR, "Unable to get a mailbuffer\n");

        return -ENOMEM;
    }

    return 0;
}

int bcm2835_prop_request(uint32_t * request)
{
    uint32_t resp;
    size_t i;
    size_t offset;
    uint32_t * buf;
    uint32_t buf_hwaddr;
    int err;

    i = isema_acquire(mb_res, num_elem(mb_res));
    offset = i * MB_SECSIZE;
    buf = (uint32_t *)(mbuf->b_data + offset);
    buf_hwaddr = (uint32_t)(mbuf->b_mmu.paddr) + offset;

    /*
     * Copy request to a buffer.
     */
    memcpy(buf, request, request[0]);
    buf[1] = 0x0; /* Ensure it will be a request. */

    /* Write. */
    err = bcm2835_writemailbox(BCM2835_MBCH_PROP_OUT, buf_hwaddr);
    if (err) {
        KERROR(KERROR_ERR, "Failed to write to a prop mbox (%d)\n", err);

        return -EIO;
    }

    /* Get response. */
    err = bcm2835_readmailbox(BCM2835_MBCH_PROP_OUT, &resp);
    if (err) {
        KERROR(KERROR_DEBUG, "Failed to read from a prop mbox (%d)\n", err);

        return err;
    }
    if (buf[1] != BCM2835_STATUS_SUCCESS) {
        KERROR(KERROR_ERR, "Invalid prop mbox response (status: %u)\n",
               buf[1]);

        return -EIO;
    }

    memcpy(request, buf, buf[0]);

    isema_release(mb_res, i); /* Release the mb. */
    return 0;
}
