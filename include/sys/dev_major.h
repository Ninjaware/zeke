/**
 *******************************************************************************
 * @file    dev_major.h
 * @author  Olli Vanhoja
 * @brief   Device major codes.
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

#ifndef DEV_MAJOR_H
#define DEV_MAJOR_H

/*
 * Note that eg. MBR is using dev_major + 1 as a major code.
 * Though major minor pairs have very little real usage or meaning
 * in Zeke.
 */

#define VDEV_MJNR_SPECIAL    1
#define VDEV_MJNR_UART       4
#define VDEV_MJNR_PTY        5
#define VDEV_MJNR_EMMC       8
#define VDEV_MJNR_FB        29
#define VDEV_MJNR_FBMM      30

/* File system major numbers */
#define VDEV_MJNR_RAMFS     10
#define VDEV_MJNR_DEVFS     11
#define VDEV_MJNR_PROCFS    12
#define VDEV_MJNR_FATFS     13

#endif /* DEV_MAJOR_H */
