// SPDX-License-Identifier: BSD-2-Clause
//
// Copyright (c) 2014-2019, NetApp, Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/*      $NetBSD: tree.h,v 1.8 2004/03/28 19:38:30 provos Exp $  */
/*      $OpenBSD: tree.h,v 1.7 2002/10/17 21:51:54 art Exp $    */
/* $FreeBSD$ */

/*-
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
 * Copyright (c) 2014-2019, NetApp, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
// IWYU pragma: private, include <quant/quant.h>

/*
 * This file defines data structures for different types of trees:
 * splay trees and red-black trees.
 *
 * A splay tree is a self-organizing data structure.  Every operation
 * on the tree causes a splay to happen.  The splay moves the requested
 * node to the root of the tree and partly rebalances it.
 *
 * This has the benefit that request locality causes faster lookups as
 * the requested nodes move to the top of the tree.  On the other hand,
 * every lookup causes memory writes.
 *
 * The Balance Theorem bounds the total access time for m operations
 * and n inserts on an initially empty tree as O((m + n)lg n).  The
 * amortized cost for a sequence of m accesses to a splay tree is O(lg n);
 *
 * A red-black tree is a binary search tree with the node color as an
 * extra attribute.  It fulfills a set of conditions:
 *      - every search path from the root to a leaf consists of the
 *        same number of black nodes,
 *      - each red node (except for the root) has a black parent,
 *      - each leaf node is black.
 *
 * Every operation on a red-black tree is bounded as O(lg n).
 * The maximum height of a red-black tree is 2lg (n+1).
 */

#define splay_head(name, type)                                                 \
    _Pragma("clang diagnostic push")                                           \
        _Pragma("clang diagnostic ignored \"-Wpadded\"") struct name {         \
        struct type * sph_root; /* root of the tree */                         \
        uint_t sph_cnt;         /* number of nodes in the tree */              \
    } _Pragma("clang diagnostic pop")

#define splay_initializer(root)                                                \
    {                                                                          \
        NULL, 0                                                                \
    }

#define splay_init(root)                                                       \
    do {                                                                       \
        (root)->sph_root = NULL;                                               \
        (root)->sph_cnt = 0;                                                   \
    } while (/*CONSTCOND*/ 0)

#define splay_entry(type)                                                      \
    struct {                                                                   \
        struct type * spe_left;  /* left element */                            \
        struct type * spe_right; /* right element */                           \
    }

#define splay_left(elm, field) (elm)->field.spe_left
#define splay_right(elm, field) (elm)->field.spe_right
#define splay_root(head) (head)->sph_root
#define splay_empty(head) (splay_root(head) == NULL)
#define splay_count(head) (head)->sph_cnt

/* splay_rotate_{left,right} expect that tmp hold splay_{right,left} */
#define splay_rotate_right(head, tmp, field)                                   \
    do {                                                                       \
        splay_left((head)->sph_root, field) = splay_right(tmp, field);         \
        splay_right(tmp, field) = (head)->sph_root;                            \
        (head)->sph_root = tmp;                                                \
    } while (/*CONSTCOND*/ 0)

#define splay_rotate_left(head, tmp, field)                                    \
    do {                                                                       \
        splay_right((head)->sph_root, field) = splay_left(tmp, field);         \
        splay_left(tmp, field) = (head)->sph_root;                             \
        (head)->sph_root = tmp;                                                \
    } while (/*CONSTCOND*/ 0)

#define splay_linkleft(head, tmp, field)                                       \
    do {                                                                       \
        splay_left(tmp, field) = (head)->sph_root;                             \
        tmp = (head)->sph_root;                                                \
        (head)->sph_root = splay_left((head)->sph_root, field);                \
    } while (/*CONSTCOND*/ 0)

#define splay_linkright(head, tmp, field)                                      \
    do {                                                                       \
        splay_right(tmp, field) = (head)->sph_root;                            \
        tmp = (head)->sph_root;                                                \
        (head)->sph_root = splay_right((head)->sph_root, field);               \
    } while (/*CONSTCOND*/ 0)

#define splay_assemble(head, node, left, right, field)                         \
    do {                                                                       \
        splay_right(left, field) = splay_left((head)->sph_root, field);        \
        splay_left(right, field) = splay_right((head)->sph_root, field);       \
        splay_left((head)->sph_root, field) = splay_right(node, field);        \
        splay_right((head)->sph_root, field) = splay_left(node, field);        \
    } while (/*CONSTCOND*/ 0)

/* Generates prototypes and inline functions */

