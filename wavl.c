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

    /* Copy Z's rank distance */
    y->rd = z->rd;

    /* Move y's left subtree (since x < left(y)) to x's right subtree */
    x->right = y->left;

    if (NULL != y->left) {
        struct wavl_tree_node *left_y = y->left;
        left_y->parent = x;
    }

    y->left = x;
    x->parent = y;
    x->rd = WAVL_NODE_RANK_DIFF_1;

    /* Set x's right subtree rank difference to 1 */
    if (NULL != x->left) {
        struct wavl_tree_node *left_x = x->left;
        left_x->rd = WAVL_NODE_RANK_DIFF_1;
    }

    /* Move y's right subtree (since z > right(y)) to z's left subtree */
    z->left = y->right;

    if (NULL != y->right) {
        struct wavl_tree_node *right_y = y->right;
        right_y->parent = z;
    }

    y->right = z;
    z->parent = y;
    z->rd = WAVL_NODE_RANK_DIFF_1;

    /* Set z's right subtree rank difference to 1 */
    if (NULL != z->right) {
        struct wavl_tree_node *right_z = z->right;
        right_z->rd = WAVL_NODE_RANK_DIFF_1;
    }
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
                          *p_z = NULL,
                          *left_z = NULL;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != x);

    z = x->parent;
    y = x->right;
    p_z = z->parent;

    WAVL_ASSERT(NULL == y || WAVL_NODE_RANK_DIFF_2 == y->rd);

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

    x->rd = z->rd;
    z->rd = WAVL_NODE_RANK_DIFF_1;

    /* Make y the left-child of z */
    z->left = y;
    if (NULL != y) {
        y->parent = z;

        /* Fix up the rank differences */
        y->rd = WAVL_NODE_RANK_DIFF_1;
    }

    /* left(z) must have a rank of 1 now */
    left_z = z->left;
    left_z->rd = WAVL_NODE_RANK_DIFF_1;
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

    /* Copy Z's rank distance, since this does not change */
    y->rd = z->rd;

    /* Move y's left subtree to z's right subtree (z < right(y)) */
    z->right = y->left;
    if (NULL != y->left) {
        struct wavl_tree_node *left_y = y->left;
        left_y->parent = z;
    }

    y->left = z;
    z->parent = y;
    z->rd = WAVL_NODE_RANK_DIFF_1;

    /* Set z's right subtree rank difference to 1 */
    if (NULL != z->left) {
        struct wavl_tree_node *left_z = z->left;
        left_z->rd = WAVL_NODE_RANK_DIFF_1;
    }

    /* Move y's right subtree to x's left */
    x->right = y->right;
    if (NULL != y->right) {
        struct wavl_tree_node *right_y = y->right;
        right_y->parent = x;
    }

    y->right = x;
    x->parent = y;
    x->rd = WAVL_NODE_RANK_DIFF_1;

    /* Set x's right subtree rank difference to 1 */
    if (NULL != x->right) {
        struct wavl_tree_node *right_x = x->right;
        right_x->rd = WAVL_NODE_RANK_DIFF_1;
    }
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
                          *p_z = NULL,
                          *right_z = NULL;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != x);

    z = x->parent;
    y = x->left;
    p_z = z->parent;

    WAVL_ASSERT(NULL == y || WAVL_NODE_RANK_DIFF_2 == y->rd);

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

    x->rd = z->rd;
    z->rd = WAVL_NODE_RANK_DIFF_1;

    /* Maye y the right-child of z */
    z->right = y;
    if (NULL != y) {
        y->parent = z;
        y->rd = WAVL_NODE_RANK_DIFF_1;
    }

    /* right(z) must also have a rank difference of 1 now */
    right_z = z->right;
    right_z->rd = WAVL_NODE_RANK_DIFF_1;
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
    struct wavl_tree_node *x = NULL;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != at);

    /* At this stage, p_x and x have a rank difference of 0. We need to promote
     * p_x's rank, so the rank difference goes to 1. */

    while (NULL != x->parent) {
        /* For each iteration, the rank difference of x is always 0 */
        struct wavl_tree_node *p_x = x->parent,
                              *s_x = __wavl_tree_node_get_sibling(x);

        /* Check if the parent is 0,2 */
        if (NULL != s_x && s_x->rd == WAVL_NODE_RANK_DIFF_2) {
            if (x == p_x->left) {
                struct wavl_tree_node *y = x->right;

                if (NULL == y || WAVL_NODE_RANK_DIFF_2 == y->rd) {
                    /* Perform a single rotation */
                    _wavl_tree_rotate_right_at(tree, x);
                } else {
                    /* Perform a double right rotation to restore rank rule */
                    _wavl_tree_double_rotate_right_at(tree, x);
                }
            } else {
                struct wavl_tree_node *y = x->left;

                if (NULL == y || WAVL_NODE_RANK_DIFF_2 == y->rd) {
                    /* Perform a single rotation */
                    _wavl_tree_rotate_left_at(tree, x);
                } else {
                    /* Perform a double-left rotation to restore the rank rule */
                    _wavl_tree_double_rotate_left_at(tree, x);
                }
            }
        } else {
            /* We know the current node has to have a rank diff of 1 */
            x->rd = WAVL_NODE_RANK_DIFF_1;

            /* Increase sibling's rank difference, if present */
            if (NULL != s_x) {
                WAVL_ASSERT(s_x->rd == WAVL_NODE_RANK_DIFF_1);
                s_x->rd = WAVL_NODE_RANK_DIFF_2;
            }

            /* Decrease the rank difference of p_x */
            if (p_x->rd == WAVL_NODE_RANK_DIFF_2) {
                p_x->rd = WAVL_NODE_RANK_DIFF_1;
                /* And we're done, everything is balanced per the rank rule */
                break;
            }
        }

        /* Keep iterating, p(x) has a rank difference of 0 atm. */
        x = x->parent;
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
    WAVL_ASSERT_ARG(NULL != key);
    WAVL_ASSERT_ARG(NULL != node);

    node->left = node-> right = NULL;

    node->rd = 0;

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
            break;
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
                                struct wavl_tree_node *parent,
                                struct wavl_tree_node *old,
                                struct wavl_tree_node *new)
{
    struct wavl_tree_node *left = old->left,
                          *right = old->right;

