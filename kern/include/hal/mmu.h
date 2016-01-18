/**
 *******************************************************************************
 * @file    mmu.h
 * @author  Olli Vanhoja
 * @brief   MMU headers.
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
 * @addtogroup HAL
 * @{
 */

/**
 * @addtogroup MMU
 * @{
 */

#pragma once
#ifndef MMU_H
#define MMU_H

#include <stdint.h>
#include <stddef.h>

#if !defined(configMMU)
#error MMU not enabled but header was included in some file.
#endif

/**
 * Zeke Domains
 * @{
 */
#define MMU_DOM_KERNEL  0 /*!< Kernel domain */
#define MMU_DOM_USER    0 /*!< User domain */
/**
 * @}
 */


/**
 * Page Table Types
 */
enum mmu_ptt {
    MMU_PTT_FAULT,
    MMU_PTT_COARSE,  /*!< Coarse page table type. */
    MMU_PTT_MASTER, /*!< Master page table type. */
};


/**
 * Access Permissions control
 *      Priv    User
 *      R W     R W
 * NANA 0 0     0 0
 * RONA 1 0     0 0
 * RWNA 1 1     0 0
 * RWRO 1 1     1 0
 * RWRW 1 1     1 1
 * RORO 1 0     1 0
 * @{
 */
#define MMU_AP_NANA    0x00 /*!< All accesses generate a permission fault */
#define MMU_AP_RONA    0x05 /*!< Privileged read-only and User no access */
#define MMU_AP_RWNA    0x01 /*!< Privileged access only */
#define MMU_AP_RWRO    0x02 /*!< Writes in User mode generate permission faults
                             * faults */
#define MMU_AP_RWRW    0x03 /*!< Full access */
#define MMU_AP_RORO    0x06 /*!< Privileged and User read-only */
/**
 * @}
 */


/**
 * Control bits
 * ============
 *
 * |31        |9       5|   4|  2|   1|  0|
 * +--------------------------------------+
 * | Not used | MEMTYPE | XN | - | nG | S |
 * +--------------------------------------+
 * S        = Shared
 * nG       = Determines if the translation is marked as global (0) or process
 *            specific (1).
 * XN       = Execute-Never, mark region not-executable.
 * MEMTYPE  = TEX C B
 *            987 6 5 b
 * @{
 */

/* S */
#define MMU_CTRL_S_OFFSET       0
/** Shared memory */
#define MMU_CTRL_S              (0x1 << MMU_CTRL_S_OFFSET)

/* nG */
#define MMU_CTRL_NG_OFFSET      1
/** Not-Global, use ASID */
#define MMU_CTRL_NG             (0x1 << MMU_CTRL_NG_OFFSET)

/* XN */
#define MMU_CTRL_XN_OFFSET      4
/** Execute-Never */
#define MMU_CTRL_XN             (0x1 << MMU_CTRL_XN_OFFSET)

/* MEMTYPE */
#define MMU_CTRL_MEMTYPE_OFFSET 5
/** Strongly ordered, shared */
#define MMU_CTRL_MEMTYPE_SO     (0x0 << MMU_CTRL_MEMTYPE_OFFSET)
/** Non-shareable device */
#define MMU_CTRL_MEMTYPE_DEV    (0x8 << MMU_CTRL_MEMTYPE_OFFSET)
/** Shared device */
#define MMU_CTRL_MEMTYPE_SDEV   (0x1 << MMU_CTRL_MEMTYPE_OFFSET)
/** Write-Through, shareable */
#define MMU_CTRL_MEMTYPE_WT     (0x2 << MMU_CTRL_MEMTYPE_OFFSET)
/** Write-Back, shareable */
#define MMU_CTRL_MEMTYPE_WB     (0x3 << MMU_CTRL_MEMTYPE_OFFSET)

/**
 * @}
 */
/* End of Control bits */

/**
 *  Page Table Region Macros
 */

/**
 * Page count by size of region.
 * @param size  Size of region.
 * @param psize Page size. 1024*1024 = 1 MB sections, 4096 = 4 KB small pages
 * @return Region size in pages.
 */
#define MMU_PAGE_CNT_BY_SIZE(size, psize)           ((size)/(psize))

/**
 * Page count by address range.
 * @param begin Region start address.
 * @param end   Region end address.
 * @param psize Page size. 1024*1024 = 1 MB sections, 4096 = 4 KB small pages
 * @return Region size in pages.
 */
#define MMU_PAGE_CNT_BY_RANGE(begin, end, psize)    (((end)-(begin)+1)/(psize))

/**
 * @}
 */

/**
 * Page Table Control Block - PTCB
 */