#define SPLAY_PROTOTYPE(name, type, field, cmp)                                \
    _Pragma("clang diagnostic push")                                           \
        _Pragma("clang diagnostic ignored \"-Wunused-function\"")              \
                                                                               \
            void name##_splay(struct name *, const struct type *);             \
    void name##_splay_minmax(struct name *, int);                              \
    struct type * name##_splay_insert(struct name *, struct type *);           \
    struct type * name##_splay_remove(struct name *, struct type *);           \
                                                                               \
    /* Finds the node with the same key as elm */                              \
    static inline struct type * name##_splay_find(struct name * head,          \
                                                  const struct type * elm)     \
    {                                                                          \
        if (splay_empty(head))                                                 \
            return (NULL);                                                     \
        name##_splay(head, elm);                                               \
        if ((cmp)(elm, (head)->sph_root) == 0)                                 \
            return (head->sph_root);                                           \
        return (NULL);                                                         \
    }                                                                          \
                                                                               \
    static inline struct type * name##_splay_next(struct name * head,          \
                                                  struct type * elm)           \
    {                                                                          \
        name##_splay(head, elm);                                               \
        if (splay_right(elm, field) != NULL) {                                 \
            elm = splay_right(elm, field);                                     \
            while (splay_left(elm, field) != NULL) {                           \
                elm = splay_left(elm, field);                                  \
            }                                                                  \
        } else                                                                 \
            elm = NULL;                                                        \
        return (elm);                                                          \
    }                                                                          \
                                                                               \
    static inline struct type * name##_splay_prev(struct name * head,          \
                                                  struct type * elm)           \
    {                                                                          \
        name##_splay(head, elm);                                               \
        if (splay_left(elm, field) != NULL) {                                  \
            elm = splay_left(elm, field);                                      \
            while (splay_right(elm, field) != NULL) {                          \
                elm = splay_right(elm, field);                                 \
            }                                                                  \
        } else                                                                 \
            elm = NULL;                                                        \
        return (elm);                                                          \
    }                                                                          \
                                                                               \
    static inline struct type * name##_splay_min_max(struct name * head,       \
                                                     int val)                  \
    {                                                                          \
        name##_splay_minmax(head, val);                                        \
        return (splay_root(head));                                             \
    }                                                                          \
                                                                               \
    _Pragma("clang diagnostic pop")

/* Main splay operation.
 * Moves node close to the key of elm to top
 */
#define SPLAY_GENERATE(name, type, field, cmp)                                 \
    struct type * name##_splay_insert(struct name * head, struct type * elm)   \
    {                                                                          \
        if (splay_empty(head)) {                                               \
            splay_left(elm, field) = splay_right(elm, field) = NULL;           \
        } else {                                                               \
            int __comp;                                                        \
            name##_splay(head, elm);                                           \
            __comp = (cmp)(elm, (head)->sph_root);                             \
            if (__comp < 0) {                                                  \
                splay_left(elm, field) = splay_left((head)->sph_root, field);  \
                splay_right(elm, field) = (head)->sph_root;                    \
                splay_left((head)->sph_root, field) = NULL;                    \
            } else if (__comp > 0) {                                           \
                splay_right(elm, field) =                                      \
                    splay_right((head)->sph_root, field);                      \
                splay_left(elm, field) = (head)->sph_root;                     \
                splay_right((head)->sph_root, field) = NULL;                   \
            } else                                                             \
                return ((head)->sph_root);                                     \
        }                                                                      \
        splay_count(head)++;                                                   \
        (head)->sph_root = (elm);                                              \
        return (NULL);                                                         \
    }                                                                          \
                                                                               \
    struct type * name##_splay_remove(struct name * head, struct type * elm)   \
    {                                                                          \
        struct type * __tmp;                                                   \
        if (splay_empty(head))                                                 \
            return (NULL);                                                     \
        name##_splay(head, elm);                                               \
        if ((cmp)(elm, (head)->sph_root) == 0) {                               \
            if (splay_left((head)->sph_root, field) == NULL) {                 \
                (head)->sph_root = splay_right((head)->sph_root, field);       \
            } else {                                                           \
                __tmp = splay_right((head)->sph_root, field);                  \
                (head)->sph_root = splay_left((head)->sph_root, field);        \
                name##_splay(head, elm);                                       \
                splay_right((head)->sph_root, field) = __tmp;                  \
            }                                                                  \
            splay_count(head)--;                                               \
            return (elm);                                                      \
        }                                                                      \
        return (NULL);                                                         \
    }                                                                          \
                                                                               \
    void name##_splay(struct name * const head, const struct type * const elm) \
    {                                                                          \
        struct type __node;                                                    \
        struct type * __left;                                                  \
        struct type * __right;                                                 \
        struct type * __tmp;                                                   \
        int __comp;                                                            \
                                                                               \
        splay_left(&__node, field) = splay_right(&__node, field) = NULL;       \
        __left = __right = &__node;                                            \
                                                                               \
        while ((__comp = (cmp)(elm, (head)->sph_root)) != 0) {                 \
            if (__comp < 0) {                                                  \
                __tmp = splay_left((head)->sph_root, field);                   \
                if (__tmp == NULL)                                             \
                    break;                                                     \
                if ((cmp)(elm, __tmp) < 0) {                                   \
                    splay_rotate_right(head, __tmp, field);                    \
                    if (splay_left((head)->sph_root, field) == NULL)           \
                        break;                                                 \
                }                                                              \
                splay_linkleft(head, __right, field);                          \
            } else if (__comp > 0) {                                           \
                __tmp = splay_right((head)->sph_root, field);                  \
                if (__tmp == NULL)                                             \
                    break;                                                     \
                if ((cmp)(elm, __tmp) > 0) {                                   \
                    splay_rotate_left(head, __tmp, field);                     \
                    if (splay_right((head)->sph_root, field) == NULL)          \
                        break;                                                 \
                }                                                              \
                splay_linkright(head, __left, field);                          \
            }                                                                  \
        }                                                                      \
        splay_assemble(head, &__node, __left, __right, field);                 \
    }                                                                          \
                                                                               \
    /* Splay with either the minimum or the maximum element                    \
     * Used to find minimum or maximum element in tree.                        \
     */                                                                        \
    void name##_splay_minmax(struct name * head, int __comp)                   \
    {                                                                          \
        struct type __node;                                                    \
        struct type * __left;                                                  \
        struct type * __right;                                                 \
        struct type * __tmp;                                                   \
                                                                               \
        splay_left(&__node, field) = splay_right(&__node, field) = NULL;       \
        __left = __right = &__node;                                            \
                                                                               \
        while (1) {                                                            \
            if (__comp < 0) {                                                  \
                __tmp = splay_left((head)->sph_root, field);                   \
                if (__tmp == NULL)                                             \
                    break;                                                     \
                if (__comp < 0) {                                              \
                    splay_rotate_right(head, __tmp, field);                    \
                    if (splay_left((head)->sph_root, field) == NULL)           \
                        break;                                                 \
                }                                                              \
                splay_linkleft(head, __right, field);                          \
            } else if (__comp > 0) {                                           \
                __tmp = splay_right((head)->sph_root, field);                  \
                if (__tmp == NULL)                                             \
                    break;                                                     \
                if (__comp > 0) {                                              \
                    splay_rotate_left(head, __tmp, field);                     \
                    if (splay_right((head)->sph_root, field) == NULL)          \
                        break;                                                 \
                }                                                              \
                splay_linkright(head, __left, field);                          \
            }                                                                  \
        }                                                                      \
        splay_assemble(head, &__node, __left, __right, field);                 \
    }

