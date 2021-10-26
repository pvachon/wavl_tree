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
 */
void _wavl_tree_promote_at(struct wavl_tree_node *parent,
                           struct wavl_tree_node *node)
{
    struct wavl_tree_node *sibling = NULL;

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

    struct wavl_tree_node *parent = NULL;

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
    parent = tree->root;

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

int main(int argc __attribute__((unused)), const char *argv[])
{
    int ret = EXIT_FAILURE;

    fprintf(stderr, "%s: tester\n", argv[0]);

    return ret;
}
#endif /* defined(__WAVL_TEST__) */

