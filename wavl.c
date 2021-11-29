#include "wavl.h"

#include <stdlib.h>
#include <stdbool.h>

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
 * Demote the given node's rank.
 */
static inline
void __wavl_tree_node_demote(struct wavl_tree_node *n)
{
    WAVL_ASSERT(NULL != n);

    n->rp = !n->rp;
}

/**
 * Get the given node's parity value
 */
static inline
bool __wavl_tree_get_node_parity(struct wavl_tree_node *n)
{
    return NULL == n ? true : n->rp;
}

/**
 * Perform a double-right rotation of the node x. We could do this using the other
 * rotate primitives, but for the sake of efficiency we will directly implement
 * the rotation.
 */
static
void _wavl_tree_double_rotate_right_at(struct wavl_tree *tree,
                                       struct wavl_tree_node *x)
{
    struct wavl_tree_node *y = NULL,
                          *z = NULL,
                          *p_z = NULL;

    fprintf(stderr, "Double rotate right\n");

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

    /* Promote Y's rank */
    __wavl_tree_node_promote(y);

    /* Move y's left subtree (since x < left(y)) to x's right subtree */
    x->right = y->left;

    if (NULL != y->left) {
        struct wavl_tree_node *left_y = y->left;
        left_y->parent = x;
    }

    y->left = x;
    x->parent = y;
    __wavl_tree_node_demote(x);

    /* Move y's right subtree (since z > right(y)) to z's left subtree */
    z->left = y->right;

    if (NULL != y->right) {
        struct wavl_tree_node *right_y = y->right;
        right_y->parent = z;
    }

    y->right = z;
    z->parent = y;
    __wavl_tree_node_demote(z);
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

    __wavl_tree_node_demote(z);

    /* Make y the left-child of z */
    z->left = y;
    if (NULL != y) {
        y->parent = z;
    }
}