#define splay_neginf -1
#define splay_inf 1

#define splay_insert(name, x, y) name##_splay_insert(x, y)
#define splay_remove(name, x, y) name##_splay_remove(x, y)
#define splay_find(name, x, y) name##_splay_find(x, y)
#define splay_next(name, x, y) name##_splay_next(x, y)
#define splay_prev(name, x, y) name##_splay_prev(x, y)
#define splay_min(name, x)                                                     \
    (splay_empty(x) ? NULL : name##_splay_min_max(x, splay_neginf))
#define splay_max(name, x)                                                     \
    (splay_empty(x) ? NULL : name##_splay_min_max(x, splay_inf))

#define splay_foreach(x, name, head)                                           \
    for ((x) = splay_min(name, head); (x) != NULL;                             \
         (x) = splay_next(name, head, x))

#define splay_foreach_rev(x, name, head)                                       \
    for ((x) = splay_max(name, head); (x) != NULL;                             \
         (x) = splay_prev(name, head, x))


#if 0

/* Macros that define a red-black tree */
#define RB_HEAD(name, type)                                                    \
    struct name {                                                              \
        struct type * rbh_root; /* root of the tree */                         \
    }

#define RB_INITIALIZER(root)                                                   \
    {                                                                          \
        NULL                                                                   \
    }

#define RB_INIT(root)                                                          \
    do {                                                                       \
        (root)->rbh_root = NULL;                                               \
    } while (/*CONSTCOND*/ 0)

#define RB_BLACK 0
#define RB_RED 1
#define RB_ENTRY(type)                                                         \
    struct {                                                                   \
        struct type * rbe_left;   /* left element */                           \
        struct type * rbe_right;  /* right element */                          \
        struct type * rbe_parent; /* parent element */                         \
        int rbe_color;            /* node color */                             \
    }

#define RB_LEFT(elm, field) (elm)->field.rbe_left
#define RB_RIGHT(elm, field) (elm)->field.rbe_right
#define RB_PARENT(elm, field) (elm)->field.rbe_parent
#define RB_COLOR(elm, field) (elm)->field.rbe_color
#define RB_ROOT(head) (head)->rbh_root
#define RB_EMPTY(head) (RB_ROOT(head) == NULL)

#define RB_SET(elm, parent, field)                                             \
    do {                                                                       \
        RB_PARENT(elm, field) = parent;                                        \
        RB_LEFT(elm, field) = RB_RIGHT(elm, field) = NULL;                     \
        RB_COLOR(elm, field) = RB_RED;                                         \
    } while (/*CONSTCOND*/ 0)

#define RB_SET_BLACKRED(black, red, field)                                     \
    do {                                                                       \
        RB_COLOR(black, field) = RB_BLACK;                                     \
        RB_COLOR(red, field) = RB_RED;                                         \
    } while (/*CONSTCOND*/ 0)

#ifndef RB_AUGMENT
#define RB_AUGMENT(x)                                                          \
    do {                                                                       \
    } while (0)
#endif

#define RB_ROTATE_LEFT(head, elm, tmp, field)                                  \
    do {                                                                       \
        (tmp) = RB_RIGHT(elm, field);                                          \
        if ((RB_RIGHT(elm, field) = RB_LEFT(tmp, field)) != NULL) {            \
            RB_PARENT(RB_LEFT(tmp, field), field) = (elm);                     \
        }                                                                      \
        RB_AUGMENT(elm);                                                       \
        if ((RB_PARENT(tmp, field) = RB_PARENT(elm, field)) != NULL) {         \
            if ((elm) == RB_LEFT(RB_PARENT(elm, field), field))                \
                RB_LEFT(RB_PARENT(elm, field), field) = (tmp);                 \
            else                                                               \
                RB_RIGHT(RB_PARENT(elm, field), field) = (tmp);                \
        } else                                                                 \
            (head)->rbh_root = (tmp);                                          \
        RB_LEFT(tmp, field) = (elm);                                           \
        RB_PARENT(elm, field) = (tmp);                                         \
        RB_AUGMENT(tmp);                                                       \
        if ((RB_PARENT(tmp, field)))                                           \
            RB_AUGMENT(RB_PARENT(tmp, field));                                 \
    } while (/*CONSTCOND*/ 0)

#define RB_ROTATE_RIGHT(head, elm, tmp, field)                                 \
    do {                                                                       \
        (tmp) = RB_LEFT(elm, field);                                           \
        if ((RB_LEFT(elm, field) = RB_RIGHT(tmp, field)) != NULL) {            \
            RB_PARENT(RB_RIGHT(tmp, field), field) = (elm);                    \
        }                                                                      \
        RB_AUGMENT(elm);                                                       \
        if ((RB_PARENT(tmp, field) = RB_PARENT(elm, field)) != NULL) {         \
            if ((elm) == RB_LEFT(RB_PARENT(elm, field), field))                \
                RB_LEFT(RB_PARENT(elm, field), field) = (tmp);                 \
            else                                                               \
                RB_RIGHT(RB_PARENT(elm, field), field) = (tmp);                \
        } else                                                                 \
            (head)->rbh_root = (tmp);                                          \
        RB_RIGHT(tmp, field) = (elm);                                          \
        RB_PARENT(elm, field) = (tmp);                                         \
        RB_AUGMENT(tmp);                                                       \
        if ((RB_PARENT(tmp, field)))                                           \
            RB_AUGMENT(RB_PARENT(tmp, field));                                 \
    } while (/*CONSTCOND*/ 0)

/* Generates prototypes and inline functions */
#define RB_PROTOTYPE(name, type, field, cmp)                                   \
    RB_PROTOTYPE_INTERNAL(name, type, field, cmp, )
#define RB_PROTOTYPE_STATIC(name, type, field, cmp)                            \
    RB_PROTOTYPE_INTERNAL(name, type, field, cmp, __unused static)
#define RB_PROTOTYPE_INTERNAL(name, type, field, cmp, attr)                    \
    RB_PROTOTYPE_INSERT_COLOR(name, type, attr);                               \
    RB_PROTOTYPE_REMOVE_COLOR(name, type, attr);                               \
    RB_PROTOTYPE_INSERT(name, type, attr);                                     \
    RB_PROTOTYPE_REMOVE(name, type, attr);                                     \
    RB_PROTOTYPE_FIND(name, type, attr);                                       \
    RB_PROTOTYPE_NFIND(name, type, attr);                                      \
    RB_PROTOTYPE_NEXT(name, type, attr);                                       \
    RB_PROTOTYPE_PREV(name, type, attr);                                       \
    RB_PROTOTYPE_MINMAX(name, type, attr);
#define RB_PROTOTYPE_INSERT_COLOR(name, type, attr)                            \
    attr void name##_RB_INSERT_COLOR(struct name *, struct type *)
#define RB_PROTOTYPE_REMOVE_COLOR(name, type, attr)                            \
    attr void name##_RB_REMOVE_COLOR(struct name *, struct type *,             \
                                     struct type *)
#define RB_PROTOTYPE_REMOVE(name, type, attr)                                  \
    attr struct type * name##_RB_REMOVE(struct name *, struct type *)
#define RB_PROTOTYPE_INSERT(name, type, attr)                                  \
    attr struct type * name##_RB_INSERT(struct name *, struct type *)
#define RB_PROTOTYPE_FIND(name, type, attr)                                    \
    attr struct type * name##_RB_FIND(struct name *, struct type *)
#define RB_PROTOTYPE_NFIND(name, type, attr)                                   \
    attr struct type * name##_RB_NFIND(struct name *, struct type *)
#define RB_PROTOTYPE_NEXT(name, type, attr)                                    \
    attr struct type * name##_RB_NEXT(struct type *)
#define RB_PROTOTYPE_PREV(name, type, attr)                                    \
    attr struct type * name##_RB_PREV(struct type *)
#define RB_PROTOTYPE_MINMAX(name, type, attr)                                  \
    attr struct type * name##_RB_MINMAX(struct name *, int)

/* Main rb operation.
 * Moves node close to the key of elm to top
 */
#define RB_GENERATE(name, type, field, cmp)                                    \
    RB_GENERATE_INTERNAL(name, type, field, cmp, )
#define RB_GENERATE_STATIC(name, type, field, cmp)                             \
    RB_GENERATE_INTERNAL(name, type, field, cmp, __unused static)
#define RB_GENERATE_INTERNAL(name, type, field, cmp, attr)                     \
    RB_GENERATE_INSERT_COLOR(name, type, field, attr)                          \
    RB_GENERATE_REMOVE_COLOR(name, type, field, attr)                          \
    RB_GENERATE_INSERT(name, type, field, cmp, attr)                           \
    RB_GENERATE_REMOVE(name, type, field, attr)                                \
    RB_GENERATE_FIND(name, type, field, cmp, attr)                             \
    RB_GENERATE_NFIND(name, type, field, cmp, attr)                            \
    RB_GENERATE_NEXT(name, type, field, attr)                                  \
    RB_GENERATE_PREV(name, type, field, attr)                                  \
    RB_GENERATE_MINMAX(name, type, field, attr)

#define RB_GENERATE_INSERT_COLOR(name, type, field, attr)                      \
    attr void name##_RB_INSERT_COLOR(struct name * head, struct type * elm)    \
    {                                                                          \
        struct type *parent, *gparent, *tmp;                                   \
        while ((parent = RB_PARENT(elm, field)) != NULL &&                     \
               RB_COLOR(parent, field) == RB_RED) {                            \
            gparent = RB_PARENT(parent, field);                                \
            if (parent == RB_LEFT(gparent, field)) {                           \
                tmp = RB_RIGHT(gparent, field);                                \
                if (tmp && RB_COLOR(tmp, field) == RB_RED) {                   \
                    RB_COLOR(tmp, field) = RB_BLACK;                           \
                    RB_SET_BLACKRED(parent, gparent, field);                   \
                    elm = gparent;                                             \
                    continue;                                                  \
                }                                                              \
                if (RB_RIGHT(parent, field) == elm) {                          \
                    RB_ROTATE_LEFT(head, parent, tmp, field);                  \
                    tmp = parent;                                              \
                    parent = elm;                                              \
                    elm = tmp;                                                 \
                }                                                              \
                RB_SET_BLACKRED(parent, gparent, field);                       \
                RB_ROTATE_RIGHT(head, gparent, tmp, field);                    \
            } else {                                                           \
                tmp = RB_LEFT(gparent, field);                                 \
                if (tmp && RB_COLOR(tmp, field) == RB_RED) {                   \
                    RB_COLOR(tmp, field) = RB_BLACK;                           \
                    RB_SET_BLACKRED(parent, gparent, field);                   \
                    elm = gparent;                                             \
                    continue;                                                  \
                }                                                              \
                if (RB_LEFT(parent, field) == elm) {                           \
                    RB_ROTATE_RIGHT(head, parent, tmp, field);                 \
                    tmp = parent;                                              \
                    parent = elm;                                              \
                    elm = tmp;                                                 \
                }                                                              \
                RB_SET_BLACKRED(parent, gparent, field);                       \
                RB_ROTATE_LEFT(head, gparent, tmp, field);                     \
            }                                                                  \
        }                                                                      \
        RB_COLOR(head->rbh_root, field) = RB_BLACK;                            \
    }

#define RB_GENERATE_REMOVE_COLOR(name, type, field, attr)                      \
    attr void name##_RB_REMOVE_COLOR(struct name * head, struct type * parent, \
                                     struct type * elm)                        \
    {                                                                          \
        struct type * tmp;                                                     \
        while ((elm == NULL || RB_COLOR(elm, field) == RB_BLACK) &&            \
               elm != RB_ROOT(head)) {                                         \
            if (RB_LEFT(parent, field) == elm) {                               \
                tmp = RB_RIGHT(parent, field);                                 \
                if (RB_COLOR(tmp, field) == RB_RED) {                          \
                    RB_SET_BLACKRED(tmp, parent, field);                       \
                    RB_ROTATE_LEFT(head, parent, tmp, field);                  \
                    tmp = RB_RIGHT(parent, field);                             \
                }                                                              \
                if ((RB_LEFT(tmp, field) == NULL ||                            \
                     RB_COLOR(RB_LEFT(tmp, field), field) == RB_BLACK) &&      \
                    (RB_RIGHT(tmp, field) == NULL ||                           \
                     RB_COLOR(RB_RIGHT(tmp, field), field) == RB_BLACK)) {     \
                    RB_COLOR(tmp, field) = RB_RED;                             \
                    elm = parent;                                              \
                    parent = RB_PARENT(elm, field);                            \
                } else {                                                       \
                    if (RB_RIGHT(tmp, field) == NULL ||                        \
                        RB_COLOR(RB_RIGHT(tmp, field), field) == RB_BLACK) {   \
                        struct type * oleft;                                   \
                        if ((oleft = RB_LEFT(tmp, field)) != NULL)             \
                            RB_COLOR(oleft, field) = RB_BLACK;                 \
                        RB_COLOR(tmp, field) = RB_RED;                         \
                        RB_ROTATE_RIGHT(head, tmp, oleft, field);              \
                        tmp = RB_RIGHT(parent, field);                         \
                    }                                                          \
                    RB_COLOR(tmp, field) = RB_COLOR(parent, field);            \
                    RB_COLOR(parent, field) = RB_BLACK;                        \
                    if (RB_RIGHT(tmp, field))                                  \
                        RB_COLOR(RB_RIGHT(tmp, field), field) = RB_BLACK;      \
                    RB_ROTATE_LEFT(head, parent, tmp, field);                  \
                    elm = RB_ROOT(head);                                       \
                    break;                                                     \
                }                                                              \
            } else {                                                           \
                tmp = RB_LEFT(parent, field);                                  \
                if (RB_COLOR(tmp, field) == RB_RED) {                          \
                    RB_SET_BLACKRED(tmp, parent, field);                       \
                    RB_ROTATE_RIGHT(head, parent, tmp, field);                 \
                    tmp = RB_LEFT(parent, field);                              \
                }                                                              \
                if ((RB_LEFT(tmp, field) == NULL ||                            \
                     RB_COLOR(RB_LEFT(tmp, field), field) == RB_BLACK) &&      \
                    (RB_RIGHT(tmp, field) == NULL ||                           \
                     RB_COLOR(RB_RIGHT(tmp, field), field) == RB_BLACK)) {     \
                    RB_COLOR(tmp, field) = RB_RED;                             \
                    elm = parent;                                              \
                    parent = RB_PARENT(elm, field);                            \
                } else {                                                       \
                    if (RB_LEFT(tmp, field) == NULL ||                         \
                        RB_COLOR(RB_LEFT(tmp, field), field) == RB_BLACK) {    \
                        struct type * oright;                                  \
                        if ((oright = RB_RIGHT(tmp, field)) != NULL)           \
                            RB_COLOR(oright, field) = RB_BLACK;                \
                        RB_COLOR(tmp, field) = RB_RED;                         \
                        RB_ROTATE_LEFT(head, tmp, oright, field);              \
                        tmp = RB_LEFT(parent, field);                          \
                    }                                                          \
                    RB_COLOR(tmp, field) = RB_COLOR(parent, field);            \
                    RB_COLOR(parent, field) = RB_BLACK;                        \
                    if (RB_LEFT(tmp, field))                                   \
                        RB_COLOR(RB_LEFT(tmp, field), field) = RB_BLACK;       \
                    RB_ROTATE_RIGHT(head, parent, tmp, field);                 \
                    elm = RB_ROOT(head);                                       \
                    break;                                                     \
                }                                                              \
            }                                                                  \
        }                                                                      \
        if (elm)                                                               \
            RB_COLOR(elm, field) = RB_BLACK;                                   \
    }

