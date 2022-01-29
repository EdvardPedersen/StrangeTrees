# Visualization of optimizing exponentially growing problems using binary trees

## Layout
A binary tree is constructed for partitioning the screen,
each level of the tree splits the screen in alternating horizontal or vertical halves.
Default tree depth = 10 giving squares of 25*25 pixels

## Usage

### Normal
Run the simulation without any flags `'./main'` or `'make run'`.
### Draw tree as pdf
The tree can be plotted using graphviz. Run with `'-d'` flag or `'make drawTree'`

### Animate search for next block
As the units cross boundaries in the screen, they will have to move into a new node in the tree, the search for these nodes can be animated on screen, showing how  one traverse the tree one level at a time. Run with `'-a'`  flag or `'make animatedSearch'`

### Arguments can be passed in any order and together:
``` -a -d // -ad/-da // -a -d```


# Requirements
For tree visualization:
## [Ubuntu]:
sudo apt-get install graphviz