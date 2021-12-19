/*
 * Copyright (c) 2021, Phil Vachon <phil@security-embedded.com>>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "wavltree.h"

#include <stdlib.h>
#include <stdbool.h>

#ifdef __WAVL_TEST__
#include <stdio.h>
#define WAVL_DEBUG_OUT(_s, ...) \
    do { fprintf(stdout,                            \
        "DEBUG: " _s " (" __FILE__ ":%d @ %s)\n",   \
        ##__VA_ARGS__, __LINE__, __FUNCTION__);     \
    } while (0)
#else
#define WAVL_DEBUG_OUT(...)
#endif

wavl_result_t wavl_tree_init(struct wavl_tree *tree,
                             wavl_node_to_node_compare_func_t node_cmp,
                             wavl_key_to_node_compare_func_t key_cmp)
{
    wavl_result_t ret = WAVL_ERR_OK;

    WAVL_ASSERT_ARG(NULL != tree);
    WAVL_ASSERT_ARG(NULL != node_cmp);
    WAVL_ASSERT_ARG(NULL != key_cmp);

    tree->root = NULL;
    tree->node_cmp = node_cmp;
    tree->key_cmp = key_cmp;

    return ret;
}

/**
 * Promote the given node's rank.
 */
static inline
void __wavl_tree_node_promote(struct wavl_tree_node *n)
{
    WAVL_ASSERT(NULL != n);

    n->rp = !n->rp;
}

/**
 * Promote the given node's rank, twice.
 */
static inline
void __wavl_tree_node_double_promote(struct wavl_tree_node *n)
{
    WAVL_ASSERT(NULL != n);
}

/**
 * Demote the given node's rank.
 */
static inline
void __wavl_tree_node_demote(struct wavl_tree_node *n)
{
    WAVL_ASSERT(NULL != n);

    n->rp = !n->rp;
}

/**
 * Demote the given node's rank, twice.
 */
static inline
void __wavl_tree_node_double_demote(struct wavl_tree_node *n)
{
    WAVL_ASSERT(NULL != n);
}

/**
 * Get the given node's parity value
 */
static inline
bool __wavl_tree_node_get_parity(struct wavl_tree_node *n)
{
    return NULL == n ? true : n->rp;
}

/**
 * Check if a node is a 2-child.
 */
static inline
bool __wavl_tree_node_is_2_child(struct wavl_tree_node *n, struct wavl_tree_node *p_n)
{
    return __wavl_tree_node_get_parity(n) == __wavl_tree_node_get_parity(p_n);
}

/**
 * Return whether or not the given node is a leaf
 */
static inline
bool __wavl_tree_node_is_leaf(struct wavl_tree_node *n)
{
    return NULL == n->left && NULL == n->right;
}

/**
 * Perform a double-right rotation of the node x. We could do this using the other
 * rotate primitives, but for the sake of efficiency we will directly implement
 * the rotation.
 *
 * Note that this function only performs the tree restructuring. Rank adjustments
 * must be handled by the caller.
 */
static
void _wavl_tree_double_rotate_right_at(struct wavl_tree *tree,
                                       struct wavl_tree_node *x)
{
    struct wavl_tree_node *y = NULL,
                          *z = NULL,
                          *p_z = NULL;

    WAVL_DEBUG_OUT("-->Double rotate right Tree %p node %p", tree, x);

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != x);

    y = x->right;
    z = x->parent;
    p_z = z->parent;

    /* Rotate Y into place */
    y->parent = p_z;
    if (NULL != p_z) {
        if (z == p_z->left) {
            p_z->left = y;
        } else {
            p_z->right = y;
        }
    } else {
        tree->root = y;
    }

    /* Move y's left subtree (since x < left(y)) to x's right subtree */
    x->right = y->left;

    if (NULL != y->left) {
        struct wavl_tree_node *left_y = y->left;
        left_y->parent = x;
    }

    y->left = x;
    x->parent = y;

    /* Move y's right subtree (since z > right(y)) to z's left subtree */
    z->left = y->right;

    if (NULL != y->right) {
        struct wavl_tree_node *right_y = y->right;
        right_y->parent = z;
    }

    y->right = z;
    z->parent = y;
}