/**
 * Perform a dobule-left rotation of the node x.
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

    fprintf(stderr, "Double rotate left\n");

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

    /* Promote Y's rank */
    __wavl_tree_node_promote(y);

    /* Move y's left subtree to z's right subtree (z < right(y)) */
    z->right = y->left;
    if (NULL != y->left) {
        struct wavl_tree_node *left_y = y->left;
        left_y->parent = z;
    }

    y->left = z;
    z->parent = y;
    __wavl_tree_node_demote(z);

    /* Move y's right subtree to x's left */
    x->right = y->right;
    if (NULL != y->right) {
        struct wavl_tree_node *right_y = y->right;
        right_y->parent = x;
    }

    y->right = x;
    x->parent = y;
    __wavl_tree_node_demote(x);
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

    __wavl_tree_node_demote(z);

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
        par_x = __wavl_tree_get_node_parity(x),
        par_p_x = __wavl_tree_get_node_parity(p_x),
        par_s_x = __wavl_tree_get_node_parity(__wavl_tree_node_get_sibling(x));

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
    if (x == p_x->left) {
        struct wavl_tree_node *y = x->right;

        if (NULL == y || y->rp == par_x) {
            /* If y is NULL or y is 2 distance (parities are equal), do a single rotation */
            _wavl_tree_rotate_right_at(tree, x);
        } else {
            /* Perform a double right rotation to restore rank rule */
            _wavl_tree_double_rotate_right_at(tree, x);
        }
    } else {
        struct wavl_tree_node *y = x->left;

        if (NULL == y || y->rp == par_x) {
            /* Perform a single rotation */
            _wavl_tree_rotate_left_at(tree, x);
        } else {
            /* Perform a double-left rotation to restore the rank rule */
            _wavl_tree_double_rotate_left_at(tree, x);
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

        switch(dir) {
        case -1:
            next = next->left;
            break;
        case 1:
            next = next->right;
            break;
        case 0:
            /* Found the requested node */
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
 * Non-exported function to fin the maximum of the subtree rooted at the specified node.
 */
static
struct wavl_tree_node *_wavl_tree_find_maximum_at(struct wavl_tree_node *node)
{
    struct wavl_tree_node *cur = node;

    while (NULL != cur->right) {
        cur = cur->right;
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
                          *p_x = NULL;

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

    /* TODO: rethink this boundary condition, we should incude a check
     * if the nodes are 3, 1 and just break immediately
     */
    do {
        struct wavl_tree_node *y = __wavl_tree_node_get_sibling(x),
                              *p_p_x = p_x->parent;

        creates_3_node = p_p_x != NULL && p_x->rp == p_p_x->rp;

        /* Figure out which demote case we have */
        if (p_x->rp == y->rp) {
            /* Distance between x_p and y is 2, so just demote r_p */
            __wavl_tree_node_demote(p_x);
        } else {
            if (y->rp == __wavl_tree_get_node_parity(y->left) &&
                    y->rp == __wavl_tree_get_node_parity(y->right))
            {
                /* Y is 2, 2, so we can just demote p_x and y */
                __wavl_tree_node_demote(p_x);
                __wavl_tree_node_demote(y);
            } else {
                /* We are at the point where we need to do some rotations to
                 * restore balance to the tree.
                 */
                break;
            }
        }

        /* Keep climbing the tree */
        x = p_x;
        p_x = p_p_x;
    } while (NULL != p_x && true == creates_3_node);

    if (false == creates_3_node) {
        /* We're done, balance has been restored */
        return;
    }

    /* Otherwise, we need to do some legwork to restore balance */

}

/**
 * Deletion created a 2,2 leaf at leaf. Fix up the leaf, and figure out
 * where that leaves us.
 */
static
void _wavl_tree_delete_rebalance_2_2_leaf(struct wavl_tree *tree,
                                          struct wavl_tree_node *leaf)
{
    struct wavl_tree_node *p_x = NULL,
                          *p_p_x = NULL,
                          *x = leaf;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != leaf);

    p_x = leaf->parent;
    p_p_x = p_x->parent;

    if (p_x->rp == p_p_x->rp) {
        /* p_x is a 2-child, so we will need to kick off the 3,1/1,3 rebalancing */
        __wavl_tree_node_demote(p_x);
        _wavl_tree_delete_rebalance_3_child(tree, p_x, p_p_x);
    } else {
        /* Just demote p_x and carry on (p_x is now a 2-child) */
        __wavl_tree_node_demote(p_x);
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
                          *p_x = NULL,
                          *p_y = NULL;

    bool is_2_child = false,
         was_leaf = false;

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
        was_leaf = x == NULL;
    }

    /* "Promote" the child of the node to be removed*/
    if (NULL != x) {
        x->parent = y->parent;
    }

    p_y = y->parent;
    if (NULL == p_y) {
        /* We're deleting the root, so swap in the new candidate */
        tree->root = x;
    } else {
        /* Check if y is a 2-child of its parent */
        is_2_child = y->rp == p_y->rp;

        /* Ensure the right/left pointer of the parent of the node to
         * be spliced out points to its new child
         */
        if (y == p_y->left) {
            p_y->left = x;
        } else {
            p_y->right = x;
        }
    }

    /*
     * At this point, y is either a node to splice in to replace the node to be deleted, or is
     * the deleted node itself. If y is a node to splice in, do so now.
     */
    if (y != node) {
        _wavl_tree_swap_in_node_at(tree, node, y);
    }

    /*
     * x is the new child we spliced in. p_y is its new parent. Fix up the tree so the wavl
     * constraints are restored.
     *
     * We know we have to restore constraints if:
     *  * y was a 2-child of p_y (note that x is NULL in this case)
     *  * y was a leaf, 1-child of p_y; p_y is unary.
     */
    if (NULL != x->parent) {
        if (true == is_2_child) {
            /* x is a 3-child of p_y, so we need to start handling that caase */
            _wavl_tree_delete_rebalance_3_child(tree, p_y, x);

        } else {
            _wavl_tree_delete_rebalance_2_2_leaf(tree, p_y);
        }
    }

    /* Clear the removed node's metadata out */
    node->left = node->right = node->parent = NULL;
    node->rp = false;

    return ret;
}

#ifdef __WAVL_TEST__
#include <stdio.h>
#include <stdbool.h>

#define WAVL_TEST_ASSERT(_x) \
    do {                                \
        if (!(_x)) {                    \
            fprintf(stderr, "WAVL Test Assertion Failure: " #_x " == FALSE. At " __FILE__ ":%d\n", __LINE__); \
            return false;               \
        }                               \
    } while (0)

#define TEST_NODE(_x) WAVL_CONTAINER_OF((_x), struct test_node, node)

/**
 * Node for testing the WAVL tree out
 */
struct test_node {
    int id; /**< For testing purposes, the ID of the node is our key */
    struct wavl_tree_node node;
};

/**
 * Array of test nodes, used in test cases
 */
struct test_node nodes[256];

static
void wavl_test_dump_tree(struct test_node *start, size_t nr_nodes)
{
    size_t null_cnt = 0;
    fprintf(stderr, "digraph {\n");
    fprintf(stderr, "  node [shape=record];\n");
    for (size_t i = 0; i < nr_nodes; i++) {
        struct test_node *tnode = &start[i];
        struct wavl_tree_node *node = &tnode->node;
        struct test_node *parent = TEST_NODE(node->parent);

        if (NULL != node->parent) {
            fprintf(stderr, "  %d [label=\"%d | P = %c | p = %d\"];\n", tnode->id, tnode->id, node->rp == true ? 'T' : 'F', parent->id);
        } else {
            fprintf(stderr, "  %d [label=\"%d | P = %c | NO PARENT\"];\n", tnode->id, tnode->id, node->rp == true ? 'T' : 'F');
        }

        if (NULL == node->left) {
            fprintf(stderr, "  null%zu [shape=point];\n", null_cnt);
            fprintf(stderr, "  %d -> null%zu;\n", tnode->id, null_cnt);
            null_cnt++;
        } else {
            struct test_node *lnode = TEST_NODE(node->left);
            fprintf(stderr, "  %d -> %d;\n", tnode->id, lnode->id);
        }

        if (NULL == node->right) {
            fprintf(stderr, "  null%zu [shape=point];\n", null_cnt);
            fprintf(stderr, "  %d -> null%zu;\n", tnode->id, null_cnt);
            null_cnt++;
        } else {
            struct test_node *rnode = TEST_NODE(node->right);
            fprintf(stderr, "  %d -> %d;\n", tnode->id, rnode->id);
        }
    }
    fprintf(stderr, "}\n");
}

static
wavl_result_t _test_node_compare_func(int lhs,
                                      int rhs,
                                      int *pdir)
{
    *pdir = lhs - rhs;
    return WAVL_ERROR_OK;
}

/**
 * Comparison function (for testing), to compare node-to-node
 */
static
wavl_result_t _test_node_to_node_compare_func(struct wavl_tree_node *lhs,
                                              struct wavl_tree_node *rhs,
                                              int *pdir)
{
    return _test_node_compare_func(TEST_NODE(lhs)->id, TEST_NODE(rhs)->id, pdir);
}

/**
 * Comparison function (for testing), to compare key-to-node
 */
static
wavl_result_t _test_node_to_value_compare_func(void *key_lhs,
                                               struct wavl_tree_node *rhs,
                                               int *pdir)
{
    return _test_node_compare_func((int)key_lhs, TEST_NODE(rhs)->id, pdir);
}

/**
 * Clear the array of test nodes
 */
void wavl_test_clear(void)
{
    for (size_t i = 0; i < sizeof(nodes)/sizeof(struct test_node); i++) {
        struct test_node *node = &nodes[i];

        node->id = 0;
        WAVL_TREE_NODE_CLEAR(&node->node);
    }
}

static
bool wavl_test_init(void)
{
    struct wavl_tree tree;

    printf("WAVL: Testing initialization.\n");

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    return true;
}

static
bool wavl_test_simple_insert(void)
{
    struct wavl_tree tree;

    printf("WAVL: Testing simple insertion.\n");

    wavl_test_clear();

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    for (size_t i = 0; i < sizeof(nodes)/sizeof(struct test_node); i++) {
        //printf("Inserting node %zu\n", i);
        nodes[i].id = (int)i;
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)(uint64_t)i, &nodes[i].node));
    }

    return true;
}

static
bool wavl_test_sign_invert_insert(void)
{
    struct wavl_tree tree;
    int node_id = 0, sign = -1;

    printf("WAVL: Testing sign inverting insertion.\n");

    wavl_test_clear();

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    for (size_t i = 0; i < sizeof(nodes)/sizeof(struct test_node); i++) {
        nodes[i].id = sign * node_id;
        printf("Inserting node %d\n", nodes[i].id);
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)(int64_t)nodes[i].id, &nodes[i].node));
        node_id += 1;
        sign = -sign;
    }

    wavl_test_dump_tree(nodes, sizeof(nodes)/sizeof(struct test_node));

    return true;
}

int main(int argc __attribute__((unused)), const char *argv[])
{
    int ret = EXIT_FAILURE;

    fprintf(stderr, "%s: tester\n", argv[0]);

    wavl_test_init();
    wavl_test_simple_insert();
    wavl_test_sign_invert_insert();

    ret = EXIT_SUCCESS;
    return ret;
}
#endif /* defined(__WAVL_TEST__) */

