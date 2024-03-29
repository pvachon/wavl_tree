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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>

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
    ptrdiff_t id; /**< For testing purposes, the ID of the node is our key */
    struct wavl_tree_node node;
};

/**
 * Array of test nodes, used in test cases
 */
struct test_node nodes[256];

static
size_t _binary_tree_log_2(size_t count)
{
    return 63U - __builtin_clzll(count);
}

static
struct wavl_tree_node *_binary_tree_find_minimum(struct wavl_tree_node *root)
{
    assert(NULL != root);

    struct wavl_tree_node *cur = root;

    while (NULL != cur->left) {
        cur = cur->left;
    }

    return cur;
}

static
struct wavl_tree_node *_binary_tree_find_successor(struct wavl_tree_node *node)
{
    assert(NULL != node);

    struct wavl_tree_node *cur = node;

    /* If this node has a right-hand tree, then the successor is the minimum of that tree */
    if (NULL == node->right) {
        return _binary_tree_find_minimum(cur);
    }

    return cur;
}

static
bool binary_tree_assert(struct wavl_tree *tree, size_t nr_nodes)
{
    ptrdiff_t ids[nr_nodes];    /* The list of values, in-order, held by the tree */
    struct wavl_tree_node *visit_stack[_binary_tree_log_2(nr_nodes) * 2];
    size_t next_id = 0,
           stack_top = 0;

    struct wavl_tree_node *cur_node = tree->root;

    while (true) {

    }

    return true;
}

static
void wavl_test_dump_tree(struct test_node *start, size_t nr_nodes)
{
    size_t null_cnt = 0;
    fprintf(stderr, "digraph {\n");
    fprintf(stderr, "  node [shape=record];\n");
    for (size_t i = 0; i < nr_nodes; i++) {
        struct test_node *tnode = &start[i];
        struct wavl_tree_node *node = &tnode->node;

        if (NULL == node->parent && NULL == node->left && NULL == node->right) {
            continue;
        }

        struct test_node *parent = TEST_NODE(node->parent);

        if (NULL != node->parent) {
            fprintf(stderr, "  %td [label=\"%td | P = %c | p = %td\"];\n", tnode->id, tnode->id, node->rp == true ? 'T' : 'F', parent->id);
        } else {
            fprintf(stderr, "  %td [label=\"%td | P = %c | NO PARENT\"];\n", tnode->id, tnode->id, node->rp == true ? 'T' : 'F');
        }

        if (NULL == node->left) {
            fprintf(stderr, "  null%zu [shape=point];\n", null_cnt);
            fprintf(stderr, "  %td -> null%zu;\n", tnode->id, null_cnt);
            null_cnt++;
        } else {
            struct test_node *lnode = TEST_NODE(node->left);
            fprintf(stderr, "  %td -> %td;\n", tnode->id, lnode->id);
        }

        if (NULL == node->right) {
            fprintf(stderr, "  null%zu [shape=point];\n", null_cnt);
            fprintf(stderr, "  %td -> null%zu;\n", tnode->id, null_cnt);
            null_cnt++;
        } else {
            struct test_node *rnode = TEST_NODE(node->right);
            fprintf(stderr, "  %td -> %td;\n", tnode->id, rnode->id);
        }
    }
    fprintf(stderr, "}\n");
}

static
wavl_result_t _test_node_compare_func(ptrdiff_t lhs,
                                      ptrdiff_t rhs,
                                      int *pdir)
{
    *pdir = (int)(lhs - rhs);
    return WAVL_ERROR_OK;
}

/**
 * Comparison function (for testing), to compare node-to-node
 */
static
wavl_result_t _test_node_to_node_compare_func(struct wavl_tree *tree,
                                              struct wavl_tree_node *lhs,
                                              struct wavl_tree_node *rhs,
                                              int *pdir)
{
    return _test_node_compare_func(TEST_NODE(lhs)->id, TEST_NODE(rhs)->id, pdir);
}

/**
 * Comparison function (for testing), to compare key-to-node
 */
static
wavl_result_t _test_node_to_value_compare_func(struct wavl_tree *tree,
                                               void *key_lhs,
                                               struct wavl_tree_node *rhs,
                                               int *pdir)
{
    return _test_node_compare_func((ptrdiff_t)key_lhs, TEST_NODE(rhs)->id, pdir);
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
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)(int64_t)nodes[i].id, &nodes[i].node));
        node_id += 1;
        sign = -sign;
    }

    //wavl_test_dump_tree(nodes, sizeof(nodes)/sizeof(struct test_node));

    return true;
}