/**
 * Perform a single right rotation of the node x.
 *
 * Rotate x into the position of its parent (z), and demote its parent.
 *
 * \param tree The tree. This is updated if z is the root of the tree.
 * \param x The node to rotate into place.
 *
 * This function is invoked as part of rebalancing.
 */
static
void _wavl_tree_rotate_right_at(struct wavl_tree *tree,
                                struct wavl_tree_node *x)
{
    struct wavl_tree_node *y = NULL,
                          *z = NULL,
                          *p_z = NULL;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != x);

    z = x->parent;
    y = x->right;
    p_z = z->parent;

    /* Rotate X into place */
    x->parent = p_z;
    if (NULL != p_z) {
        if (p_z->left == z) {
            p_z->left = x;
        } else {
            p_z->right = x;
        }
    } else {
        tree->root = x;
    }

    /* Make z the right-child of x */
    x->right = z;
    z->parent = x;

    /* Make y the left-child of z */
    z->left = y;
    if (NULL != y) {
        y->parent = z;
    }
}

/**
 * Perform a dobule-left rotation of the node x.
 *
 * Note that this function performs a double rotate restructuring, but
 * does not update the ranks. Updating the ranks is up to the caller.
 */
static
void _wavl_tree_double_rotate_left_at(struct wavl_tree *tree,
                                      struct wavl_tree_node *x)
{
    struct wavl_tree_node *y = NULL,
                          *z = NULL,
                          *p_z = NULL;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != x);

    WAVL_DEBUG_OUT("--> Double rotate left (tree %p node %p)", tree, x);

    y = x->left;
    z = x->parent;
    p_z = z->parent;

    /* Splice Y into its new position */
    y->parent = p_z;
    if (NULL != p_z) {
        if (z == p_z->left) {
            p_z->left = y;
        } else {
            p_z->right = y;
        }
    } else {
        tree->root = y;
    }

    /* Move y's left subtree to z's right subtree (z < right(y)) */
    z->right = y->left;
    if (NULL != y->left) {
        struct wavl_tree_node *left_y = y->left;
        left_y->parent = z;
    }

    y->left = z;
    z->parent = y;

    /* Move y's right subtree to x's left */
    x->right = y->right;
    if (NULL != y->right) {
        struct wavl_tree_node *right_y = y->right;
        right_y->parent = x;
    }

    y->right = x;
    x->parent = y;
}

/**
 * Perform a single left rotation about the node x.
 *
 */
static
void _wavl_tree_rotate_left_at(struct wavl_tree *tree,
                               struct wavl_tree_node *x)
{
    struct wavl_tree_node *y = NULL,
                          *z = NULL,
                          *p_z = NULL;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != x);

    z = x->parent;
    y = x->left;
    p_z = z->parent;

    /* Rotate X into its new place */
    x->parent = p_z;
    if (NULL != p_z) {
        if (p_z->left == z) {
            p_z->left = x;
        } else {
            p_z->right = x;
        }
    } else {
        tree->root = x;
    }

    /* make z the left-child of x */
    x->left = z;
    z->parent = x;

    /* Maye y the right-child of z */
    z->right = y;
    if (NULL != y) {
        y->parent = z;
    }
}

static
struct wavl_tree_node *__wavl_tree_node_get_sibling(struct wavl_tree_node *node)
{
    struct wavl_tree_node *p_node = NULL;

    WAVL_ASSERT(NULL != node);

    p_node = node->parent;

    if (NULL == p_node) {
        return NULL;
    }

    return p_node->left == node ? p_node->right : p_node->left;
}