#define RB_GENERATE_REMOVE(name, type, field, attr)                            \
    attr struct type * name##_RB_REMOVE(struct name * head, struct type * elm) \
    {                                                                          \
        struct type *child, *parent, *old = elm;                               \
        int color;                                                             \
        if (RB_LEFT(elm, field) == NULL)                                       \
            child = RB_RIGHT(elm, field);                                      \
        else if (RB_RIGHT(elm, field) == NULL)                                 \
            child = RB_LEFT(elm, field);                                       \
        else {                                                                 \
            struct type * left;                                                \
            elm = RB_RIGHT(elm, field);                                        \
            while ((left = RB_LEFT(elm, field)) != NULL)                       \
                elm = left;                                                    \
            child = RB_RIGHT(elm, field);                                      \
            parent = RB_PARENT(elm, field);                                    \
            color = RB_COLOR(elm, field);                                      \
            if (child)                                                         \
                RB_PARENT(child, field) = parent;                              \
            if (parent) {                                                      \
                if (RB_LEFT(parent, field) == elm)                             \
                    RB_LEFT(parent, field) = child;                            \
                else                                                           \
                    RB_RIGHT(parent, field) = child;                           \
                RB_AUGMENT(parent);                                            \
            } else                                                             \
                RB_ROOT(head) = child;                                         \
            if (RB_PARENT(elm, field) == old)                                  \
                parent = elm;                                                  \
            (elm)->field = (old)->field;                                       \
            if (RB_PARENT(old, field)) {                                       \
                if (RB_LEFT(RB_PARENT(old, field), field) == old)              \
                    RB_LEFT(RB_PARENT(old, field), field) = elm;               \
                else                                                           \
                    RB_RIGHT(RB_PARENT(old, field), field) = elm;              \
                RB_AUGMENT(RB_PARENT(old, field));                             \
            } else                                                             \
                RB_ROOT(head) = elm;                                           \
            RB_PARENT(RB_LEFT(old, field), field) = elm;                       \
            if (RB_RIGHT(old, field))                                          \
                RB_PARENT(RB_RIGHT(old, field), field) = elm;                  \
            if (parent) {                                                      \
                left = parent;                                                 \
                do {                                                           \
                    RB_AUGMENT(left);                                          \
                } while ((left = RB_PARENT(left, field)) != NULL);             \
            }                                                                  \
            goto color;                                                        \
        }                                                                      \
        parent = RB_PARENT(elm, field);                                        \
        color = RB_COLOR(elm, field);                                          \
        if (child)                                                             \
            RB_PARENT(child, field) = parent;                                  \
        if (parent) {                                                          \
            if (RB_LEFT(parent, field) == elm)                                 \
                RB_LEFT(parent, field) = child;                                \
            else                                                               \
                RB_RIGHT(parent, field) = child;                               \
            RB_AUGMENT(parent);                                                \
        } else                                                                 \
            RB_ROOT(head) = child;                                             \
    color:                                                                     \
        if (color == RB_BLACK)                                                 \
            name##_RB_REMOVE_COLOR(head, parent, child);                       \
        return (old);                                                          \
    }

