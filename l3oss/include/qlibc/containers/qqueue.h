/******************************************************************************
 * qLibc
 *
 * Copyright (c) 2010-2015 Seungyoung Kim.
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
 *****************************************************************************/

/**
 * Queue container.
 *
 * @file qqueue.h
 */

#ifndef QQUEUE_H
#define QQUEUE_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include "qlist.h"

#ifdef __cplusplus
extern "C" {
#endif

/* types */
typedef struct qqueue_s qqueue_t;

enum {
    QQUEUE_THREADSAFE = (QLIST_THREADSAFE)  /*!< make it thread-safe */
};

/* member functions
 *
 * All the member functions can be accessed in both ways:
 *  - tbl->push(tbl, ...);    // easier to switch the container type to other kinds.
 *  - qqueue_push(tbl, ...);  // where avoiding pointer overhead is preferred.
 */
extern qqueue_t *qqueue(int options);
extern size_t qqueue_setsize(qqueue_t *queue, size_t max);

extern bool qqueue_push(qqueue_t *queue, const void *data, size_t size);
extern bool qqueue_pushstr(qqueue_t *queue, const char *str);
extern bool qqueue_pushint(qqueue_t *queue, int64_t num);

extern void *qqueue_pop(qqueue_t *queue, size_t *size);
extern char *qqueue_popstr(qqueue_t *queue);
extern int64_t qqueue_popint(qqueue_t *queue);
extern void *qqueue_popat(qqueue_t *queue, int index, size_t *size);

extern void *qqueue_get(qqueue_t *queue, size_t *size, bool newmem);
extern char *qqueue_getstr(qqueue_t *queue);
extern int64_t qqueue_getint(qqueue_t *queue);
extern void *qqueue_getat(qqueue_t *queue, int index, size_t *size, bool newmem);

extern size_t qqueue_size(qqueue_t *queue);
extern void qqueue_clear(qqueue_t *queue);
extern bool qqueue_debug(qqueue_t *queue, FILE *out);
extern void qqueue_free(qqueue_t *queue);

/**
 * qqueue container object structure
 */
struct qqueue_s {
    /* encapsulated member functions */
    size_t (*setsize) (qqueue_t *stack, size_t max);

    bool (*push) (qqueue_t *stack, const void *data, size_t size);
    bool (*pushstr) (qqueue_t *stack, const char *str);
    bool (*pushint) (qqueue_t *stack, int64_t num);

    void *(*pop) (qqueue_t *stack, size_t *size);
    char *(*popstr) (qqueue_t *stack);
    int64_t (*popint) (qqueue_t *stack);
    void *(*popat) (qqueue_t *stack, int index, size_t *size);

    void *(*get) (qqueue_t *stack, size_t *size, bool newmem);
    char *(*getstr) (qqueue_t *stack);
    int64_t (*getint) (qqueue_t *stack);
    void *(*getat) (qqueue_t *stack, int index, size_t *size, bool newmem);

    size_t (*size) (qqueue_t *stack);
    void (*clear) (qqueue_t *stack);
    bool (*debug) (qqueue_t *stack, FILE *out);
    void (*free) (qqueue_t *stack);

    /* private variables - do not access directly */
    qlist_t  *list;  /*!< data container */
};

#ifdef __cplusplus
}
#endif

#endif /* QQUEUE_H */