static
void _wavl_tree_insert_rebalance(struct wavl_tree *tree,
                                 struct wavl_tree_node *at)
{
    struct wavl_tree_node *x = at,
                          *p_x = NULL;
    bool par_x,
         par_p_x,
         par_s_x;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != at);

    p_x = x->parent;

    do {
        /* Promote the current parent */
        __wavl_tree_node_promote(p_x);

        x = p_x;
        p_x = x->parent;

        if (NULL == p_x) {
            /* We made it to the root of the tree, terminate */
            return;
        }

        /* Get the parities of the next node */
        par_x = __wavl_tree_node_get_parity(x),
        par_p_x = __wavl_tree_node_get_parity(p_x),
        par_s_x = __wavl_tree_node_get_parity(__wavl_tree_node_get_sibling(x));

        /* Continue iteration iff p(x) is 1,0 or 0,1 */
    } while  ((!par_x && !par_p_x &&  par_s_x) ||
              ( par_x &&  par_p_x && !par_s_x));

    /* If p(x) isn't 2,0 or 0,2, the rank rule has been restored */
    if (!(( par_x &&  par_p_x &&  par_s_x) ||
          (!par_x && !par_p_x && !par_s_x)))
    {
        /* We're done, return */
        return;
    }

    /* p(x) is 2,0 or 0,2. Determine the type of rotation needed to restore the
     * rank rule.
     */
    struct wavl_tree_node *z = x->parent;
    if (x == p_x->left) {
        struct wavl_tree_node *y = x->right;

        if (NULL == y || __wavl_tree_node_get_parity(y) == par_x) {
            /* If y is NULL or y is 2 distance (parities are equal), do a single rotation */
            _wavl_tree_rotate_right_at(tree, x);
            if (NULL != z) {
                __wavl_tree_node_demote(z);
            }
        } else {
            /* Perform a double right rotation to restore rank rule */
            _wavl_tree_double_rotate_right_at(tree, x);
            __wavl_tree_node_promote(y);
            __wavl_tree_node_demote(x);
            if (NULL != z) {
                __wavl_tree_node_demote(z);
            }
        }
    } else {
        struct wavl_tree_node *y = x->left;

        if (NULL == y || __wavl_tree_node_get_parity(y) == par_x) {
            /* Perform a single rotation */
            _wavl_tree_rotate_left_at(tree, x);
            if (NULL != z) {
                __wavl_tree_node_demote(z);
            }
        } else {
            /* Perform a double-left rotation to restore the rank rule */
            _wavl_tree_double_rotate_left_at(tree, x);
            __wavl_tree_node_promote(y);
            __wavl_tree_node_demote(x);
            if (NULL != z) {
                __wavl_tree_node_demote(z);
            }
        }
    }

}


wavl_result_t wavl_tree_insert(struct wavl_tree *tree,
                               void *key,
                               struct wavl_tree_node *node)
{
    wavl_result_t ret = WAVL_ERR_OK;

    struct wavl_tree_node *parent = NULL;

    bool was_leaf = false;

    WAVL_ASSERT_ARG(NULL != tree);
    WAVL_ASSERT_ARG(NULL != node);

    node->left = node-> right = NULL;

    /* Set initial rank parity (freshly inserted nodes are 0-children) */
    node->rp = false;

    /* Check if this is an empty tree */
    if (NULL == tree->root) {
        /* Put the node in as the root */
        tree->root = node;
        goto done;
    }

    /* Hunt for a candidate leaf to insert this node in */
    parent = tree->root;

    while (NULL != parent) {
        int dir = -1;

        if (WAVL_FAILED(ret = tree->key_cmp(key, parent, &dir))) {
            goto done;
        }

        if (dir < 0) {
            if (NULL == parent->left) {
                was_leaf = parent->left == NULL && parent->right == NULL;
                /* Stitch in the node and break */
                parent->left = node;
                node->parent = parent;
                break;
            } else {
                parent = parent->left;
            }
        } else if (dir > 0) {
            if (NULL == parent->right) {
                was_leaf = parent->left == NULL && parent->right == NULL;
                /* Stitch in the node and break */
                parent->right = node;
                node->parent = parent;
                break;
            } else {
                parent = parent->right;
            }
        } else {
            /* Leave the tree unchanged - this node is a duplicate */
            ret = WAVL_ERR_TREE_DUPE;
            goto done;
        }
    }