static
bool wavl_test_delete_leaf_unary_sibling(void)
{
    struct wavl_tree tree;
    int node_id = 0, sign = -1;

    printf("WAVL: Testing deleting a leaf node with unary sibling.\n");

    wavl_test_clear();

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    for (size_t i = 0; i < 16; i++) {
        nodes[i].id = sign * node_id;
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)(int64_t)nodes[i].id, &nodes[i].node));
        node_id += 1;
        sign = -sign;
    }

    /* Remove node 9 from the tree. Node 9 is a 2-child of node 11. */
    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_remove(&tree, &nodes[9].node));

    //wavl_test_dump_tree(nodes, 16);

    return true;
}

/*
 * Test an iteration of the 2,2 leaf rebalance case.
 */
static
bool wavl_test_delete_leaf_leaf_sibling(void)
{
    struct wavl_tree tree;
    int node_id = 0, sign = -1;

    printf("WAVL: Testing deleting a leaf node with a leaf sibling.\n");

    wavl_test_clear();

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    for (size_t i = 0; i < 16; i++) {
        nodes[i].id = sign * node_id;
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)(int64_t)nodes[i].id, &nodes[i].node));
        node_id += 1;
        sign = -sign;
    }

    /* Remove node -14 from the tree. Node -14 is a 1-child of node -12. */
    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_remove(&tree, &nodes[14].node));
    /* Remove node -10 */
    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_remove(&tree, &nodes[10].node));

    //wavl_test_dump_tree(nodes, 16);

    return true;

}

static
bool wavl_test_delete_inner_1(void)
{
    struct wavl_tree tree;
    int node_id = 0, sign = -1;

    printf("WAVL: Testing deleting an inner node.\n");

    wavl_test_clear();

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    for (size_t i = 0; i < 16; i++) {
        nodes[i].id = sign * node_id;
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)(int64_t)nodes[i].id, &nodes[i].node));
        node_id += 1;
        sign = -sign;
    }

    /* Remove node -8 from the tree. Node -8 is a 2-child of node 0. */
    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_remove(&tree, &nodes[8].node));

    //wavl_test_dump_tree(nodes, 16);

    return true;
}

static
bool wavl_test_delete_every_third(void)
{
    struct wavl_tree tree;
    int node_id = 0, sign = -1;
    const size_t nr_nodes = 32; //sizeof(nodes)/sizeof(struct test_node);

    printf("WAVL: Testing sign inverting insertion.\n");

    wavl_test_clear();

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    for (size_t i = 0; i < nr_nodes; i++) {
        nodes[i].id = sign * node_id;
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)(int64_t)nodes[i].id, &nodes[i].node));
        node_id += 1;
        sign = -sign;
    }

    for (size_t i = 2; i < nr_nodes; i += 3) {
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_remove(&tree, &nodes[i].node));
    }

    //wavl_test_dump_tree(nodes, nr_nodes);

    return true;
}

static
bool wavl_test_delete_every_third_then_reinsert(void)
{
    struct wavl_tree tree;
    int node_id = 0, sign = -1;
    const size_t nr_nodes = 32; //sizeof(nodes)/sizeof(struct test_node);

    printf("WAVL: Testing sign inverting insertion.\n");

    wavl_test_clear();

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    for (size_t i = 0; i < nr_nodes; i++) {
        nodes[i].id = sign * node_id;
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)nodes[i].id, &nodes[i].node));
        node_id += 1;
        sign = -sign;
    }

    /* Remove every third node from the tree */
    for (size_t i = 2; i < nr_nodes; i += 3) {
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_remove(&tree, &nodes[i].node));
    }

    /* Re-insert every third node into the tree */
    for (size_t i = 2; i < nr_nodes; i += 3) {
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)nodes[i].id, &nodes[i].node));
    }

    // wavl_test_dump_tree(nodes, nr_nodes);

    return true;
}

