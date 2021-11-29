#pragma once

struct wavl_tree_node;
struct wavl_tree;

#define __WAVL_INCLUDING_WAVL_PRIV_H__
#include "wavl_priv.h"
#undef __WAVL_INCLUDING_WAVL_PRIV_H__

/**
 * Initialize a new WAVL Tree.
 *
 * \param tree Pointer to memory to be initialized as a new WAVL tree
 * \param node_cmp Pointer to function that performs node-to-node comparisons
 * \param key_cmp Pointer to function that performs key-to-node comparisons
 */
wavl_result_t wavl_tree_init(struct wavl_tree *tree,
                             wavl_node_to_node_compare_func_t node_cmp,
                             wavl_key_to_node_compare_func_t key_cmp);

/**
 * Insert the given item, for the specified key, into the provided WAVL tree.
 */
wavl_result_t wavl_tree_insert(struct wavl_tree *tree,
                               void *key,
                               struct wavl_tree_node *node);

/**
 * Find the given key in the WAVL tree, and return it if present.
 */
wavl_result_t wavl_tree_find(struct wavl_tree *tree,
                             void *key,
                             struct wavl_tree_node **pfound);

/**
 * Remove the specified item directly from the WAVL tree. If you do not already
 * have a reference to the node itself, use `wavl_tree_find` to get a reference
 * to the node.
 */
wavl_result_t wavl_tree_remove(struct wavl_tree *tree,
                               struct wavl_tree_node *node);