    /* Rebalance after insertion */
    if (true == was_leaf) {
        /* We just made a leaf into a unary node, we need to rebalance now */
        _wavl_tree_insert_rebalance(tree, node);
    }

done:
    return ret;
}

wavl_result_t wavl_tree_find(struct wavl_tree *tree,
                             void *key,
                             struct wavl_tree_node **pfound)
{
    wavl_result_t ret = WAVL_ERR_OK;

    struct wavl_tree_node *next = NULL;

    WAVL_ASSERT_ARG(NULL != tree);
    WAVL_ASSERT_ARG(NULL != key);
    WAVL_ASSERT_ARG(NULL != pfound);

    *pfound = NULL;

    next = tree->root;

    while (NULL != next) {
        int dir = -1;

        if (WAVL_FAILED(ret = tree->key_cmp(key, next, &dir))) {
            goto done;
        }

        if (dir < 0) {
            next = next->left;
        } else if (dir > 0) {
            next = next->right;
        } else {
            *pfound = next;
            goto done;
        }
    }

    ret = WAVL_ERR_TREE_NOT_FOUND;

done:
    return ret;
}

/**
 * Non-exported function to find the minimum of the subtree rooted at the specified node.
 */
static
struct wavl_tree_node *_wavl_tree_find_minimum_at(struct wavl_tree_node *node)
{
    struct wavl_tree_node *cur = node;

    while (NULL != cur->left) {
        cur = cur->left;
    }

    return cur;
}

/**
 * Swap the new node in for the old node, effectively splicing in the new node.
 *
 * \param tree The tree
 * \param parent The parent of the old node/to be parent of new node
 * \param old The old node (to be removed)
 * \param new The new node (to be swapped in)
 */
static
void _wavl_tree_swap_in_node_at(struct wavl_tree *tree,
                                struct wavl_tree_node *old,
                                struct wavl_tree_node *new)
{
    struct wavl_tree_node *left = old->left,
                          *right = old->right,
                          *parent = old->parent;;

    new->parent = parent;
    if (NULL != parent) {
        /* Update the parent to point to the new node */
        if (parent->left == old) {
            parent->left = new;
        } else {
            parent->right = new;
        }
    } else {
        /* The old node is at the root */
        tree->root = new;
    }

    new->right = right;
    if (NULL != new->right) {
        struct wavl_tree_node *right_new = new->right;
        right_new->parent = new;
    }
    old->right = NULL;

    new->left = left;
    if (NULL != new->left) {
        struct wavl_tree_node *left_new = new->left;
        left_new->parent = new;
    }
    old->left = NULL;

    /* Swap in the parity of the old node in to the new node */
    new->rp = old->rp;

    old->parent = NULL;
}

/**
 * Rebalance the tree after removing the node, where p_x is the parent of the removed node,
 * and x is the node that was promoted or spliced in.
 *
 * Handle the specific case where a 2 child was removed, resulting in a 3-child.
 *
 * \param tree The tree we are updating
 * \param p_n The parent of the removed node.
 * \param n The node that replaced the removed node.
 *
 * Note that the rank difference between n and p_n is 3 at entry to this function.
 */
