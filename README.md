# `wavltree` - An Intrusive Balanced Binary Search Tree
`wavltree` is a portable, easy-to-use C language balanced binary search tree,
using the rebalancing weak AVL algorithm described in [Haeupler et. al][1].

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

# References
1. Haeupler, Sen, Tarjan, "Rank-Balanced Trees" (see [http://sidsen.azurewebsites.net//papers/rb-trees-talg.pdf])