bool wavl_test_find(void)
{
    struct wavl_tree tree;
    int node_id = 0, sign = -1;
    const size_t nr_nodes = 32;
    struct wavl_tree_node *found = NULL;
    struct test_node *node = NULL;

    printf("WAVL: Test search and removal.\n");

    wavl_test_clear();

    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_init(&tree, _test_node_to_node_compare_func, _test_node_to_value_compare_func));

    for (size_t i = 0; i < nr_nodes; i++) {
        nodes[i].id = sign * node_id;
        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_insert(&tree, (void *)nodes[i].id, &nodes[i].node));
        node_id += 1;
        sign = -sign;
    }

    /* Try to find a node that definitely doesn't exist */
    WAVL_TEST_ASSERT(WAVL_ERR_TREE_NOT_FOUND == wavl_tree_find(&tree, (void *)(ptrdiff_t)4, &found));

    /* Try to find a node we know exists */
    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_find(&tree, (void *)(ptrdiff_t)-4, &found));
    node = TEST_NODE(found);
    WAVL_TEST_ASSERT(node->id == -4);

    /* Try to remove this node */
    WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_remove(&tree, found));

    /* Now try finding it in the tree, once again */
    WAVL_TEST_ASSERT(WAVL_ERR_TREE_NOT_FOUND == wavl_tree_find(&tree, (void *)(ptrdiff_t)-4, &found));

    return true;
}

#define LFSR_POLY_6B_1 0x36
#define LFSR_POLY_6B_2 0x30

static
uint32_t lfsr_next(uint32_t lfsr, uint32_t poly)
{
    bool fb = !!(lfsr & 1);

    lfsr >>= 1;
    lfsr = !fb ? lfsr : (lfsr ^ poly);

    return lfsr;
}

static
bool wavl_test_pseudorandom_1(void)
{
    uint32_t lfsr = LFSR_POLY_6B_1;
    struct wavl_tree tree;

    wavl_test_clear();

    WAVL_TEST_ASSERT(WAVL_ERR_OK ==
            wavl_tree_init(
                &tree,
                _test_node_to_node_compare_func,
                _test_node_to_value_compare_func));

    printf("Inserting: ");
    for (size_t i = 0; i < 63; i++) {
        printf(" %02x ", (unsigned int)lfsr);
        fflush(stdout);
        nodes[i].id = lfsr;
        WAVL_TEST_ASSERT(WAVL_ERR_OK ==
                wavl_tree_insert(&tree,
                    (void *)nodes[i].id,
                    &nodes[i].node));

        lfsr = lfsr_next(lfsr, LFSR_POLY_6B_1);
    }

    printf("\n");

    printf("Removing: ");
    for (size_t i = 0; i < 63; i++) {
        struct wavl_tree_node *nd = NULL;
        struct test_node *tn = NULL;
        printf(" %02x (%u) ", (unsigned int)lfsr, (unsigned int)lfsr);
        fflush(stdout);

        if (0x15 == lfsr) {
            printf("\n");
            wavl_test_dump_tree(nodes, 63);
            printf("\n");
        }

        WAVL_TEST_ASSERT(WAVL_ERR_OK ==
                wavl_tree_find(&tree,
                    (void *)(ptrdiff_t)lfsr,
                    &nd));
        WAVL_TEST_ASSERT(NULL != nd);
        tn = TEST_NODE(nd);

        WAVL_TEST_ASSERT(tn->id == lfsr);

        WAVL_TEST_ASSERT(WAVL_ERR_OK == wavl_tree_remove(
                    &tree,
                    nd));

        lfsr = lfsr_next(lfsr, LFSR_POLY_6B_2);
    }

    printf("\n");

    wavl_test_dump_tree(nodes, 63);

    WAVL_TEST_ASSERT(tree.root == NULL);

    return true;
}

int main(int argc __attribute__((unused)), const char *argv[])
{
    int ret = EXIT_FAILURE;

    fprintf(stdout, "%s: tester\n", argv[0]);

    wavl_test_init();
    wavl_test_simple_insert();
    wavl_test_sign_invert_insert();
    wavl_test_delete_leaf_leaf_sibling();
    wavl_test_delete_leaf_unary_sibling();
    wavl_test_delete_inner_1();
    wavl_test_delete_every_third();
    wavl_test_delete_every_third_then_reinsert();

    wavl_test_find();

    wavl_test_pseudorandom_1();

    ret = EXIT_SUCCESS;
    return ret;
}