#define RB_GENERATE_INSERT(name, type, field, cmp, attr)                       \
    /* Inserts a node into the RB tree */                                      \
    attr struct type * name##_RB_INSERT(struct name * head, struct type * elm) \
    {                                                                          \
        struct type * tmp;                                                     \
        struct type * parent = NULL;                                           \
        int comp = 0;                                                          \
        tmp = RB_ROOT(head);                                                   \
        while (tmp) {                                                          \
            parent = tmp;                                                      \
            comp = (cmp)(elm, parent);                                         \
            if (comp < 0)                                                      \
                tmp = RB_LEFT(tmp, field);                                     \
            else if (comp > 0)                                                 \
                tmp = RB_RIGHT(tmp, field);                                    \
            else                                                               \
                return (tmp);                                                  \
        }                                                                      \
        RB_SET(elm, parent, field);                                            \
        if (parent != NULL) {                                                  \
            if (comp < 0)                                                      \
                RB_LEFT(parent, field) = elm;                                  \
            else                                                               \
                RB_RIGHT(parent, field) = elm;                                 \
            RB_AUGMENT(parent);                                                \
        } else                                                                 \
            RB_ROOT(head) = elm;                                               \
        name##_RB_INSERT_COLOR(head, elm);                                     \
        return (NULL);                                                         \
    }

