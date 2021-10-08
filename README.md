# Libretro-script

This is a middleware layer intended to lie between a libretro frontend and libretro implementation.

It allows executing scripts that modify the behaviour of or extract information from a running instance. For example, a script might cause enemies to spawn in a particular NES game.

# Note for Developers

- malloc is used, so this might not be ideal for some old hardware. (TODO: implement an alternative to malloc..?)
- active scripts are stored in a linkedlist, so some operations go as O(n) with number of scripts.