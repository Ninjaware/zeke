/**
 *******************************************************************************
 * @file    mmu.h
 * @author  Olli Vanhoja
 * @brief   MMU headers.
 * @section LICENSE
 * Copyright (c) 2013 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
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

/** @addtogroup HAL
  * @{
  */

/** @addtogroup ARM11
  * @{
  */

#pragma once
#ifndef ARM11_MMU_H
#define ARM11_MMU_H

#include <hal/mmu.h>

/* Zeke Domains */
#define MMU_DOM_KERNEL  0 /*!< Kernel domain */
#define MMU_DOM_APP     1 /*!< Application/Process domain */

/**
 * Size of translation table pointed by TTBR0.
 *
 * N    bound   Table size  Entries
 * --------------------------------
 * 0    4GB     16KB        4096    (ARMv5 behaviour)
 * 1    2GB     8KB         2048
 * 2    1GB     4KB         1024
 * 3    512MB   2KB         512
 * 4    256MB   1KB         256
 * 5    128MB   512B        128
 * 6    64MB    256B        64
 * 7    32MB    128B        32
 */
#define MMU_TTBCR_N     0

/* L1 Page Table Entry Types
 * These corresponds directly to the bits of first-level descriptor on ARMv6 */
#define MMU_PTE_FAULT   0 /*!< Translation fault. */
#define MMU_PTE_COARSE  1 /*!< Coarse page table. */
#define MMU_PTE_SECTION 2 /*!< Section entry. */

/* Page Table Types */
#define MMU_PTT_COARSE  MMU_PTE_COARSE  /*!< Coarse page table type. */
#define MMU_PTT_MASTER  MMU_PTE_SECTION /*!< Master page table type. */

/* Page table sizes in bytes */
#define MMU_PTSZ_FAULT      0x0000 /*!< Page table size for translation fault. */
#define MMU_PTSZ_COARSE     0x0400 /*!< Coarse page table */
#define MMU_PTSZ_MASTER     0x4000 /*!< L1 master page table size */

/* Access Permissions control
 * NA = No Access, RO = Read Only, RW = Read/Write
 * +----+
 * |2 10|
 * +----+
 * |A A |
 * |P P |
 * |X   |
 * +----+
 */
#define MMU_AP_NANA    0x00 /*!< All accesses generate a permission fault */
#define MMU_AP_RWNA    0x01 /*!< Privileged access only */
#define MMU_AP_RWRO    0x02 /*!< Writes in User mode generate permission faults
                             * faults */
#define MMU_AP_RWRW    0x03 /*!< Full access */
#define MMU_AP_RONA    0x05 /*!< Privileged read-only and User no access */
#define MMU_AP_RORO    0x06 /*!< Privileged and User read-only */


/* Control bits
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
#define MMU_CTRL_MEMTYPE_OFFSET 2
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

/* End of Control bits */


/**
 * Page Table Control Block - PTCB
 */
typedef struct {
    vaddr_t vaddr;      /*!< identifies a starting address of a 1MB section of
                         * a virtual memory controlled by either a section
                         * entry or a L2 page table. */
    uint32_t pt_addr;   /*!< is the address where the page table is located in
                         * virtual memory. */
    uint32_t master_pt_addr; /*!< is the address of a parent master L1 page
                              * table. If the table is an L1 table, then the
                              * value is same as pt_addr. */
    uint32_t type;      /*!< identifies the type of the page table. */
    uint32_t dom;       /*!< is the domain of the page table. */
} mmu_pagetable_t;

/**
 * Region Control Block - RCB
 */
typedef struct {
    vaddr_t vaddr;      /*!< is the virtual starting address of the region in
                         * virtual memory. */
    uint32_t num_pages; /*!< is the number of pages in the region. */
    uint32_t ap;        /*!< selects the region access permissions. */
    uint32_t control;   /*!< selects the cache, write buffer, execution and
                         * sharing (nG, S) attributes. */
    paddr_t paddr;      /*!< is the physical starting address of the region in
                         * virtual memory. */
    mmu_pagetable_t * pt; /*!< is a pointer to the page table in which
                           * the region resides. */
} mmu_region_t;

#endif /* ARM11_MMU_H */

/**
  * @}
  */

/**
  * @}
  */
