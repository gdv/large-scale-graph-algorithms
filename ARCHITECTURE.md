# Architecture

This file documents the high-level architecture of the codebase.
It helps AI agents understand the project structure, key modules,
data flow, and design decisions without having to re-discover
them on every exploration.

## Directory Layout

| Directory | Purpose |
|-----------|---------|
| `src/` | Source code: C algorithms (igraph), Python reference implementations (NetworkX), Jupyter notebooks |
| `bin/` | Compiled C executables: `coloring`, `dijkstra-igraph` (baseline), `dijkstra` (pluggable backends), `test_coloring`, `test_dijkstra` |
| `tests/` | C unit tests (custom minimal framework, no external test lib) — `test_coloring.c`, `test_dijkstra.c` |
| `scripts/` | Python utilities: benchmark plotting, test graph generation, result verification |
| `deps/` | Symlinks to igraph 1.0.1 public headers (resolved via `pkg-config` at build time) |
| `data/` | Graph datasets: DIMACS `.col` files, edge lists, `datasets.csv` registry |
| `results/` | Benchmark outputs: per-algorithm `.txt` files (time/memory) and `.txt.json` (JSON results), PNG plots |
| `slides/` | Lecture slides (LaTeX Beamer) for the course |
| `docs/` | Documentation |
| `.github/workflows/` | CI: compiles slides on push |

## Key Types and Abstractions

### Graph representation

All C code uses **igraph** (`igraph_t` from `igraph_datatype.h`). The graph is edge-list based with indexing vectors (`from`, `to`, `oi`, `ii`, `os`, `is`). Vertex/edge iterators (`igraph_vit_t`, `igraph_eit_t`) and dynamic vectors (`igraph_vector_t`, `igraph_vector_int_t`) are used throughout.

### Algorithm interface (coloring)

Every coloring algorithm follows a consistent signature (`src/greedy.h`, `src/dsatur.h`, etc.):

```c
igraph_integer_t algo_run(const igraph_t *g, igraph_integer_t *color);
```

Parameters are algorithm-specific and passed via `struct` or extra args for metaheuristics (`src/sa.h`, `src/tabucol.h`, `src/antcolony.h`). Return value is number of colors used (constructive) or conflicts remaining (metaheuristic).

### Algorithm interface (shortest path)

**Baseline (igraph-based):** `dijkstra_run()` (`src/dijkstra.h`) computes distances, parent pointers, and reachable count:

```c
void dijkstra_run(const igraph_t *g, igraph_integer_t from,
                  igraph_real_t *dist, igraph_integer_t *parent,
                  igraph_integer_t *n_reached);
```

**Pluggable architecture:** `dijkstra_core_run()` (`src/dijkstra_core.h`) is backend-agnostic, using pluggable adjacency storage and priority queues:

```c
void dijkstra_core_run(const adjacency_t *adj,
                       const pq_vtable_t *pq_vt,
                       igraph_integer_t source,
                       igraph_real_t *dist,
                       igraph_integer_t *parent,
                       igraph_integer_t *n_reached);
```

**Adjacency backends** (`src/adjacency.h`): CSR array (`adj_array.c`), sorted CSR (`adj_sorted.c`), singly-linked list (`adj_linked.c`), open-addressing hash table (`adj_hash.c`). All implement the same vtable interface for uniform dispatch.

**Priority queue backends** (`src/priority_queue.h`): Binary min-heap with lazy deletion (`src/pq_binary.c`). Extensible via vtable for future backends (d-ary, pairing, Fibonacci heaps).

### Output format

`output_json()` (`src/output.h`) serializes coloring results to JSON: algorithm name, color count, per-vertex assignments, runtime, conflicts.

### Test framework

Custom minimal framework in `tests/test_coloring.c` and `tests/test_dijkstra.c`: global `passed`/`failed` counters, a `check(name, ok)` macro, and `main()` printing PASS/FAIL per assertion and a summary.

## Control Flow

### Build (`Makefile`)

`make` compiles `src/coloring.c`, `src/dijkstra_main.c`, and per-algorithm `.c` files against igraph (via `pkg-config`), outputs to `bin/`. `make test` builds and runs `test_coloring` and `test_dijkstra`. Dependency tracking via `-MMD -MP`.

### Coloring CLI (`bin/coloring` → `src/coloring.c`)

1. Parse CLI arguments (algorithm name, input file `-i`, target colors `-k`, iterations `-I`, ordering `-o`, SA temp `-t`, alpha `-a`, tabu tenure `-l`, ants `-n`, rho `-r`)
2. `read_graph_or_die()` — detect format (DIMACS `.col` via `igraph_read_graph_col()`, edge list via `igraph_read_graph_ncol()`, support `.gz`)
3. Dispatch to selected algorithm via function pointer
4. `output_json()` — print JSON to stdout

