# Coloring Algorithms — Design Spec

## Overview

A single C binary `coloring` implementing 9 graph coloring algorithms from the LSGA course. Uses the igraph C library for graph storage and DIMACS I/O. All algorithm logic is in plain C.

## Binary

Single executable `coloring` built from `coloring.c` plus algorithm modules.

## CLI Interface

```
coloring                        -- prints list of available algorithms
coloring help                   -- prints full usage, algorithms, and flags
coloring <algo> [options]       -- runs the specified algorithm
```

### Algorithms and flags

| Subcommand | Flags |
|---|---|
| `greedy` | `--input FILE` (default: stdin) |
| `welsh-powell` | `--input FILE` |
| `dsatur` | `--input FILE` |
| `rlf` | `--input FILE` |
| `iterated-greedy` | `--input FILE`, `--ordering largest\|reverse\|random` (default: `largest`), `--iterations N` (default: 100) |
| `sa1` | `--input FILE`, `--colors K` (default: DSatur bound), `--iterations N` (default: 10000), `--t0 T` (default: 1.0), `--alpha A` (default: 0.95) |
| `sa2` | `--input FILE`, `--colors K` (default: DSatur bound), `--iterations N` (default: 10000), `--t0 T` (default: 1.0), `--alpha A` (default: 0.95) |
| `tabucol` | `--input FILE`, `--colors K` (default: DSatur bound), `--iterations N` (default: 10000), `--tenure L` (default: 7) |
| `antcolony` | `--input FILE`, `--iterations N` (default: 100), `--ants N` (default: 10), `--alpha A` (default: 1.0), `--rho R` (default: 0.9) |

## Input

DIMACS graph coloring format. Read via `igraph_read_graph_dimacs()`. The graph is stored as an `igraph_t` (undirected, unweighted). Only the graph structure is used from igraph; all algorithm logic is hand-written C.

## Output

JSON to stdout:

```json
{
  "algorithm": "dsatur",
  "colors": 5,
  "assignments": {"0": 0, "1": 2, "2": 1, ...},
  "time_ms": 123
}
```

For heuristic algorithms that may not find a feasible k-coloring, an additional field:
```json
{
  "algorithm": "tabucol",
  "colors": 5,
  "assignments": {"0": 0, "1": 2, ...},
  "time_ms": 1234,
  "conflicts": 3
}
```

## File structure

```
src/
  coloring.c              -- main entry, arg parsing, dispatch
  output.c / output.h     -- JSON serialization helper
  greedy.c / greedy.h
  welsh_powell.c / welsh_powell.h
  dsatur.c / dsatur.h
  rlf.c / rlf.h
  iterated_greedy.c / iterated_greedy.h
  sa.c / sa.h             -- both SA variants
  tabucol.c / tabucol.h
  antcolony.c / antcolony.h

data/
  queen5_5.col            -- 5x5 queen graph, chi = 5
  myciel3.col             -- Mycielski graph on triangle, chi = 4
```

## Algorithm descriptions

### 1. Greedy
Iterate vertices in arbitrary order. Assign each vertex the smallest color not used by its neighbors.

### 2. Welsh-Powell
Sort vertices by decreasing degree. Then apply greedy.

### 3. DSatur
While uncolored vertices remain:
- Compute saturation degree for each uncolored vertex (number of distinct colors among neighbors).
- Pick vertex with max saturation; break ties by max uncolored neighbors.
- Assign smallest available color.

### 4. Recursive Largest First (RLF)
While uncolored vertices remain:
- Pick uncolored vertex with max degree as seed.
- Build maximal independent set I: repeatedly add vertex not adjacent to I, adjacent to max neighbors of I, min degree.
- Assign a new color to all of I.
- Remove I from graph.

### 5. Iterated Greedy
Start with an initial greedy coloring. Repeat:
- Reorder color classes (largest first / reverse / random).
- Apply greedy with vertices ordered by reordered color classes.
- Keep best solution found.

### 6. Simulated Annealing v1
Maintain a complete (possibly infeasible) k-coloring. Moves: change a vertex's color. Objective: minimize number of monochromatic edges (conflicts). Standard SA cooling schedule.

### 7. Simulated Annealing v2
Maintain an incomplete (partially feasible) k-coloring. Moves: pick uncolored vertex v, assign color c, then uncolor all neighbors of v that have color c. Objective: minimize number of uncolored vertices. Standard SA cooling schedule.

### 8. TabuCol
Maintain an incomplete k-coloring. Moves: pick a conflicting vertex v (has a neighbor with same color), change v from color b to c. Tabu list: forbid changing v back to b for `tenure` iterations. Aspiration: accept if better than best known.

### 9. Ant Colony
- Trail matrix `t[u][v]`: pheromone encouraging u and v to share a color.
- Each ant builds a k-coloring solution.
- If partial: complete probabilistically using trail values.
- Update trail: reinforce for vertices sharing colors, evaporate over time.
- Tighten k when a feasible solution is found.

## Build

Build tool: `coffee`. Uses existing `Coffee.toml` with `igraph` dependency. No changes to build configuration needed beyond adding source files.

## Test graphs

- `queen5_5.col`: 5x5 queen graph (25 vertices), chi = 5
- `myciel3.col`: Mycielski graph on C5 (11 vertices), chi = 4

Both are small enough for manual verification and work as correctness smoke tests.