#define RB_GENERATE_FIND(name, type, field, cmp, attr)                         \
    /* Finds the node with the same key as elm */                              \
    attr struct type * name##_RB_FIND(struct name * head, struct type * elm)   \
    {                                                                          \
        struct type * tmp = RB_ROOT(head);                                     \
        int comp;                                                              \
        while (tmp) {                                                          \
            comp = cmp(elm, tmp);                                              \
            if (comp < 0)                                                      \
                tmp = RB_LEFT(tmp, field);                                     \
            else if (comp > 0)                                                 \
                tmp = RB_RIGHT(tmp, field);                                    \
            else                                                               \
                return (tmp);                                                  \
        }                                                                      \
        return (NULL);                                                         \
    }

#define RB_GENERATE_NFIND(name, type, field, cmp, attr)                        \
    /* Finds the first node greater than or equal to the search key */         \
    attr struct type * name##_RB_NFIND(struct name * head, struct type * elm)  \
    {                                                                          \
        struct type * tmp = RB_ROOT(head);                                     \
        struct type * res = NULL;                                              \
        int comp;                                                              \
        while (tmp) {                                                          \
            comp = cmp(elm, tmp);                                              \
            if (comp < 0) {                                                    \
                res = tmp;                                                     \
                tmp = RB_LEFT(tmp, field);                                     \
            } else if (comp > 0)                                               \
                tmp = RB_RIGHT(tmp, field);                                    \
            else                                                               \
                return (tmp);                                                  \
        }                                                                      \
        return (res);                                                          \
    }