### Dijkstra CLI (baseline: `bin/dijkstra-igraph` → `src/dijkstra_main.c`)

1. Parse `-i` (input file, auto-detect format by extension: `.gr` DIMACS SP, `.col` DIMACS COL, `.ncol`/`.txt`/`.tsv` edge list, `.gml`/`.graphml`/`.lgl`/`.net`/`.pajek`/`.dl` igraph formats, all support `.gz`) and `-s` (source vertex)
2. Ensure directed graph
3. `dijkstra_run()` — Dijkstra with custom binary min-heap (`src/dijkstra.c`)
4. Output JSON: `source`, `n`, `n_reached`, `distances`, `parents`, `paths`

### Dijkstra CLI (pluggable: `bin/dijkstra` → `src/dijkstra_main2.c`)

1. Parse `-i`, `-s`, `--graph={array,sorted,linked,hash}`, `--pq=binary`
2. Read graph via `read_graph_or_die()` (shared with baseline, factored into `src/graph_io.c`)
3. Build adjacency backend from igraph edges
4. `dijkstra_core_run()` — backend-agnostic Dijkstra
5. Output JSON: same as baseline + `adjacency` and `priority_queue` fields

### Snakemake pipeline (`Snakefile`)

1. Read `data/datasets.csv` — maps dataset names to algorithm groups, directions, weighting
2. Download missing datasets via `curl`
3. For each (dataset, program) pair, run the algorithm binary under `/usr/bin/time -v`, capture stdout and stderr to `results/{dataset}-{program}.txt`
4. Run `scripts/benchmark_plots.py` — parse result files, generate `coloring_speed.png`, `coloring_memory.png`, `coloring_colors.png`

## Data Flow

```
Input graph (DIMACS .col / edge list / igraph formats)
    │
    ▼
read_graph_or_die() / igraph_read_graph_*()
    │
    ▼
igraph_t  (in-memory graph representation)
    │
    ├──► greedy_run() / welsh_powell_run() / dsatur_run() / rlf_run()
    │         Constructive algorithms → color[] array + color count
    │
    ├──► iterated_greedy_run() / sa1_run() / sa2_run() / tabucol_run() / antcolony_run()
    │         Metaheuristics → color[] array + conflicts
    │
    └──► dijkstra_run()
              Shortest path → dist[] + parent[] + n_reached
    │
    ▼
output_json() / manual JSON print → stdout (machine-readable JSON)
```

## Design Decisions

- **igraph for graph core**: Provides production-grade graph I/O (DIMACS, NCOL, GML, GraphML, etc.), iterators, and data structures. Avoids reimplementing graph storage.
- **Pluggable Dijkstra architecture**: Adjacency storage and priority queues are abstracted behind vtables, enabling controlled experiments on data structure performance without duplicating the algorithm logic. The hash backend is self-implemented (open-addressing, linear probing) to avoid external dependencies in the critical path.
- **Consistent algorithm signatures**: All coloring algorithms share `algo_run(igraph_t*, color[])` — enables uniform dispatch and testing with function pointers.
- **Custom test framework**: Avoids external test dependencies; minimal `check()` macro is sufficient for a teaching codebase.
- **JSON output**: Machine-readable, language-agnostic, easy to parse in Python for downstream analysis and plotting.
- **Dual language (C + Python)**: C for performance on large graphs (up to 2M nodes like RoadNet-CA); Python/NetworkX for readable reference implementations used in teaching.
- **DIMACS format**: Standard in the graph coloring community; benchmarks are comparable with published results.
- **Snakemake pipeline**: Reproducible benchmarks with automatic dependency tracking, parallel execution, and result aggregation.
- **No vendored igraph**: `deps/` contains only header symlinks; the library is resolved via `pkg-config`, keeping the repo slim.

## External Dependencies

| Dependency | Type | Used For | Scope |
|------------|------|----------|-------|
| **igraph** (C, v1.0.1) | C library (via pixi) | Graph representation, I/O, iterators, utilities | All C code |
| **pkg-config** | Build tool (via pixi) | Locate igraph headers and libraries | Makefile |
| **NetworkX** (Python, via pixi) | Python | Reference coloring/shortest-path implementations | `src/coloring-*.py`, `src/shortest-path.py` |
| **pandas** (Python, via pixi) | Python | Result parsing in benchmark plots | `scripts/benchmark_plots.py`, `scripts/shortest_plots.py` |
| **matplotlib** (Python, via pixi) | Python | Benchmark plot generation | `scripts/benchmark_plots.py`, `scripts/shortest_plots.py` |
| **numpy** (Python, via pixi) | Python | Numerical operations in plotting | `scripts/benchmark_plots.py` |
| **heapdict** (Python, via pixi) | Python | Priority queue for DSatur reference implementations | `src/coloring-dsatur-heap.py`, `src/coloring-dsatur-heap-faster.py` |
| **snakemake** (Python, via pixi) | Python | Benchmark pipeline orchestration | `Snakefile` |
| **jupyterlab** (Python, via pixi) | Python | Interactive notebooks | `src/hdf5.ipynb` |
| **math.h** (`-lm`) | C stdlib | Floating-point operations | SA cooling, Dijkstra |
| **/usr/bin/time** | System | Wall-clock time + peak memory measurement | Snakemake pipeline |