static
void _wavl_tree_delete_rebalance_3_child(struct wavl_tree *tree,
                                         struct wavl_tree_node *p_n,
                                         struct wavl_tree_node *n)
{
    struct wavl_tree_node *x = NULL,
                          *p_x = NULL,
                          *y = NULL;
    bool creates_3_node = false,
         done = true;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != p_n);

    /* Start with rebalancing X */
    x = n;
    p_x = p_n;

    /*
     * By the conventions in the paper:
     * - x is a 3-child of p_x
     * - p_x is the parent that we might need in rebalancing
     * - y is the sibling of x
     */

    do {
        struct wavl_tree_node *p_p_x = p_x->parent;
        y = p_x->left == x ? p_x->right : p_x->left;

        /* Check if the next step will create a 3-node of P(x) */
        creates_3_node = p_p_x != NULL &&
            __wavl_tree_node_get_parity(p_x) == __wavl_tree_node_get_parity(p_p_x);

        /* Figure out which demote case we have */
        if (__wavl_tree_node_is_2_child(y, p_x)) {
            /* y is a 2-child, x is a 3 child, simply demote the parent */
            __wavl_tree_node_demote(p_x);
        } else {
            bool y_rank_parity = __wavl_tree_node_get_parity(y);
            if (y_rank_parity == __wavl_tree_node_get_parity(y->left) &&
                y_rank_parity == __wavl_tree_node_get_parity(y->right))
            {
                /* p_x is 3,1 Y (a 1-child) is 2, 2, so we can demote p_x and y */
                __wavl_tree_node_demote(p_x);
                __wavl_tree_node_demote(y);
            } else {
                done = false;
                break;
            }
        }

        /* Keep climbing the tree */
        x = p_x;
        p_x = p_p_x;
    } while (NULL != p_x && true == creates_3_node);

    if (true == done) {
        /* We're done, there should be no more violations of the WAVL constraint */
        return;
    }

    /* Otherwise, perform rotations needed to restore balance */
    struct wavl_tree_node *z = p_x;
    if (x == p_x->left) {
        struct wavl_tree_node *w = y->right;
        if (__wavl_tree_node_get_parity(w) != __wavl_tree_node_get_parity(y)) {
            /* w is a 1-child of y */
            _wavl_tree_rotate_left_at(tree, y);
            __wavl_tree_node_promote(y);
            __wavl_tree_node_demote(z);
            if (__wavl_tree_node_is_leaf(z)) {
                __wavl_tree_node_demote(z);
            }
        } else {
            struct wavl_tree_node *v = y->left;
            /* w is a 2-child of y, thus v must be a 1-child */
            WAVL_ASSERT(__wavl_tree_node_get_parity(y) != __wavl_tree_node_get_parity(v));

            _wavl_tree_double_rotate_left_at(tree, v);
            __wavl_tree_node_double_promote(v);
            __wavl_tree_node_demote(y);
            __wavl_tree_node_double_demote(z);
        }
    } else {
        struct wavl_tree_node *w = y->left;
        if (__wavl_tree_node_get_parity(w) != __wavl_tree_node_get_parity(y)) {
            /* w is a 1-child of y */
            _wavl_tree_rotate_right_at(tree, y);
            __wavl_tree_node_promote(y);
            __wavl_tree_node_demote(z);
            if (__wavl_tree_node_is_leaf(z)) {
                __wavl_tree_node_demote(z);
            }
        } else {
            struct wavl_tree_node *v = y->right;
            WAVL_ASSERT(__wavl_tree_node_get_parity(y) != __wavl_tree_node_get_parity(v));

            _wavl_tree_double_rotate_right_at(tree, v);
            __wavl_tree_node_double_promote(v);
            __wavl_tree_node_demote(y);
            __wavl_tree_node_double_demote(z);
        }
    }
}

/**
 * Deletion created a 2,2 leaf at leaf. Fix up the leaf, and figure out
 * where that leaves us.
 */
static
void _wavl_tree_delete_rebalance_2_2_leaf(struct wavl_tree *tree,
                                          struct wavl_tree_node *leaf)
{
    struct wavl_tree_node *x = leaf;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != leaf);

    /* Check if x is a 2-child of P(x) */
    if (__wavl_tree_node_get_parity(x->parent) == __wavl_tree_node_get_parity(x)) {
        /* The leaf was a 2-child, so we will need to kick off the 3,1/1,3 rebalancing */
        __wavl_tree_node_demote(x);

        /* p_x is now a 3-child, so we need to proceed with a normal 3-child rebalance */
        _wavl_tree_delete_rebalance_3_child(tree, x, x->parent);
    } else {
        /* Just demote the leaf and carry on (leaf is now a 2-child) */
        __wavl_tree_node_demote(x);
    }
}