#define RB_GENERATE_NEXT(name, type, field, attr)                              \
    /* ARGSUSED */                                                             \
    attr struct type * name##_RB_NEXT(struct type * elm)                       \
    {                                                                          \
        if (RB_RIGHT(elm, field)) {                                            \
            elm = RB_RIGHT(elm, field);                                        \
            while (RB_LEFT(elm, field))                                        \
                elm = RB_LEFT(elm, field);                                     \
        } else {                                                               \
            if (RB_PARENT(elm, field) &&                                       \
                (elm == RB_LEFT(RB_PARENT(elm, field), field)))                \
                elm = RB_PARENT(elm, field);                                   \
            else {                                                             \
                while (RB_PARENT(elm, field) &&                                \
                       (elm == RB_RIGHT(RB_PARENT(elm, field), field)))        \
                    elm = RB_PARENT(elm, field);                               \
                elm = RB_PARENT(elm, field);                                   \
            }                                                                  \
        }                                                                      \
        return (elm);                                                          \
    }

#define RB_GENERATE_PREV(name, type, field, attr)                              \
    /* ARGSUSED */                                                             \
    attr struct type * name##_RB_PREV(struct type * elm)                       \
    {                                                                          \
        if (RB_LEFT(elm, field)) {                                             \
            elm = RB_LEFT(elm, field);                                         \
            while (RB_RIGHT(elm, field))                                       \
                elm = RB_RIGHT(elm, field);                                    \
        } else {                                                               \
            if (RB_PARENT(elm, field) &&                                       \
                (elm == RB_RIGHT(RB_PARENT(elm, field), field)))               \
                elm = RB_PARENT(elm, field);                                   \
            else {                                                             \
                while (RB_PARENT(elm, field) &&                                \
                       (elm == RB_LEFT(RB_PARENT(elm, field), field)))         \
                    elm = RB_PARENT(elm, field);                               \
                elm = RB_PARENT(elm, field);                                   \
            }                                                                  \
        }                                                                      \
        return (elm);                                                          \
    }