**All external libraries are installed via pixi (conda-forge channel).** No system libraries from `/usr`, `/lib`, or similar directories are used. The only exception is `-lm` (math library), which is part of the C standard library and always available.

The Makefile embeds an rpath (`-Wl,-rpath,$(CONDA_PREFIX)/lib`) in all binaries, ensuring they link to pixi's libraries at runtime, not system libraries. This is verified by `ldd bin/dijkstra | grep igraph` showing `.pixi/envs/default/lib/libigraph.so.4`.

Build and test commands should be run via pixi to ensure the correct environment:
```bash
pixi run build    # or: pixi run make
pixi run test     # or: pixi run make test
pixi run clean    # or: pixi run make clean
```

No external libraries are used in the Dijkstra critical path (adjacency storage and priority queues are self-implemented).

## Entry Points

| Binary / Script | Language | Mode | Source File(s) |
|-----------------|----------|------|----------------|
| `bin/coloring` | C | 9 graph coloring algorithms | `src/coloring.c`, `src/{greedy,welsh_powell,dsatur,rlf,iterated_greedy,sa,tabucol,antcolony}.c` |
| `bin/dijkstra-igraph` | C | Shortest path (Dijkstra, igraph baseline) | `src/dijkstra_main.c`, `src/dijkstra.c` |
| `bin/dijkstra` | C | Shortest path (Dijkstra, pluggable backends) | `src/dijkstra_main2.c`, `src/dijkstra_core.c`, `src/adjacency.c`, `src/adj_{array,sorted,linked,hash}.c`, `src/priority_queue.c`, `src/pq_binary.c`, `src/graph_io.c` |
| `bin/test_coloring` | C | Unit tests for coloring | `tests/test_coloring.c` |
| `bin/test_dijkstra` | C | Unit tests for Dijkstra (including all backends) | `tests/test_dijkstra.c` |
| `bin/search` | C | Traversal & connectivity (BFS, DFS, A*, SCC, AP, BCC, bridges, 2-edge) | `src/search_main.c`, `src/search.c`, `src/connectivity.c`, `src/csr.c` |
| `bin/flow` | C | Max flow (Ford–Fulkerson, Edmonds–Karp, Dinic, preflow-push) + min cut | `src/flow_main.c`, `src/flow.c` |
| `bin/matching` | C | Bipartite matching (Hopcroft–Karp, via flow) + Hungarian assignment | `src/matching_main.c`, `src/matching.c`, `src/hungarian.c` |
| `bin/compress` | C | Graph compression codecs | `src/compress_main.c`, `src/bitio.c`, `src/codes.c`, `src/huffman.c`, `src/mtf.c`, `src/graphcode.c` |
| `bin/randomized` | C | Randomized max cut, Karger, Karger–Stein | `src/rand_main.c`, `src/maxcut.c`, `src/karger.c`, `src/uf.c`, `src/edgegraph.c` |
| `bin/test_csr`, `bin/test_search`, `bin/test_connectivity`, `bin/test_flow`, `bin/test_matching`, `bin/test_compress`, `bin/test_random` | C | Unit tests for the new modules | `tests/test_*.c` |
| `src/coloring-greedy.py` etc. | Python | Reference coloring implementations | `src/coloring-*.py` |
| `src/shortest-path.py` | Python | Reference shortest path | `src/shortest-path.py` |
| `scripts/benchmark_plots.py` | Python | Benchmark visualization (coloring) | `scripts/benchmark_plots.py` |
| `scripts/shortest_plots.py` | Python | Benchmark visualization (Dijkstra backends) | `scripts/shortest_plots.py` |
| `scripts/gen_test_graphs.py` | Python | Generate DIMACS test graphs | `scripts/gen_test_graphs.py` |
| `scripts/verify_coloring.py` | Python | Verify coloring correctness | `scripts/verify_coloring.py` |
| `snakemake` | Python | Full benchmark pipeline | `Snakefile` + `data/datasets.csv` |

## Implemented Algorithms

### Coloring (C, all via `bin/coloring`):

