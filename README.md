# Jane Street puzzle, October 2026: *It's a Metric, Too*

**Answer: 408951.**

An 11-by-11 grid has to be cut into symmetric "states". A state's capitol is the
square left fixed by all of its symmetries, when there is exactly one such square.
Moving between adjacent squares costs the size of the state, or the smaller of the
two sizes across a border, and each clue gives the cheapest route to any capitol.
The answer is the sum of the squares of the row sums, once every cell is labelled
with the size of its state.

Puzzle statement: [It's a Metric, Too](https://www.janestreet.com/puzzles/it-s-a-metric-too-index/)

## The solution

Each number is the size of the state containing that cell. A star marks a capitol.

```
  4   4   4   4   7  10   5   5   5   5   5
  7   7   7*  7   7  10  10  10   7   7*  7
  7  37  37  37  37  10   1* 10   7  12   7
 11  37  37  37  37  10  10  10   7  12   7
 11  11  37  37  37  37   5  10   2  12  12
 11  37  37  37  37  37   5   5*  2  12   1*
 11  11* 11   2  37  37* 37   5   5  12  12
 10  10  11   2   4  37  37  37  37  37  12
 10  11  11   4   4  37  37  37  37  12  12
 10  10  11  10   4   3  37  37  37  37  12
 10  10  10  10   3   3* 37  37  37  37  12
```

Row sums: 58, 86, 202, 215, 211, 221, 180, 234, 212, 208, 206.

## Structure

- A 37-cell diagonal state runs from (2,1) to (10,9), with its capitol at (6,5).
  Coordinates are (row, column) from zero. It explains the largest clues:
  51 = 14 + 37 and 77 = 3 + 37 + 37.
- The 12-cell state on the right edge gives the arithmetic run
  1, 13, 25, 37, 49, 61 down to the clue of 61 in the bottom row.
- Two monominoes, at (2,6) and (5,10), give the clues of 1 around them.
- The clue of 0 at (10,5) is the corner of an L-tromino, whose only symmetry fixes
  that square.

## Method

Constraint programming was the first attempt. CP-SAT stalled on every encoding,
but it remained useful as a refuter and proved that the state containing (3,3) has
at least 37 cells. That model is not included in this repository.

What found the solution is the simulated annealing in `src/anneal.cpp`. It works on
partitions of the grid, using symmetric add and remove moves plus whole-state
merges, and its energy is the clue error plus a penalty for asymmetry.

## Run it

```bash
python3 src/verify.py
clang++ -O2 -std=c++17 -o anneal src/anneal.cpp
./anneal data/example.txt 2 3000000 20 0.1
./anneal data/clues.txt 6 60000000 30 0.2 6 3 3 37
```

`verify.py` uses only the Python standard library. It checks the worked example
(8,045) and the 11-by-11 grid: connectivity, symmetry, capitols, all 33 clues and
the final sum. The last argument group of the annealing run is the lower bound
above: cell (3,3), minimum size 37.

Uniqueness is not proven. Ten further annealing runs reached no zero-error
partition at all, so they say little either way.

Seed 6 reaches zero error and prints the grid above, in about five minutes on an
Apple laptop. Other seeds may not converge.

## Outcome

The official answer on the
[solution page](https://www.janestreet.com/puzzles/it-s-a-metric-too-solution/) is
**408951**, and my submission is listed there among the correct ones.