typedef struct {
    uintptr_t vaddr;    /*!< Identifies a starting virtual address of a 1MB
                         * section. (Only meaningful with coarse tables) */
    uintptr_t pt_addr;  /*!< The address where the page table is located in
                         * physical memory. */
    size_t nr_tables;   /*!< Size of the page table block in system tables.
                         *   (Only meaningful with coarse tables) */
    uintptr_t master_pt_addr; /*!< The address of a parent master L1 page
                               * table. If the table is an L1 table, then
                               * the value is same as pt_addr. */
    enum mmu_ptt pt_type; /*!< Identifies the type of the page table. */
    uint32_t pt_dom;    /*!< The domain of the page table. */
} mmu_pagetable_t;

/**
 * Region Control Block - RCB
 */
typedef struct mmu_region {
    uintptr_t vaddr;    /*!< The virtual address that begins the region in
                         * virtual memory. */
    size_t num_pages;   /*!< The number of pages in the region or region size
                         *   in mega bytes if pt points to a master page table.
                         */
    uint32_t ap;        /*!< Selects the region access permissions. */
    uint32_t control;   /*!< Selects the cache, write buffer, execution and
                         *   sharing (nG, S) attributes.
                         */
    uintptr_t paddr;      /*!< The physical starting address of the region. */
    mmu_pagetable_t * pt; /*!< A pointer to the page table struct in which
                           * the region resides. This is mostly a broken concept
                           * for vm subsystem but for now it's easier to keep it
                           * around than completely removing it. */
} mmu_region_t;

/**
 * Calc coarse page table virtual address from virtual address x.
 * Returned address is a possible start vaddr of a coarse page table.
 */
#define MMU_CPT_VADDR(x) ((x) & 0xFFF00000)

#if defined(__ARM6__) || defined(__ARM6K__) /* ARM11 uses ARMv6 arch */
#include "../../hal/arm11/arm11_mmu.h"
#else
    #error MMU for selected ARM profile/architecture is not supported
#endif

struct proc_info;
struct thread_info;

/**
 * MMU Abort type.
 */
enum mmu_abo_type {
    MMU_ABO_DATA,
    MMU_ABO_PREFETCH,
};

/**
 * MMU Abort Parameters.
 */
struct mmu_abo_param {
    enum mmu_abo_type abo_type;
    uint32_t fsr;
    uint32_t far;
    uint32_t psr;
    uint32_t lr;
    struct proc_info * proc;
    struct thread_info * thread;
};

/**
 * A typedef for prefetch and data abort handlers.
 */
typedef int abo_handler(const struct mmu_abo_param * restrict abo);

/**
 * "Generic" MMU interface, must be implemented by HAL
 * @{
 */
int mmu_init_pagetable(const mmu_pagetable_t * pt);
int mmu_map_region(const mmu_region_t * region);
int mmu_unmap_region(const mmu_region_t * region);
int mmu_attach_pagetable(const mmu_pagetable_t * pt);
int mmu_detach_pagetable(const mmu_pagetable_t * pt);
uint32_t mmu_domain_access_get(void);
void mmu_domain_access_set(uint32_t value, uint32_t mask);
void mmu_control_set(uint32_t value, uint32_t mask);
void * mmu_translate_vaddr(const mmu_pagetable_t * pt, uintptr_t vaddr);

const char * mmu_abo_strtype(const struct mmu_abo_param * restrict abo);

/**
 * Get error string for a MMU abort.
 */
const char * mmu_abo_strerror(const struct mmu_abo_param * restrict abo);

/**
 * @}
 */

/**
 * Generic for all, implemented in mmu.c
 * @{
 */

void mmu_pf_event(void);

/**
 * Get the size of a page table entry.
 * @param pt is the page table.
 * @return Returns the size of the page table or zero in case of error.
 */
size_t mmu_sizeof_pt(const mmu_pagetable_t * region);

/**
 * Get the size of a page table image.
 * The size of the mapping pointed by pt.
 * @param pt is the page table.
 * @return Retunrs the  size og the page table image.
 */
size_t mmu_sizeof_pt_img(const mmu_pagetable_t * pt);

/**
 * Calculate size of a vm region.
 * @param region is a pointer to the region control block.
 * @return Returns the size of the region in bytes.
 */
size_t mmu_sizeof_region(const mmu_region_t * region);
int mmu_ptcpy(mmu_pagetable_t * dest, const mmu_pagetable_t * src);

/**
 * Die on fatal MMU related abort.
 * This can be called from an DAB/PAB interrupt handler when the
 * interrupted thread must be killed by a signal.
 * ksignal_sendsig_fatal() must be called before calling this
 * function.
 */
void mmu_die_on_fatal_abort(void);

/**
 * Print a stack dump to kerror on MMU abort.
 */
void mmu_abo_dump(const struct mmu_abo_param * restrict abo);

/**
 * @}
 */

#endif /* MMU_H */

/**
 * @}
 */


/**
 * @}
 */
