# `wavltree` - An Intrusive Balanced Binary Search Tree
`wavltree` is a portable, easy-to-use C language balanced binary search tree,
using the rebalancing weak AVL algorithm described in [Haeupler et. al][1].

# What?

The weak AVL (or *wavl* for short) tree is a relaxation of the constraints
enforced on AVL trees. This relaxation permits much less complex rebalancing
of the tree after node removals than what AVL trees enforce, giving a wavl
tree worst case performance similar to a red-black tree for workloads that
include a mix of deletions.

For workloads that are insertion-only, the worst case search time for a wavl
tree is equivalent to that of an AVL tree, with a slightly less complex
set of rebalancing operations.

Not all wavl trees are AVL trees (but those that are insertion-only are),
but interestingly, all AVL trees are valid red-black trees. Red-Black trees,
however, are a superset of wavl trees. Thus, not all red-black trees are
valid wavl trees. How's that for an interesting result?

## Design Considerations

The wavl tree implementation uses an embedded state struct that contains 3
pointers and a boolean value to indicate the rank parity of the current node.

The rank parity could fairly easily be encoded as a high-order bit in one
of these pointers. The groundwork, sort of, has been laid by not having
any of the main algorithms refer to the rank parity field directly, but there
is still some work to be done.

# Dependencies
The `wavltree` library depends only on the C standard library. The code is
written to compile with any C99-capable compiler. If you want to use `wavltree`
as a part of your project, you can simply copy `wavltree.c` into your project,
and put `wavltree.h` and `wavltree_priv.h` wherever you keep your project's
headers.

# Usage
All functions and structures have Doxygen documentation describing members, any
library-level functions and their usage.

# License
The `wavltree` implementation is licensed under a 2-clause BSD-style license.
For more information, please see the `COPYING` file in the project directory.

# Why?

Phil got nerd sniped. :-(

# References
1. Haeupler, Sen, Tarjan, "Rank-Balanced Trees" (see [http://sidsen.azurewebsites.net//papers/rb-trees-talg.pdf])