#define RB_GENERATE_MINMAX(name, type, field, attr)                            \
    attr struct type * name##_RB_MINMAX(struct name * head, int val)           \
    {                                                                          \
        struct type * tmp = RB_ROOT(head);                                     \
        struct type * parent = NULL;                                           \
        while (tmp) {                                                          \
            parent = tmp;                                                      \
            if (val < 0)                                                       \
                tmp = RB_LEFT(tmp, field);                                     \
            else                                                               \
                tmp = RB_RIGHT(tmp, field);                                    \
        }                                                                      \
        return (parent);                                                       \
    }

#define RB_NEGINF -1
#define RB_INF 1

#define RB_INSERT(name, x, y) name##_RB_INSERT(x, y)
#define RB_REMOVE(name, x, y) name##_RB_REMOVE(x, y)
#define RB_FIND(name, x, y) name##_RB_FIND(x, y)
#define RB_NFIND(name, x, y) name##_RB_NFIND(x, y)
#define RB_NEXT(name, x, y) name##_RB_NEXT(y)
#define RB_PREV(name, x, y) name##_RB_PREV(y)
#define RB_MIN(name, x) name##_RB_MINMAX(x, RB_NEGINF)
#define RB_MAX(name, x) name##_RB_MINMAX(x, RB_INF)

#define RB_FOREACH(x, name, head)                                              \
    for ((x) = RB_MIN(name, head); (x) != NULL; (x) = name##_RB_NEXT(x))

#define RB_FOREACH_FROM(x, name, y)                                            \
    for ((x) = (y); ((x) != NULL) && ((y) = name##_RB_NEXT(x), (x) != NULL);   \
         (x) = (y))

#define RB_FOREACH_SAFE(x, name, head, y)                                      \
    for ((x) = RB_MIN(name, head);                                             \
         ((x) != NULL) && ((y) = name##_RB_NEXT(x), (x) != NULL); (x) = (y))

#define RB_FOREACH_REVERSE(x, name, head)                                      \
    for ((x) = RB_MAX(name, head); (x) != NULL; (x) = name##_RB_PREV(x))

#define RB_FOREACH_REVERSE_FROM(x, name, y)                                    \
    for ((x) = (y); ((x) != NULL) && ((y) = name##_RB_PREV(x), (x) != NULL);   \
         (x) = (y))

#define RB_FOREACH_REVERSE_SAFE(x, name, head, y)                              \
    for ((x) = RB_MAX(name, head);                                             \
         ((x) != NULL) && ((y) = name##_RB_PREV(x), (x) != NULL); (x) = (y))

#endif
