/**
 *******************************************************************************
 * @file    uart.c
 * @author  Olli Vanhoja
 * @brief   UART HAL.
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

#include <errno.h>
#include <fcntl.h>
#include <sys/dev_major.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <thread.h>
#include <fs/devfs.h>
#include <hal/uart.h>
#include <kinit.h>
#include <kstring.h>
#include <libkern.h>
#include <tty.h>

static const char drv_name[] = "UART";

static struct uart_port * uart_ports[UART_PORTS_MAX];
static int uart_nr_ports;
static int vfs_ready;

static int make_uartdev(struct uart_port * port, int port_num);
static ssize_t uart_read(struct tty * tty, off_t blkno,
                         uint8_t * buf, size_t bcount, int oflags);
static ssize_t uart_write(struct tty * tty, off_t blkno,
                          uint8_t * buf, size_t bcount, int oflags);
static int uart_ioctl(struct dev_info * devnfo, uint32_t request,
                      void * arg, size_t arg_len);

int __kinit__ uart_init(void)
{
    SUBSYS_DEP(devfs_init);
    SUBSYS_INIT("uart");
    vfs_ready = 1;

    /*
     * Register all UART devices with devfs that were registered with UART
     * subsystem before devfs was initialized.
     */
    for (int i = 0; i < uart_nr_ports; i++) {
        struct uart_port * port = uart_ports[i];

        if (!(port->flags & UART_PORT_FLAG_FS))
            make_uartdev(port, i);
    }

    return 0;
}

/**
 * Register a new UART with devfs.
 */
static int make_uartdev(struct uart_port * port, int port_num)
{
    struct tty * tty;
    dev_t dev_id;
    char dev_name[SPECNAMELEN];

    dev_id = DEV_MMTODEV(VDEV_MJNR_UART, port_num);
    ksprintf(dev_name, sizeof(dev_name), "ttyS%i", port_num);
    tty = tty_alloc(drv_name, dev_id, dev_name, 0);
    if (!tty) {
        return -ENOMEM;
    }

    tty->opt_data = port;
    tty->read = uart_read;
    tty->write = uart_write;
    tty->setconf = port->setconf;
    tty->ioctl = uart_ioctl;

    if (make_ttydev(tty)) {
        tty_free(tty);
        return -ENODEV;
    }

    port->flags |= UART_PORT_FLAG_FS;

    return 0;
}

int uart_register_port(struct uart_port * port)
{
    int i;

    i = uart_nr_ports;
    if (i >= UART_PORTS_MAX)
        return -1;

    uart_ports[i] = port;
    uart_nr_ports++;
    if (vfs_ready)
        make_uartdev(port, i);

    return i;
}

int uart_nports(void)
{
    return uart_nr_ports;
}

struct uart_port * uart_getport(int port_num)
{
    struct uart_port * retval = NULL;

    if (port_num < uart_nr_ports)
        retval = uart_ports[port_num];

    return retval;
}

static ssize_t uart_read(struct tty * tty, off_t blkno,
                         uint8_t * buf, size_t bcount, int oflags)
{
    struct uart_port * port = (struct uart_port *)tty->opt_data;
    size_t n = 0;

    if (!port)
        return -ENODEV;

    if (!(oflags & O_NONBLOCK)) {
        /* TODO Block until new data event */
        while (!port->peek(port)) {
            thread_sleep(50);
        }
    }

    while (n < bcount) {
        int ret = port->ugetc(port);
        if (ret == -1)
            break;
        buf[n++] = (char)ret;
    };
    if (n == 0 && bcount != 0)
        return -EAGAIN;

    return n;
}

static ssize_t uart_write(struct tty * tty, off_t blkno,
                          uint8_t * buf, size_t bcount, int oflags)
{
    struct uart_port * port = (struct uart_port *)tty->opt_data;
    const unsigned block = !(oflags & O_NONBLOCK);
    int err;

    if (!port)
        return -ENODEV;

    do {
        err = port->uputc(port, *buf);
    } while (block && err);

    if (err == -1)
        return -EAGAIN;
    return 1;
}

static int uart_ioctl(struct dev_info * devnfo, uint32_t request,
                      void * arg, size_t arg_len)
{
    struct tty * tty;
    struct uart_port * port;

    tty = (struct tty *)(devnfo->opt_data);
    if (!tty)
        return -EINVAL;

    port = (struct uart_port *)tty->opt_data;
    if (!port)
        return -ENODEV;

    /* TODO Support FIONWRITE and FIONSPACE */
    switch (request) {
    case FIONREAD:
        /*
         * Currently we don't have a generic way to tell how many bytes are
         * available but between 0 and 1 is a decent scale for most cases.
         */
        sizetto(port->peek(port) ? 1 : 0, arg, arg_len);
        break;
    default:
        return -EINVAL;
    }

    return 0;
}
