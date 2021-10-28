#include "wavl.h"

#include <stdlib.h>

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
 * Given a node, promote the neighbourhood.
 *
 * This is the first step of rebalancing after insertion.
 */
void _wavl_tree_promote_at(struct wavl_tree_node *parent,
                           struct wavl_tree_node *node)
{
    struct wavl_tree_node *sibling = NULL;

    WAVL_ASSERT(NULL != parent);
    WAVL_ASSERT(NULL != node);

    /* Since the new node is already spliced into the tree, we can simply
     * assume that the node that isn't this node is the sibling node.
     */
    sibling = parent->left == node ? parent->right : parent->left;

    /* Adjust the ranks */
    parent->rank -= 1;
    sibling->rank += 1;
    node->rank = 1;
}

/**
 * Perform a right rotation of the node x.
 *
 * This requires parent(z), z and x. z also is parent(x). y is right(x)
 */
void _wavl_tree_rotate_right_at(struct wavl_tree *tree,
                                struct wavl_tree_node *parent,
                                struct wavl_tree_node *z,
                                struct wavl_tree_node *x)
{
    struct wavl_tree_node *y = NULL;

    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != parent);
    WAVL_ASSERT(NULL != z);
    WAVL_ASSERT(NULL != x);

    y = x->right;


}

void _wavl_tree_rotate_left_at(struct wavl_tree *tree,
                               struct wavl_tree_node *node)
{
    WAVL_ASSERT(NULL != tree);
    WAVL_ASSERT(NULL != node);
}



wavl_result_t wavl_tree_insert(struct wavl_tree *tree,
                               void *key,
                               struct wavl_tree_node *node)
{
    wavl_result_t ret = WAVL_ERR_OK;

    struct wavl_tree_node *parent = NULL,
                          *safe_node = NULL;

    WAVL_ASSERT_ARG(NULL != tree);
    WAVL_ASSERT_ARG(NULL != key);
    WAVL_ASSERT_ARG(NULL != node);

    node->left = node-> right = NULL;

    /* Leaf nodes are always rank 0 */
    node->rank = 0;

    /* Check if this is an empty tree */
    if (NULL == tree->root) {
        /* Put the node in as the root */
        tree->root = node;
        goto done;
    }

    /* Hunt for a candidate leaf to insert this node in */
    parent = safe_node = tree->root;

    while (NULL != parent) {
        int dir = -1;

        if (WAVL_FAILED(ret = tree->key_cmp(key, parent, &dir))) {
            goto done;
        }

        if (dir < 0) {
            if (NULL == parent->left) {
                /* Stitch in the node and break */
                parent->left = node;
                break;
            } else {
                parent = parent->left;
            }
            break;
        } else if (dir > 0) {
            if (NULL == parent->right) {
                /* Stitch in the node and break */
                parent->right = node;
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

    /* TODO: Rebalance after insertion */

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
        } else {
            parent->right = new;
        }
    } else {
        /* The old node is at the root */
        tree->root = new;
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
bool wavl_test_promote(void)
{
    struct test_node x,
                     x_p,
                     y;


    struct wavl_tree_node *nd_x = &x.node,
                          *nd_x_p = &x_p.node,
                          *nd_y = &y.node;

    printf("WAVL: Testing promotion.\n");

    WAVL_TREE_NODE_CLEAR(nd_x);
    WAVL_TREE_NODE_CLEAR(nd_x_p);
    WAVL_TREE_NODE_CLEAR(nd_y);

    nd_x->rank = 0;
    nd_x_p->left = nd_x;
    nd_x_p->rank = 2;
    nd_x_p->right = nd_y;
    nd_y->rank = 1;

    _wavl_tree_promote_at(nd_x_p, nd_x);

    return true;
}


int main(int argc __attribute__((unused)), const char *argv[])
{
    int ret = EXIT_FAILURE;

    fprintf(stderr, "%s: tester\n", argv[0]);

    wavl_test_init();
    wavl_test_promote();

    ret = EXIT_SUCCESS;
    return ret;
}
#endif /* defined(__WAVL_TEST__) */

