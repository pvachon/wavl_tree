#pragma once

struct wavl_tree_node;
struct wavl_tree;

#define __WAVL_INCLUDING_WAVL_PRIV_H__
#include "wavltree_priv.h"
#undef __WAVL_INCLUDING_WAVL_PRIV_H__

/**
 * Container-of macro. This is meant to make it easy to get a pointer to
 * a structure containing a `struct wavl_tree_node`.
 * \param pointer Node pointer
 * \param type The target type of the containing structure
 * \param member The name of the member of the containing structure
 *
 * \return Pointer to the structure that contains the node
 */
#define WAVL_CONTAINER_OF(pointer, type, member) \
    ({ __typeof__( ((type *)0)->member ) *__memb = (pointer); \
       (type *)( (char *)__memb - offsetof(type, member) ); })

/**
 * Initialize a new WAVL Tree.
 *
 * \param tree Pointer to memory to be initialized as a new WAVL tree
 * \param node_cmp Pointer to function that performs node-to-node comparisons
 * \param key_cmp Pointer to function that performs key-to-node comparisons
 *
 * \return WAVL_ERR_OK on success, an error code otherwise.
 */
wavl_result_t wavl_tree_init(struct wavl_tree *tree,
                             wavl_node_to_node_compare_func_t node_cmp,
                             wavl_key_to_node_compare_func_t key_cmp);

/**
 * Insert the given item, for the specified key, into the provided WAVL tree.
 *
 * \param tree Pointer to the tree state structure.
 * \param key The key for the item to be inserted. This is assumed to be an attribute
 *            of the structure containing the `struct wavl_tree_node`, and thus storing
 *            the kehy is the responsibility of the user.
 * \param node The `struct wavl_tree_node` that represents an element to be inserted.
 *
 * \return WAVL_ERR_OK on success. If a duplicate node is found, returns WAVL_ERR_TREE_DUPE.
 *
 * \note This function rebalances the WAVL tree automatically.
 */
wavl_result_t wavl_tree_insert(struct wavl_tree *tree,
                               void *key,
                               struct wavl_tree_node *node);

/**
 * Find the given key in the WAVL tree, and return it if present.
 *
 * \param tree Pointer to the tree state structure.
 * \param key The key to search for in the tree.
 * \param pfound The found node. Set to NULL if the node is not found.
 *
 * \return WAVL_ERR_OK when the node is found, WAVL_ERR_TREE_NOT_FOUND if the node is not
 *         found. The actual node is returned by reference in pfound.
 */
wavl_result_t wavl_tree_find(struct wavl_tree *tree,
                             void *key,
                             struct wavl_tree_node **pfound);

/**
 * Remove the specified item directly from the WAVL tree. If you do not already
 * have a reference to the node itself, use `wavl_tree_find` to get a reference
 * to the node.
 *
 * \param tree Pointer to the tree state structure.
 * \param node Pointer to the node to remove from the tree.
 *
 * \return WAVL_ERR_OK on successful removal. Most errors are housekeeping-related.
 *
 * \note This function rebalances the WAVL tree automatically.
 */
wavl_result_t wavl_tree_remove(struct wavl_tree *tree,
                               struct wavl_tree_node *node);