/**
 * Remove the given node from the tree.
 *
 * While insertion and search are pretty straightforward, removal is a bit more work.
 *
 * We need to consider 3 cases during removal from a binary search tree
 * 1. Where the node has no children (just delete it) - a leaf node
 * 2. Where the node has one child subtree (left or right - promote the child) - unary node
 * 3. Where the node has two children - binary node
 *
 * For the third case, we need to find the appropriate replacement for the node we are
 * deleting, which is effectively the successor (next) node to the node we are deleting.
 * We promote the successor to current node's position. The successor is usually a unary
 * node or a leaf.
 *
 * When we splice in the replacement node in case 3, we need to track where we removed the
 * successor node from. The spliced in node takes on the exact same rank as the removed
 * node.
 *
 * Once we have a candidate node, we have to start rebalancing. To start the rebalancing
 * process off, we need to consider, if the node removed:
 *  * was a leaf, and 1-child of a unary node, we have a 2,2 leaf to fix
 *  * was a 2-child of a node
 */
wavl_result_t wavl_tree_remove(struct wavl_tree *tree,
                               struct wavl_tree_node *node)
{
    wavl_result_t ret = WAVL_ERR_OK;

    struct wavl_tree_node *y = NULL,
                          *x = NULL,
                          *p_y = NULL;

    bool is_2_child = false;

    WAVL_ASSERT_ARG(NULL != tree);
    WAVL_ASSERT_ARG(NULL != node);

    /* Figure out which node we need to splice in, replacing node */
    if (NULL == node->left || NULL == node->right) {
        y = node;
    } else {
        /* Find the minimum of the right subtree of the node to be deleted, to
         * find the replacement for node. We'll need to fix up the tree at the
         * original location of y.
         */
        y = _wavl_tree_find_minimum_at(node->right);
    }

    /* Find the child of the node to splice we will move up */
    if (NULL != y->left) {
        x = y->left;
    } else {
        x = y->right;
    }

    /* Splice in the child of the node to be removed */
    if (NULL != x) {
        x->parent = y->parent;
    }

    p_y = y->parent;
    if (NULL == p_y) {
        /* We're deleting the root, so swap in the new candidate */
        tree->root = x;
    } else {
        /* Check if y is a 2-child of its parent */
        is_2_child = __wavl_tree_node_is_2_child(y, p_y);

        /* Ensure the right/left pointer of the parent of the node to
         * be spliced out points to its new child
         */
        if (y == p_y->left) {
            p_y->left = x;
        } else {
            WAVL_ASSERT(p_y->right == y);
            p_y->right = x;
        }
    }

    /*
     * At this point, y is either a node to splice in to replace the node to be deleted, or is
     * the deleted node itself. If y is a node to splice in, do so now.
     */
    if (y != node) {
        _wavl_tree_swap_in_node_at(tree, node, y);
        if (node == p_y) {
            p_y = y;
        }
    }

    /*
     * x is the new child we spliced in. p_y is its new parent. Fix up the tree so the wavl
     * constraints are restored.
     *
     * We know we have to restore constraints if:
     *  * y was a 2-child of p_y (note that x is NULL in this case)
     *  * y was a leaf, 1-child of p_y; p_y is unary.
     */
    if (NULL != p_y) {
        if (true == is_2_child) {
            /* x is a 3-child of p_y, so we need to start handling that caase */
            _wavl_tree_delete_rebalance_3_child(tree, p_y, x);

        } else if (NULL == x && p_y->left == p_y->right) {
            /* p_y is a 2,2 leaf, so we need to fix it up and figure out if this
             * will result in p_y becoming a 3-child
             */
            _wavl_tree_delete_rebalance_2_2_leaf(tree, p_y);
        }

        /* Ensure the parent is not a leaf, and that the parity isn't true */
        WAVL_ASSERT(!(__wavl_tree_node_is_leaf(p_y) && __wavl_tree_node_get_parity(p_y)));
    }

    /* Clear the removed node's metadata out */
    node->left = node->right = node->parent = NULL;
    node->rp = false;

    return ret;
}

