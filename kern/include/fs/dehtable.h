/**
 *******************************************************************************
 * @file    dehtable.h
 * @author  Olli Vanhoja
 * @brief   Directory Entry Hashtable.
 * @section LICENSE
 * Copyright (c) 2013 - 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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
 * @addtogroup fs
 * @{
 */

#pragma once
#ifndef DEHTABLE_H
#define DEHTABLE_H

#include <fs/fs.h>

#define DEHTABLE_SIZE 16

/**
 * Directory entry.
 */
typedef struct dh_dirent {
    ino_t dh_ino; /*!< File serial number. */
    uint8_t dh_type; /*!< Dirent type. */
    size_t dh_size; /*!< Size of this directory entry. */
    uint8_t dh_link; /*!< Internal link tag. If 0 chain ends here; otherwise
                      * chain linking continues. */
    char dh_name[1]; /*!< Name of the entry. */
} dh_dirent_t;

/**
 * Directory entry hash table array type.
 */
typedef struct dh_table {
    uint32_t k[2];
    struct dh_dirent * htable[DEHTABLE_SIZE];
} dh_table_t;

/**
 * Directory iterator.
 */
typedef struct dh_dir_iter {
    dh_table_t * dir;
    size_t dea_ind;
    size_t ch_ind;
} dh_dir_iter_t;

/**
 * Initialize a directory entry table.
 */
void dh_init(dh_table_t * dir);

/**
 * Insert a new directory entry link.
 * @param dir       is a directory entry table.
 * @param vnode_num is the inode number.
 * @param dt_type   is the dirent type as presented by dirent.h.
 * @param name      is the name of the hard link.
 * @return Returns 0 if succeed; Otherwise a negative errno code is returned.
 */
int dh_link(dh_table_t * dir, ino_t vnode_num, uint8_t d_type,
            const char * name);

/**
 * Remove a directory entry link.
 * @param dir       is the dh_table of a directory.
 * @param name      is the name of directory entry.
 * @return Returns 0 if succeed; Otherwise a negative errno code is returned.
 */
int dh_unlink(dh_table_t * dir, const char * name);

/**
 * Destroy all dir entries.
 * @param dir is a directory entry hash table.
 */
void dh_destroy_all(dh_table_t * dir);

/**
 * Lookup for hard link in dh_table of dir.
 * @param dir       is the dh_table of a directory.
 * @param name      is the link name searched for.
 * @param name_len  is the length of name.
 * @param vnode_num is the vnode number the link is pointing to.
 * @return Returns zero if link with specified name was found;
 *         Otherwise a negative errno code is returned.
 */
int dh_lookup(dh_table_t * dir, const char * name, ino_t * vnode_num);

/**
 * Reverse lookup for directory entry name in dh_table by ino.
 * @param dir       is the dh_table of a directory.
 * @param ino       is the inode number of the vnode.
 * @param[out] name is destination string.
 * @param name_len  is the size of name_len
 * @return Returns zero if link with specified inode number was found;
 *         Otherwise a negative errno code is returned.
 */
int dh_revlookup(dh_table_t * dir, ino_t ino, char * name, size_t name_len);

/**
 * Get a dirent hashtable iterator.
 * @param is a directory entry hash table.
 * @return Returns a dent hash table iterator struct.
 */
dh_dir_iter_t dh_get_iter(dh_table_t * dir);

/**
 * Get the next directory entry from iterator it.
 * @param it is a dirent hash table iterator.
 * @return Next directory entry in hash table.
 */
dh_dirent_t * dh_iter_next(dh_dir_iter_t * it);

/**
 * Get the number of entries in a dh_table.
 */
size_t dh_nr_entries(dh_table_t * dir);

#endif /* DEHTABLE_H */

/**
 * @}
 */
