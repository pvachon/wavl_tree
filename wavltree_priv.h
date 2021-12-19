#pragma once

/** \file wavl_priv.h
 * Implementation details for the WAVL Tree Library.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * A normal WAVL tree result
 */
typedef uint32_t wavl_result_t;

#define WAVL_ERROR_FLAG                 (1ull << 31)
#define WAVL_ERROR_OK                   0
#define WAVL_ERROR_SYS_SHIFT            16
#define WAVL_ERROR_SYS(_x)              (((uint32_t)(_x) & 0x7fff) << WAVL_ERROR_SYS_SHIFT)
#define WAVL_ERROR_CODE(_x)             ((uint32_t)(_x) & 0xffff)

#define WAVL_SYS_CORE                   1 /**< Error in some core part of the system, or state */
#define WAVL_SYS_TREE                   2 /**< Error in the tree handling code */

#define WAVL_RESULT(_err, _sys, _code)  ((_err) | WAVL_ERROR_SYS(_sys) | WAVL_ERROR_CODE(_code))
#define WAVL_ERROR(_sys, _code)         WAVL_RESULT(WAVL_ERROR_FLAG, (_sys), (_code))


#define WAVL_ERR_OK                     0
#define WAVL_ERR_BAD_ARG                WAVL_ERROR(WAVL_SYS_CORE, 0) /**< Bad argument, i.e. unexpected NULL */

#define WAVL_ERR_TREE_DUPE              WAVL_ERROR(WAVL_SYS_TREE, 0)    /**< Item to be inserted is a duplicate */
#define WAVL_ERR_TREE_NOT_FOUND         WAVL_ERROR(WAVL_SYS_TREE, 1)    /**< Item not found in the tree */

/**
 * Predicate to check if result code is OK
 */
#define WAVL_OK(_x)                     (!((_x) & WAVL_ERROR_FLAG))

#define WAVL_FAILED(_xx)                (!WAVL_OK((_xx)))

/**
 * Mark branch unlikely to be taken
 */
#define WAVL_UNLIKELY(_b)               __builtin_expect(!!(_b), 0)

#ifdef DEBUG
#include <stdio.h>

/**
 * Check an argument, return bad arg if it fails the check
 */
#define WAVL_ASSERT_ARG(_wassert)               \
    do {                                        \
        if (!(WAVL_UNLIKELY(_wassert))) {       \
            fprintf(stderr, "WAVL Argument Assertion failure: " \
                #_wassert " (" __FILE__ ":%d)\n", \
                __LINE__);                      \
            return WAVL_ERR_BAD_ARG;            \
        }                                       \
    } while (0)                                 \

/**
 * Assert the truthfulness of a statement. If it is false, return an error.
 */
#define WAVL_ASSERT(_x)                         \
    do {                                        \
        if (!(WAVL_UNLIKELY(_x))) {             \
            fprintf(stderr, "WAVL Assertion Failure: " \
                #_x " (" __FILE__ ":%d)\n", \
                __LINE__);                      \
            abort();                            \
        }                                       \
    } while (0)

#else /* !defined(DEBUG) */
/* Eat the macro under normal circumstances */
#define WAVL_ASSERT(_x)

/**
 * Check an argument, return bad arg if it fails the check
 */
#define WAVL_ASSERT_ARG(_wassert)               \
    do {                                        \
        if (!(WAVL_UNLIKELY(_wassert))) {       \
            return WAVL_ERR_BAD_ARG;            \
        }                                       \
    } while (0)                                 \


#endif /* defined(DEBUG) */

/**
 * Ordering function to compare a node to another node.
 */
typedef wavl_result_t (*wavl_node_to_node_compare_func_t)(struct wavl_tree_node *lhs,
                                                          struct wavl_tree_node *rhs,
                                                          int *pdir);

/**
 * Ordering function to compare a node to a key.
 */
typedef wavl_result_t (*wavl_key_to_node_compare_func_t)(void *key_lhs,
                                                         struct wavl_tree_node *rhs,
                                                         int *pdir);

/**
 * A WAVL-tree node. Embed this in your own structure. All members of this structure
 * are private.
 */
struct wavl_tree_node {
    struct wavl_tree_node *left,    /**< Left-hand child; NULL if not present */
                          *right;   /**< Right-hand child; NULL if not present */
    struct wavl_tree_node *parent;  /**< The parent of this node */
    bool rp;                         /**< Rank parity */
};

/**
 * Create an empty node, through assignment
 */
#define WAVL_TREE_NODE_EMPTY    (struct wavl_tree_node){ .left = NULL, .right = NULL, .parent = NULL, .rp = false}

/**
 * Clear a newly allocated WAVL tree node.
 */
#define WAVL_TREE_NODE_CLEAR(_n) do { (_n)->left = (_n)->right = (_n)->parent = NULL; (_n)->rp = false; } while (0)

/**
 * A WAVL tree. This structure contains all the state needed to maintain a wavl
 * tree. All members of this structure are private, and should not be inspected or
 * modified by users.
 */
struct wavl_tree {
    struct wavl_tree_node *root;                /**< Root of the tree */
    wavl_node_to_node_compare_func_t node_cmp;  /**< Function pointer to compare a node to a node */
    wavl_key_to_node_compare_func_t key_cmp;    /**< Function pointer to compare a key to a node */
};

#ifndef __WAVL_INCLUDING_WAVL_PRIV_H__
#error "Do not include wavl_priv.h directly!"
#endif /* ndef __INCLUDING_WAVL_PRIV_H__ */