    if (NULL != parent) {
        /* Update the parent to point to the new node */
        if (parent->left == old) {
            parent->left = new;
            new->parent = parent;
        } else {
            parent->right = new;
            new->parent = parent;
        }
    } else {
        /* The old node is at the root */
        tree->root = new;
        new->parent = NULL;
    }

    new->right = right;
    old->right = NULL;

    new->left = left;
    old->left = NULL;

    /* TODO: Grab any state we need from the old node and copy to the new node */
}

/**
 * While insertion and search are pretty straightforward, removal is a bit more complex.
 *
 * We need to effectively consider 3 cases for removal:
 * 1. Where the node has no children (just delete it)
 * 2. Where the node has one child subtree (left or right - promote the child)
 * 3. Where the node has two children subtrees
 *
 * For the third case, we need to find the appropriate replacement for the node we are
 * deleting, which is effectively the successor (next) node to the node we are deleting.
 * We promote the successor to current node's position.
 *
 * The successor, by definition, has to be in the right subtree of the node to be deleted
 * in case 3. We seek a successor on the right branch (g.t. branch
 */
wavl_result_t wavl_tree_remove(struct wavl_tree *tree,
                               struct wavl_tree_node *node)
{
    wavl_result_t ret = WAVL_ERR_OK;

    WAVL_ASSERT_ARG(NULL != tree);
    WAVL_ASSERT_ARG(NULL != node);

    if (NULL == node->left) {
        /* Shift up the right-most node */
    } else if (NULL == node->right) {
        /* Shift up the left-most node */
    } else {
        /* Find the successor */

        struct wavl_tree_node *succ = _wavl_tree_find_minimum_at(node->right);

        if (succ != node->right) {
            /* Replace successor with successor's right tree */
        }

        /* Replace node to delete with successor we found */

    }

    /* WAVL fixups */

    /* Clear the node's metadata out */
    node->left = node->right = NULL;

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
bool wavl_test_insert(void)
{
    struct wavl_tree tree;

    printf("WAVL: Testing simple insertion.\n");

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    return true;
}

int main(int argc __attribute__((unused)), const char *argv[])
{
    int ret = EXIT_FAILURE;

    fprintf(stderr, "%s: tester\n", argv[0]);

    wavl_test_init();
    wavl_test_insert();

    ret = EXIT_SUCCESS;
    return ret;
}
#endif /* defined(__WAVL_TEST__) */