| Algorithm | Type | Function | Source |
|-----------|------|----------|--------|
| Greedy | Constructive (first-fit) | `greedy_run()` | `src/greedy.c` |
| Welsh–Powell | Constructive (degree ordering) | `welsh_powell_run()` | `src/welsh_powell.c` |
| DSatur | Constructive (saturation degree) | `dsatur_run()` | `src/dsatur.c` |
| Recursive Largest First (RLF) | Constructive (max independent set) | `rlf_run()` | `src/rlf.c` |
| Iterated Greedy | Metaheuristic | `iterated_greedy_run()` | `src/iterated_greedy.c` |
| Simulated Annealing (v1) | Metaheuristic | `sa1_run()` | `src/sa.c` |
| Simulated Annealing (v2) | Metaheuristic | `sa2_run()` | `src/sa.c` |
| TabuCol | Metaheuristic | `tabucol_run()` | `src/tabucol.c` |
| Ant Colony Optimization | Metaheuristic | `antcolony_run()` | `src/antcolony.c` |

### Shortest Path:

| Algorithm | Language | Source |
|-----------|----------|--------|
| Dijkstra (binary heap, igraph baseline) | C | `src/dijkstra.c` |
| Dijkstra (pluggable: array/sorted/linked/hash adjacency, binary heap PQ) | C | `src/dijkstra_core.c`, `src/adj_*.c`, `src/pq_binary.c` |
| Dijkstra (NetworkX) | Python | `src/shortest-path.py` |
| Dijkstra (Rust) | Rust | `src/target/release/st-shortest-path` |

### Traversal & Connectivity (C, via `bin/search`):

| Algorithm | Function | Source |
|-----------|----------|--------|
| BFS | `bfs_run()` | `src/search.c` |
| DFS (iterative, discovery/finish times) | `dfs_run()` | `src/search.c` |
| A* (unit weights, binary heap on f = g + h) | `astar_run()` | `src/search.c` |
| Strongly connected components (Tarjan) | `scc_run()` | `src/connectivity.c` |
| Articulation points | `articulation_points_run()` | `src/connectivity.c` |
| Biconnected components (edge blocks) | `biconnected_run()` | `src/connectivity.c` |
| Bridges | `bridges_run()` | `src/connectivity.c` |
| 2-edge-connected components | `edge2_components_run()` | `src/connectivity.c` |

All operate on the CSR representation (`src/csr.h`: offsets/targets/rev), built from igraph.

### Max Flow (C, via `bin/flow`):

| Algorithm | Function | Source |
|-----------|----------|--------|
| Ford–Fulkerson (DFS augmenting paths) | `flow_ford_fulkerson()` | `src/flow.c` |
| Edmonds–Karp (BFS augmenting paths) | `flow_edmonds_karp()` | `src/flow.c` |
| Dinic (level graph + blocking flow) | `flow_dinic()` | `src/flow.c` |
| Preflow-push (Goldberg–Tarjan, FIFO) | `flow_preflow_push()` | `src/flow.c` |
| Min s-t cut (residual reachability) | `flow_mincut_side()` | `src/flow.c` |

All share the paired-arc residual network (`src/flow.h`: arc a and its reverse a XOR 1).

### Bipartite Matching (C, via `bin/matching`):

| Algorithm | Function | Source |
|-----------|----------|--------|
| Hopcroft–Karp | `matching_hopcroft_karp()` | `src/matching.c` |
| Matching via max flow | `matching_via_flow()` | `src/matching.c` |
| Hungarian (assignment problem, O(n³)) | `hungarian_solve()` | `src/hungarian.c` |

### Graph Compression (C, via `bin/compress`):

| Codec | Source |
|-------|--------|
| MSB-first bit I/O | `src/bitio.c` |
| Elias γ/δ, nibble, minimal binary, ζk | `src/codes.c` |
| Canonical Huffman (binary heap + trie decode) | `src/huffman.c` |
| Move-to-front, run-length encoding | `src/mtf.c` |
| Gap / reference / interval adjacency coding | `src/graphcode.c` |

### Randomized Algorithms (C, via `bin/randomized`):

| Algorithm | Function | Source |
|-----------|----------|--------|
| Randomized max cut (best of trials) | `maxcut_best()` | `src/maxcut.c` |
| Karger contraction (trials) | `karger_mincut()` | `src/karger.c` |
| Karger–Stein (recursive doubling) | `karger_stein()` | `src/karger.c` |
| Brute-force min cut (test oracle, n ≤ 24) | `karger_brute_mincut()` | `src/karger.c` |
| Union-find (path halving, union by rank) | `uf_*` | `src/uf.c` |

### Connectivity (Rust):

| Algorithm | Binary |
|-----------|--------|
| DFS | `src/target/release/dfs` |
| BFS | `src/target/release/bfs` |
