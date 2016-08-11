/**
 *******************************************************************************
 * @file    fb.h
 * @author  Olli Vanhoja
 * @brief   Generic frame buffer driver.
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
#ifndef HAL_FB_H
#define HAL_FB_H

#include <stddef.h>
#include <stdint.h>
#include <sys/fb.h>
#include <sys/linker_set.h>

struct dev_info;
struct buf;

/*
 * Console option flags.
 */
#define FB_CONSOLE_WRAP 0x01 /*!< Wrap lines. */

/**
 * Frame buffer console state and configuration.
 */
struct fb_console {
    unsigned flags;
    size_t max_cols;
    size_t max_rows;
    struct fb_console_state {
        int cursor_state;
        size_t consx;
        size_t consy;
        uint32_t fg_color; /*!< Current fg color. */
        uint32_t bg_color; /*!< Current bg color. */
    } state;
};

/*
 * FB feature flags.
 */
#define FB_CONF_FEATURE_HW_CURSOR   0x01

/**
 * Frame buffer configuration.
 */
struct fb_conf {
    int feature;
    struct buf mem; /* This will be used for user space mappings. */
    size_t width;
    size_t height;
    size_t pitch;
    size_t depth;
    struct fb_console con;

    /**
     * Change screen resolution.
     * This should be set by the actual hw driver.
     * @param width is the screen width.
     * @param height is the screen height.
     * @param depth is the color depth.
     */
    int (*set_resolution)(struct fb_conf * fb, size_t width, size_t height,
                          size_t depth);
    int (*set_page)(struct fb_conf * fb, int i);
    int (*set_hw_cursor_state)(int enable, int x, int y);
};

/**
 * Init fb buffer struct used for user space mappings.
 * This function must be called only once when creating a new FB.
 * @param fb is a pointer to the frame buffer device configuration.
 */
void fb_mm_initbuf(struct fb_conf * fb);

/**
 * Update buffer attributes in mem struct.
 * This fnuction must be called after changing FB hardware configuration.
 * @param fb is a pointer to the frame buffer device configuration.
 * @param region is a pointer to the memory region used to access frame buffer
 *               memory.
 */
void fb_mm_updatebuf(struct fb_conf * restrict fb,
                     const mmu_region_t * restrict region);

/**
 * Register a new initialized frame buffer device with the kernel.
 * @param fb is a pointer to the frame buffer device configuration.
 */
int fb_register(struct fb_conf * fb);

/**
 * Write text to a frame buffer console.
 * @param fb is a pointer to the frame buffer device configuration.
 * @param text is a pointer to a C string.
 */
void fb_console_write(struct fb_conf * fb, char * text);

int fb_console_set_cursor(struct fb_conf * fb, int state, int col, int row);

#ifdef FB_INTERNAL
/**
 * Initialize fb_console struct in fb_conf struct.
 * This function can be only called after (or by) fb_register() has been called
 * and normally this function shall be called only by fb_register().
 */
void fb_console_init(struct fb_conf * fb);

/**
 * Make a tty console device for a frame buffer.
 * This is called by fb_register() to create a tty file for the console.
 * @param fb is a pointer to the frame buffer device configuration.
 * @param dv_id is the device identification number used for this tty.
 */
int fb_console_maketty(struct fb_conf * fb, dev_t dev_id);
#endif

#endif /* HAL_FB_H */
