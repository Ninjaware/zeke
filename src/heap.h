/**
 *******************************************************************************
 * @file    heap.h
 * @author  Olli Vanhoja
 *
 * @brief   Header file for heap data structure for threadInfo_t (heap.c).
 *
 *******************************************************************************
 */

#pragma once
#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>
#include "sched.h"

#define HEAP_NEW_EMPTY {{NULL}, -1}

typedef struct {
    threadInfo_t * a[configSCHED_MAX_THREADS];
    int size;
} heap_t;

void heapify(heap_t * heap, int i);
void heap_del_max(heap_t * heap);
void heap_insert(heap_t * heap, threadInfo_t * k);
void heap_inc_key(heap_t * heap, int i);
void heap_dec_key(heap_t * heap, int i);

#endif /* HEAP_H */
