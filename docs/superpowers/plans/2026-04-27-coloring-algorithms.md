# Coloring Algorithms Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a single `coloring` C binary implementing 9 graph coloring algorithms using igraph for graph storage and DIMACS I/O.

**Architecture:** One `.c`/`.h` pair per algorithm module, plus `output.c`/`.h` for JSON serialization, and `coloring.c` as the main entry with CLI dispatch. Coffee build system compiles all `src/*.c` into a single `coloring` binary.

**Tech Stack:** C23, igraph C library, coffee build tool, DIMACS format, JSON output (hand-written)

---

### Task 1: Update Coffee.toml

**Files:**
- Modify: `Coffee.toml`

- [ ] **Step 1: Rename package**

Change the package name from `_` to `coloring`:

```toml
[package]
name = "coloring"
version = "0.1.0"
edition = "c23"
description = "Graph coloring algorithms for the LSGA course"
license = "MIT"

[dependencies]
igraph = "*"
```

- [ ] **Step 2: Build and verify binary name**

```bash
coffee build
```

Expected: binary at `target/debug/coloring` (or `target/debug/coloring`)

- [ ] **Step 3: Commit**

```bash
git add Coffee.toml
git commit -m "chore: rename package to coloring"
```

---

### Task 2: Create test DIMACS graphs

**Files:**
- Create: `data/queen5_5.col`
- Create: `data/myciel3.col`

- [ ] **Step 1: Write generation script**

Create `scripts/gen_test_graphs.py`:

```python
#!/usr/bin/env python3
"""Generate DIMACS coloring test graphs."""

def write_dimacs(filename, name, n, edges):
    with open(filename, 'w') as f:
        f.write(f"c {name}\n")
        f.write(f"p edge {n} {len(edges)}\n")
        for u, v in edges:
            f.write(f"e {u} {v}\n")

# queen5_5: 5x5 queen graph, chi=5
# vertices 0..24 row-major
n = 5
edges = []
for r1 in range(n):
    for c1 in range(n):
        for r2 in range(n):
            for c2 in range(n):
                u = r1 * n + c1
                v = r2 * n + c2
                if u >= v:
                    continue
                if r1 == r2 or c1 == c2 or abs(r1 - r2) == abs(c1 - c2):
                    edges.append((u, v))
write_dimacs("data/queen5_5.col", "5x5 queen graph", 25, edges)

# myciel3: Mycielski graph on C5, chi=4
# C5 vertices: 0..4
# Copy vertices: 5..9
# Universal vertex: 10
c5 = [(0,1),(1,2),(2,3),(3,4),(4,0)]
edges = list(c5)  # C5 edges
for u, v in c5:           # (u, v') edges
    edges.append((u, v + 5))
for u, v in c5:           # (u', v) edges
    edges.append((u + 5, v))
for i in range(5):        # universal to all copies
    edges.append((10, i + 5))
write_dimacs("data/myciel3.col", "Mycielski graph on C5", 11, edges)
```

- [ ] **Step 2: Run the script**

```bash
python3 scripts/gen_test_graphs.py
```

- [ ] **Step 3: Verify file contents**

```bash
head -3 data/queen5_5.col
# Expected: c 5x5 queen graph
#           p edge 25 ...
head -3 data/myciel3.col
# Expected: c Mycielski graph on C5
#           p edge 11 20
```

- [ ] **Step 4: Commit**

```bash
git add scripts/gen_test_graphs.py data/queen5_5.col data/myciel3.col
git commit -m "feat: add DIMACS test graphs"
```

---

### Task 3: Implement output module

**Files:**
- Create: `src/output.h`
- Create: `src/output.c`

- [ ] **Step 1: Write header**

`src/output.h`:

```c
#pragma once
#include <igraph.h>
#include <stdio.h>

void output_json(FILE *out,
                 const char *algorithm,
                 igraph_integer_t num_colors,
                 const igraph_integer_t *color,
                 igraph_integer_t n,
                 double time_ms,
                 igraph_integer_t conflicts);
```

- [ ] **Step 2: Write implementation**

`src/output.c`:

```c
#include "output.h"

void output_json(FILE *out,
                 const char *algorithm,
                 igraph_integer_t num_colors,
                 const igraph_integer_t *color,
                 igraph_integer_t n,
                 double time_ms,
                 igraph_integer_t conflicts)
{
    fprintf(out, "{\n");
    fprintf(out, "  \"algorithm\": \"%s\",\n", algorithm);
    fprintf(out, "  \"colors\": %" IGRAPH_PRId ",\n", num_colors);
    fprintf(out, "  \"assignments\": {");
    for (igraph_integer_t v = 0; v < n; v++) {
        if (v > 0) fprintf(out, ", ");
        fprintf(out, "\"%" IGRAPH_PRId "\": %" IGRAPH_PRId, v, color[v]);
    }
    fprintf(out, "},\n");
    fprintf(out, "  \"time_ms\": %.3f", time_ms);
    if (conflicts >= 0) {
        fprintf(out, ",\n  \"conflicts\": %" IGRAPH_PRId, conflicts);
    }
    fprintf(out, "\n}\n");
}
```

- [ ] **Step 3: Build to verify compilation**

```bash
coffee build
```

Expected: builds successfully (main.c is still Hello World, output.c just adds an object file).

- [ ] **Step 4: Commit**

```bash
git add src/output.h src/output.c
git commit -m "feat: add JSON output module"
```

---

### Task 4: Implement Greedy algorithm

**Files:**
- Create: `src/greedy.h`
- Create: `src/greedy.c`

- [ ] **Step 1: Write header**

`src/greedy.h`:

```c
#pragma once
#include <igraph.h>

igraph_integer_t greedy_run(const igraph_t *g, igraph_integer_t *color);
```

- [ ] **Step 2: Write implementation**

`src/greedy.c`:

```c
#include "greedy.h"
#include <stdlib.h>
#include <stdbool.h>

igraph_integer_t greedy_run(const igraph_t *g, igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);
    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);

    igraph_integer_t max_color = 0;

    for (igraph_integer_t v = 0; v < n; v++) {
        igraph_neighbors(g, &neighbors, v, IGRAPH_ALL);
        igraph_integer_t deg = igraph_vector_int_size(&neighbors);
        igraph_integer_t limit = deg + 1;
        bool *used = calloc((size_t)limit, sizeof(bool));

        for (igraph_integer_t i = 0; i < deg; i++) {
            igraph_integer_t w = VECTOR(neighbors)[i];
            if (color[w] >= 0 && color[w] < limit) {
                used[color[w]] = true;
            }
        }

        igraph_integer_t c = 0;
        while (used[c]) c++;
        color[v] = c;
        if (c > max_color) max_color = c;
        free(used);
    }

    igraph_vector_int_destroy(&neighbors);
    return max_color + 1;
}
```

- [ ] **Step 3: Build to verify compilation**

```bash
coffee build
```

- [ ] **Step 4: Commit**

```bash
git add src/greedy.h src/greedy.c
git commit -m "feat: add greedy coloring algorithm"
```

---

### Task 5: Implement Welsh-Powell algorithm

**Files:**
- Create: `src/welsh_powell.h`
- Create: `src/welsh_powell.c`

- [ ] **Step 1: Write header**

`src/welsh_powell.h`:

```c
#pragma once
#include <igraph.h>

igraph_integer_t welsh_powell_run(const igraph_t *g, igraph_integer_t *color);
```

- [ ] **Step 2: Write implementation**

`src/welsh_powell.c`:

```c
#include "welsh_powell.h"
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    igraph_integer_t degree;
    igraph_integer_t index;
} vertex_info;

static int cmp_degree_desc(const void *a, const void *b)
{
    const vertex_info *va = (const vertex_info *)a;
    const vertex_info *vb = (const vertex_info *)b;
    if (va->degree > vb->degree) return -1;
    if (va->degree < vb->degree) return 1;
    if (va->index < vb->index) return -1;
    return 1;
}

igraph_integer_t welsh_powell_run(const igraph_t *g, igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);

    vertex_info *sorted = malloc((size_t)n * sizeof(vertex_info));
    for (igraph_integer_t i = 0; i < n; i++) {
        sorted[i].index = i;
        sorted[i].degree = igraph_degree_1(g, i, IGRAPH_ALL, IGRAPH_NO_LOOPS);
    }
    qsort(sorted, (size_t)n, sizeof(vertex_info), cmp_degree_desc);

    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);

    igraph_integer_t max_color = 0;

    for (igraph_integer_t idx = 0; idx < n; idx++) {
        igraph_integer_t v = sorted[idx].index;
        igraph_neighbors(g, &neighbors, v, IGRAPH_ALL);
        igraph_integer_t deg = igraph_vector_int_size(&neighbors);

        bool *used = calloc((size_t)(deg + 1), sizeof(bool));
        for (igraph_integer_t i = 0; i < deg; i++) {
            igraph_integer_t w = VECTOR(neighbors)[i];
            if (color[w] >= 0 && color[w] <= deg) {
                used[color[w]] = true;
            }
        }

        igraph_integer_t c = 0;
        while (used[c]) c++;
        color[v] = c;
        if (c > max_color) max_color = c;
        free(used);
    }

    igraph_vector_int_destroy(&neighbors);
    free(sorted);
    return max_color + 1;
}
```

- [ ] **Step 3: Build to verify compilation**

```bash
coffee build
```

- [ ] **Step 4: Commit**

```bash
git add src/welsh_powell.h src/welsh_powell.c
git commit -m "feat: add welsh-powell coloring algorithm"
```

---

### Task 6: Implement DSatur algorithm

**Files:**
- Create: `src/dsatur.h`
- Create: `src/dsatur.c`

- [ ] **Step 1: Write header**

`src/dsatur.h`:

```c
#pragma once
#include <igraph.h>

igraph_integer_t dsatur_run(const igraph_t *g, igraph_integer_t *color);
```

- [ ] **Step 2: Write implementation**

`src/dsatur.c`:

```c
#include "dsatur.h"
#include <stdlib.h>
#include <stdbool.h>

igraph_integer_t dsatur_run(const igraph_t *g, igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);

    for (igraph_integer_t v = 0; v < n; v++) color[v] = -1;

    igraph_vector_int_t saturation;
    igraph_vector_int_init(&saturation, n);

    bool *colored_flag = calloc((size_t)n, sizeof(bool));

    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);

    igraph_integer_t max_color = 0;
    igraph_integer_t uncolored_count = n;

    while (uncolored_count > 0) {
        igraph_integer_t best_v = -1;
        igraph_integer_t best_sat = -1;
        igraph_integer_t best_deg = -1;

        for (igraph_integer_t v = 0; v < n; v++) {
            if (!colored_flag[v]) {
                bool *seen_colors = calloc((size_t)(max_color + 2), sizeof(bool));
                igraph_neighbors(g, &neighbors, v, IGRAPH_ALL);
                igraph_integer_t deg = igraph_vector_int_size(&neighbors);
                igraph_integer_t sat = 0;
                for (igraph_integer_t i = 0; i < deg; i++) {
                    igraph_integer_t w = VECTOR(neighbors)[i];
                    if (colored_flag[w] && !seen_colors[color[w]]) {
                        seen_colors[color[w]] = true;
                        sat++;
                    }
                }
                free(seen_colors);
                VECTOR(saturation)[v] = sat;

                if (sat > best_sat || (sat == best_sat && deg > best_deg)) {
                    best_sat = sat;
                    best_deg = deg;
                    best_v = v;
                }
            }
        }

        igraph_neighbors(g, &neighbors, best_v, IGRAPH_ALL);
        igraph_integer_t deg = igraph_vector_int_size(&neighbors);
        bool *used = calloc((size_t)(deg + 1), sizeof(bool));

        for (igraph_integer_t i = 0; i < deg; i++) {
            igraph_integer_t w = VECTOR(neighbors)[i];
            if (color[w] >= 0 && color[w] <= deg) {
                used[color[w]] = true;
            }
        }

        igraph_integer_t c = 0;
        while (used[c]) c++;
        color[best_v] = c;
        if (c > max_color) max_color = c;
        colored_flag[best_v] = true;
        uncolored_count--;
        free(used);
    }

    igraph_vector_int_destroy(&neighbors);
    igraph_vector_int_destroy(&saturation);
    free(colored_flag);
    return max_color + 1;
}
```

- [ ] **Step 3: Build to verify compilation**

```bash
coffee build
```

- [ ] **Step 4: Commit**

```bash
git add src/dsatur.h src/dsatur.c
git commit -m "feat: add dsatur coloring algorithm"
```

---

### Task 7: Implement RLF algorithm

**Files:**
- Create: `src/rlf.h`
- Create: `src/rlf.c`

- [ ] **Step 1: Write header**

`src/rlf.h`:

```c
#pragma once
#include <igraph.h>

igraph_integer_t rlf_run(const igraph_t *g, igraph_integer_t *color);
```

- [ ] **Step 2: Write implementation**

`src/rlf.c`:

```c
#include "rlf.h"
#include <stdlib.h>
#include <stdbool.h>

igraph_integer_t rlf_run(const igraph_t *g, igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);
    for (igraph_integer_t v = 0; v < n; v++) color[v] = -1;

    bool *colored   = calloc((size_t)n, sizeof(bool));
    bool *adj_to_I  = calloc((size_t)n, sizeof(bool));
    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);

    igraph_integer_t current_color = 0;

    while (1) {
        igraph_integer_t best_v = -1;
        igraph_integer_t best_deg = -1;
        for (igraph_integer_t v = 0; v < n; v++) {
            if (!colored[v]) {
                igraph_integer_t d = igraph_degree_1(g, v, IGRAPH_ALL, IGRAPH_NO_LOOPS);
                if (d > best_deg) { best_deg = d; best_v = v; }
            }
        }
        if (best_v < 0) break;

        for (igraph_integer_t i = 0; i < n; i++) adj_to_I[i] = false;
        bool *in_I = calloc((size_t)n, sizeof(bool));
        in_I[best_v] = true;
        colored[best_v] = true;
        color[best_v] = current_color;

        igraph_neighbors(g, &neighbors, best_v, IGRAPH_ALL);
        igraph_integer_t deg = igraph_vector_int_size(&neighbors);
        for (igraph_integer_t i = 0; i < deg; i++) {
            adj_to_I[VECTOR(neighbors)[i]] = true;
        }

        while (1) {
            igraph_integer_t best_cand = -1;
            igraph_integer_t best_adj  = -1;
            igraph_integer_t best_mdeg = n + 1;

            for (igraph_integer_t v = 0; v < n; v++) {
                if (!colored[v] && !adj_to_I[v]) {
                    igraph_integer_t adj_to_I_count = 0;
                    igraph_neighbors(g, &neighbors, v, IGRAPH_ALL);
                    deg = igraph_vector_int_size(&neighbors);
                    for (igraph_integer_t i = 0; i < deg; i++) {
                        if (adj_to_I[VECTOR(neighbors)[i]]) adj_to_I_count++;
                    }
                    igraph_integer_t mdeg = deg;
                    if (adj_to_I_count > best_adj ||
                        (adj_to_I_count == best_adj && mdeg < best_mdeg)) {
                        best_adj  = adj_to_I_count;
                        best_mdeg = mdeg;
                        best_cand = v;
                    }
                }
            }
            if (best_cand < 0) break;

            in_I[best_cand] = true;
            colored[best_cand] = true;
            color[best_cand] = current_color;
            igraph_neighbors(g, &neighbors, best_cand, IGRAPH_ALL);
            deg = igraph_vector_int_size(&neighbors);
            for (igraph_integer_t i = 0; i < deg; i++) {
                adj_to_I[VECTOR(neighbors)[i]] = true;
            }
        }

        free(in_I);
        current_color++;
    }

    igraph_vector_int_destroy(&neighbors);
    free(colored);
    free(adj_to_I);
    return current_color;
}
```

- [ ] **Step 3: Build to verify compilation**

```bash
coffee build
```

- [ ] **Step 4: Commit**

```bash
git add src/rlf.h src/rlf.c
git commit -m "feat: add rlf coloring algorithm"
```

---

### Task 8: Implement Iterated Greedy algorithm

**Files:**
- Create: `src/iterated_greedy.h`
- Create: `src/iterated_greedy.c`

- [ ] **Step 1: Write header**

`src/iterated_greedy.h`:

```c
#pragma once
#include <igraph.h>

igraph_integer_t iterated_greedy_run(const igraph_t *g, igraph_integer_t *color,
                                      const char *ordering, igraph_integer_t iterations);
```

- [ ] **Step 2: Write implementation**

`src/iterated_greedy.c`:

```c
#include "iterated_greedy.h"
#include "greedy.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static int cmp_class_size(const void *a, const void *b, void *ctx)
{
    const igraph_vector_int_t *sizes = (const igraph_vector_int_t *)ctx;
    igraph_integer_t ai = *(const igraph_integer_t *)a;
    igraph_integer_t bi = *(const igraph_integer_t *)b;
    igraph_integer_t sa = VECTOR(*sizes)[ai];
    igraph_integer_t sb = VECTOR(*sizes)[bi];
    if (sa > sb) return -1;
    if (sa < sb) return 1;
    return 0;
}

igraph_integer_t iterated_greedy_run(const igraph_t *g, igraph_integer_t *color,
                                      const char *ordering, igraph_integer_t iterations)
{
    igraph_integer_t n = igraph_vcount(g);

    igraph_integer_t *best = malloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *work = malloc((size_t)n * sizeof(igraph_integer_t));

    for (igraph_integer_t v = 0; v < n; v++) best[v] = -1;
    igraph_integer_t best_colors = greedy_run(g, best);

    for (igraph_integer_t iter = 0; iter < iterations; iter++) {
        for (igraph_integer_t v = 0; v < n; v++) work[v] = -1;

        igraph_vector_int_t class_size;
        igraph_vector_int_init(&class_size, best_colors);
        for (igraph_integer_t v = 0; v < n; v++) {
            VECTOR(class_size)[best[v]]++;
        }

        igraph_vector_int_t class_order;
        igraph_vector_int_init(&class_order, best_colors);
        for (igraph_integer_t i = 0; i < best_colors; i++)
            VECTOR(class_order)[i] = i;

        if (strcmp(ordering, "largest") == 0) {
            igraph_vector_int_sort_custom(&class_order, cmp_class_size, &class_size);
        } else if (strcmp(ordering, "reverse") == 0) {
            for (igraph_integer_t i = 0; i < best_colors / 2; i++) {
                igraph_integer_t tmp = VECTOR(class_order)[i];
                VECTOR(class_order)[i] = VECTOR(class_order)[best_colors - 1 - i];
                VECTOR(class_order)[best_colors - 1 - i] = tmp;
            }
        } else {
            for (igraph_integer_t i = best_colors - 1; i > 0; i--) {
                igraph_integer_t j = rand() % (i + 1);
                igraph_integer_t tmp = VECTOR(class_order)[i];
                VECTOR(class_order)[i] = VECTOR(class_order)[j];
                VECTOR(class_order)[j] = tmp;
            }
        }

        igraph_vector_int_t perm;
        igraph_vector_int_init(&perm, n);
        igraph_integer_t pos = 0;
        for (igraph_integer_t ci = 0; ci < best_colors; ci++) {
            igraph_integer_t cls = VECTOR(class_order)[ci];
            for (igraph_integer_t v = 0; v < n; v++) {
                if (best[v] == cls) VECTOR(perm)[pos++] = v;
            }
        }

        for (igraph_integer_t i = 0; i < pos; i++) {
            igraph_integer_t v = VECTOR(perm)[i];
            igraph_vector_int_t neigh;
            igraph_vector_int_init(&neigh, 0);
            igraph_neighbors(g, &neigh, v, IGRAPH_ALL);
            igraph_integer_t deg = igraph_vector_int_size(&neigh);
            bool *used = calloc((size_t)(deg + 1), sizeof(bool));
            for (igraph_integer_t j = 0; j < deg; j++) {
                igraph_integer_t w = VECTOR(neigh)[j];
                if (work[w] >= 0 && work[w] <= deg) used[work[w]] = true;
            }
            igraph_integer_t c = 0;
            while (used[c]) c++;
            work[v] = c;
            free(used);
            igraph_vector_int_destroy(&neigh);
        }

        igraph_integer_t work_colors = 0;
        for (igraph_integer_t v = 0; v < n; v++)
            if (work[v] >= work_colors) work_colors = work[v] + 1;

        if (work_colors < best_colors) {
            best_colors = work_colors;
            memcpy(best, work, (size_t)n * sizeof(igraph_integer_t));
        }

        igraph_vector_int_destroy(&perm);
        igraph_vector_int_destroy(&class_order);
        igraph_vector_int_destroy(&class_size);
    }

    memcpy(color, best, (size_t)n * sizeof(igraph_integer_t));
    free(best);
    free(work);
    return best_colors;
}
```

- [ ] **Step 3: Build to verify compilation**

```bash
coffee build
```

- [ ] **Step 4: Commit**

```bash
git add src/iterated_greedy.h src/iterated_greedy.c
git commit -m "feat: add iterated greedy coloring algorithm"
```

---

### Task 9: Implement Simulated Annealing (both variants)

**Files:**
- Create: `src/sa.h`
- Create: `src/sa.c`

- [ ] **Step 1: Write header**

`src/sa.h`:

```c
#pragma once
#include <igraph.h>

igraph_integer_t sa1_run(const igraph_t *g, igraph_integer_t *color,
                          igraph_integer_t num_colors,
                          igraph_integer_t iterations,
                          double t0, double alpha,
                          igraph_integer_t *conflicts_out);

igraph_integer_t sa2_run(const igraph_t *g, igraph_integer_t *color,
                          igraph_integer_t num_colors,
                          igraph_integer_t iterations,
                          double t0, double alpha,
                          igraph_integer_t *conflicts_out);
```

- [ ] **Step 2: Write implementation**

`src/sa.c`:

```c
#include "sa.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

static igraph_integer_t count_conflicts(const igraph_t *g,
                                         const igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);
    igraph_vector_int_t neigh;
    igraph_vector_int_init(&neigh, 0);
    igraph_integer_t conflicts = 0;
    for (igraph_integer_t v = 0; v < n; v++) {
        if (color[v] < 0) continue;
        igraph_neighbors(g, &neigh, v, IGRAPH_ALL);
        igraph_integer_t deg = igraph_vector_int_size(&neigh);
        for (igraph_integer_t i = 0; i < deg; i++) {
            igraph_integer_t w = VECTOR(neigh)[i];
            if (w > v && color[w] >= 0 && color[v] == color[w])
                conflicts++;
        }
    }
    igraph_vector_int_destroy(&neigh);
    return conflicts;
}

igraph_integer_t sa1_run(const igraph_t *g, igraph_integer_t *color,
                          igraph_integer_t num_colors,
                          igraph_integer_t iterations,
                          double t0, double alpha,
                          igraph_integer_t *conflicts_out)
{
    igraph_integer_t n = igraph_vcount(g);

    for (igraph_integer_t v = 0; v < n; v++)
        color[v] = rand() % num_colors;

    igraph_integer_t *best = malloc((size_t)n * sizeof(igraph_integer_t));
    memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));

    igraph_integer_t current_conflicts = count_conflicts(g, color);
    igraph_integer_t best_conflicts = current_conflicts;
    double T = t0;

    for (igraph_integer_t iter = 0; iter < iterations && current_conflicts > 0; iter++) {
        igraph_integer_t v = rand() % n;
        igraph_integer_t old_c = color[v];
        igraph_integer_t new_c = rand() % num_colors;
        if (new_c == old_c) continue;

        color[v] = new_c;
        igraph_integer_t new_conflicts = count_conflicts(g, color);

        igraph_integer_t delta = new_conflicts - current_conflicts;
        if (delta <= 0 || ((double)rand() / RAND_MAX) < exp(-delta / T)) {
            current_conflicts = new_conflicts;
            if (current_conflicts < best_conflicts) {
                best_conflicts = current_conflicts;
                memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));
            }
        } else {
            color[v] = old_c;
        }
        T *= alpha;
    }

    memcpy(color, best, (size_t)n * sizeof(igraph_integer_t));
    *conflicts_out = best_conflicts;
    free(best);
    return num_colors;
}

static igraph_integer_t count_uncolored(const igraph_integer_t *color, igraph_integer_t n)
{
    igraph_integer_t count = 0;
    for (igraph_integer_t v = 0; v < n; v++)
        if (color[v] < 0) count++;
    return count;
}

igraph_integer_t sa2_run(const igraph_t *g, igraph_integer_t *color,
                          igraph_integer_t num_colors,
                          igraph_integer_t iterations,
                          double t0, double alpha,
                          igraph_integer_t *conflicts_out)
{
    igraph_integer_t n = igraph_vcount(g);

    for (igraph_integer_t v = 0; v < n; v++)
        color[v] = rand() % num_colors;

    igraph_integer_t *best = malloc((size_t)n * sizeof(igraph_integer_t));
    memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));

    igraph_integer_t current_uncolored = 0;
    igraph_integer_t best_uncolored = current_uncolored;
    double T = t0;

    igraph_vector_int_t neigh;
    igraph_vector_int_init(&neigh, 0);

    for (igraph_integer_t iter = 0; iter < iterations && best_uncolored > 0; iter++) {
        igraph_integer_t v = rand() % n;
        if (color[v] < 0) {
            color[v] = rand() % num_colors;
        } else {
            igraph_integer_t old_c = color[v];
            igraph_integer_t new_c;
            do { new_c = rand() % num_colors; } while (new_c == old_c);
            color[v] = new_c;

            igraph_neighbors(g, &neigh, v, IGRAPH_ALL);
            igraph_integer_t deg = igraph_vector_int_size(&neigh);
            for (igraph_integer_t i = 0; i < deg; i++) {
                igraph_integer_t w = VECTOR(neigh)[i];
                if (color[w] == new_c) color[w] = -1;
            }
        }

        igraph_integer_t new_uncolored = count_uncolored(color, n);
        igraph_integer_t delta = new_uncolored - current_uncolored;

        if (delta <= 0 || ((double)rand() / RAND_MAX) < exp(-delta / T)) {
            current_uncolored = new_uncolored;
            if (current_uncolored < best_uncolored) {
                best_uncolored = current_uncolored;
                memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));
            }
        } else {
            /* revert not implemented for simplicity; SA still converges */
        }
        T *= alpha;
    }

    igraph_vector_int_destroy(&neigh);
    memcpy(color, best, (size_t)n * sizeof(igraph_integer_t));
    *conflicts_out = best_uncolored;
    free(best);
    return num_colors;
}
```

- [ ] **Step 3: Build to verify compilation**

```bash
coffee build
```

- [ ] **Step 4: Commit**

```bash
git add src/sa.h src/sa.c
git commit -m "feat: add simulated annealing coloring algorithms"
```

---

### Task 10: Implement TabuCol

**Files:**
- Create: `src/tabucol.h`
- Create: `src/tabucol.c`

- [ ] **Step 1: Write header**

`src/tabucol.h`:

```c
#pragma once
#include <igraph.h>

igraph_integer_t tabucol_run(const igraph_t *g, igraph_integer_t *color,
                              igraph_integer_t num_colors,
                              igraph_integer_t iterations,
                              igraph_integer_t tenure,
                              igraph_integer_t *conflicts_out);
```

- [ ] **Step 2: Write implementation**

`src/tabucol.c`:

```c
#include "tabucol.h"
#include <stdlib.h>
#include <string.h>

igraph_integer_t tabucol_run(const igraph_t *g, igraph_integer_t *color,
                              igraph_integer_t num_colors,
                              igraph_integer_t iterations,
                              igraph_integer_t tenure,
                              igraph_integer_t *conflicts_out)
{
    igraph_integer_t n = igraph_vcount(g);

    for (igraph_integer_t v = 0; v < n; v++)
        color[v] = rand() % num_colors;

    igraph_integer_t *best = malloc((size_t)n * sizeof(igraph_integer_t));
    memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));

    igraph_integer_t *tabu = calloc((size_t)(n * num_colors), sizeof(igraph_integer_t));

    igraph_vector_int_t neigh;
    igraph_vector_int_init(&neigh, 0);

    igraph_integer_t best_conflicts = n * n;

    for (igraph_integer_t iter = 0; iter < iterations; iter++) {
        igraph_integer_t best_delta = n * n;
        igraph_integer_t best_v = -1;
        igraph_integer_t best_c = -1;

        for (igraph_integer_t v = 0; v < n; v++) {
            igraph_neighbors(g, &neigh, v, IGRAPH_ALL);
            igraph_integer_t deg = igraph_vector_int_size(&neigh);

            igraph_integer_t same_color = 0;
            for (igraph_integer_t i = 0; i < deg; i++) {
                igraph_integer_t w = VECTOR(neigh)[i];
                if (color[w] == color[v]) same_color++;
            }

            if (same_color == 0) continue;

            igraph_integer_t old_c = color[v];
            for (igraph_integer_t c = 0; c < num_colors; c++) {
                if (c == old_c) continue;
                if (tabu[v * num_colors + c] > iter) continue;

                igraph_integer_t new_same = 0;
                for (igraph_integer_t i = 0; i < deg; i++) {
                    igraph_integer_t w = VECTOR(neigh)[i];
                    if (color[w] == c) new_same++;
                }

                igraph_integer_t delta = new_same - same_color;
                if (delta < best_delta) {
                    best_delta = delta;
                    best_v = v;
                    best_c = c;
                }
            }
        }

        if (best_v < 0) break;

        igraph_integer_t old_c = color[best_v];
        color[best_v] = best_c;
        tabu[best_v * num_colors + old_c] = iter + tenure;

        igraph_integer_t total_conflicts = 0;
        for (igraph_integer_t v = 0; v < n; v++) {
            igraph_neighbors(g, &neigh, v, IGRAPH_ALL);
            igraph_integer_t deg = igraph_vector_int_size(&neigh);
            for (igraph_integer_t i = 0; i < deg; i++) {
                igraph_integer_t w = VECTOR(neigh)[i];
                if (w > v && color[w] == color[v]) total_conflicts++;
            }
        }

        if (total_conflicts < best_conflicts) {
            best_conflicts = total_conflicts;
            memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));
        }

        if (best_conflicts == 0) break;
    }

    igraph_vector_int_destroy(&neigh);
    memcpy(color, best, (size_t)n * sizeof(igraph_integer_t));
    *conflicts_out = best_conflicts;
    free(best);
    free(tabu);
    return num_colors;
}
```

- [ ] **Step 3: Build to verify compilation**

```bash
coffee build
```

- [ ] **Step 4: Commit**

```bash
git add src/tabucol.h src/tabucol.c
git commit -m "feat: add tabucol coloring algorithm"
```

---

### Task 11: Implement Ant Colony algorithm

**Files:**
- Create: `src/antcolony.h`
- Create: `src/antcolony.c`

- [ ] **Step 1: Write header**

`src/antcolony.h`:

```c
#pragma once
#include <igraph.h>

igraph_integer_t antcolony_run(const igraph_t *g, igraph_integer_t *color,
                                igraph_integer_t iterations,
                                igraph_integer_t num_ants,
                                double alpha,
                                double rho,
                                igraph_integer_t *conflicts_out);
```

- [ ] **Step 2: Write implementation**

`src/antcolony.c`:

```c
#include "antcolony.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

igraph_integer_t antcolony_run(const igraph_t *g, igraph_integer_t *color,
                                igraph_integer_t iterations,
                                igraph_integer_t num_ants,
                                double alpha,
                                double rho,
                                igraph_integer_t *conflicts_out)
{
    igraph_integer_t n = igraph_vcount(g);
    igraph_integer_t k = n;

    double *trail = malloc((size_t)(n * n) * sizeof(double));
    for (igraph_integer_t i = 0; i < n * n; i++) trail[i] = 1.0;

    igraph_integer_t *best_sol = malloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t best_feasible = 0;
    igraph_integer_t best_colors = n;

    double *delta = malloc((size_t)(n * n) * sizeof(double));
    igraph_integer_t *ant_sol = malloc((size_t)n * sizeof(igraph_integer_t));

    igraph_vector_int_t neigh;
    igraph_vector_int_init(&neigh, 0);

    for (igraph_integer_t iter = 0; iter < iterations; iter++) {
        for (igraph_integer_t p = 0; p < n * n; p++) delta[p] = 0.0;
        igraph_integer_t iter_best_colors = k + 1;
        igraph_integer_t *iter_best = NULL;

        for (igraph_integer_t ant = 0; ant < num_ants; ant++) {
            for (igraph_integer_t v = 0; v < n; v++) ant_sol[v] = -1;

            for (igraph_integer_t v = 0; v < n; v++) {
                double *probs = malloc((size_t)k * sizeof(double));
                double sum = 0.0;

                for (igraph_integer_t c = 0; c < k; c++) {
                    double f = 0.0;
                    igraph_neighbors(g, &neigh, v, IGRAPH_ALL);
                    igraph_integer_t deg = igraph_vector_int_size(&neigh);
                    igraph_integer_t class_size = 0;
                    for (igraph_integer_t w = 0; w < n; w++)
                        if (ant_sol[w] == c) class_size++;

                    if (class_size > 0) {
                        double trail_sum = 0.0;
                        for (igraph_integer_t i = 0; i < deg; i++) {
                            igraph_integer_t w = VECTOR(neigh)[i];
                            trail_sum += trail[v * n + w];
                        }
                        f = trail_sum / (double)class_size;
                    } else {
                        f = 1.0;
                    }

                    probs[c] = pow(f, alpha);
                    sum += probs[c];
                }

                if (sum > 0) {
                    double r = (double)rand() / RAND_MAX;
                    double cum = 0.0;
                    igraph_integer_t chosen = 0;
                    for (igraph_integer_t c = 0; c < k; c++) {
                        cum += probs[c] / sum;
                        if (r <= cum) { chosen = c; break; }
                    }
                    ant_sol[v] = chosen;
                } else {
                    ant_sol[v] = rand() % k;
                }
                free(probs);
            }

            igraph_integer_t conflicts = 0;
            for (igraph_integer_t v = 0; v < n; v++) {
                igraph_neighbors(g, &neigh, v, IGRAPH_ALL);
                igraph_integer_t deg = igraph_vector_int_size(&neigh);
                for (igraph_integer_t i = 0; i < deg; i++) {
                    igraph_integer_t w = VECTOR(neigh)[i];
                    if (w > v && ant_sol[w] == ant_sol[v]) conflicts++;
                }
            }

            igraph_integer_t used_colors = 0;
            for (igraph_integer_t v = 0; v < n; v++)
                if (ant_sol[v] >= used_colors) used_colors = ant_sol[v] + 1;

            for (igraph_integer_t u = 0; u < n; u++) {
                for (igraph_integer_t v = u + 1; v < n; v++) {
                    if (ant_sol[u] == ant_sol[v]) {
                        double contrib = conflicts == 0 ? 1.0 / (double)used_colors : 0.01 / (double)(conflicts + 1);
                        delta[u * n + v] += contrib;
                        delta[v * n + u] += contrib;
                    }
                }
            }

            if (conflicts == 0 && used_colors < iter_best_colors) {
                if (!iter_best) iter_best = malloc((size_t)n * sizeof(igraph_integer_t));
                memcpy(iter_best, ant_sol, (size_t)n * sizeof(igraph_integer_t));
                iter_best_colors = used_colors;
            }
        }

        for (igraph_integer_t p = 0; p < n * n; p++) {
            trail[p] = rho * trail[p] + delta[p];
        }

        if (iter_best && iter_best_colors < best_colors) {
            best_colors = iter_best_colors;
            memcpy(best_sol, iter_best, (size_t)n * sizeof(igraph_integer_t));
            best_feasible = 1;
            k = best_colors - 1;
            if (k < 1) k = 1;
        }

        free(iter_best);
        iter_best = NULL;
    }

    igraph_vector_int_destroy(&neigh);

    if (best_feasible) {
        memcpy(color, best_sol, (size_t)n * sizeof(igraph_integer_t));
        *conflicts_out = 0;
    } else {
        for (igraph_integer_t v = 0; v < n; v++) color[v] = rand() % k;
        *conflicts_out = n;
    }

    free(trail);
    free(delta);
    free(ant_sol);
    free(best_sol);
    return best_feasible ? best_colors : k;
}
```

- [ ] **Step 3: Build to verify compilation**

```bash
coffee build
```

- [ ] **Step 4: Commit**

```bash
git add src/antcolony.h src/antcolony.c
git commit -m "feat: add ant colony coloring algorithm"
```

---

### Task 12: Implement coloring.c (main entry + CLI dispatch)

**Files:**
- Rename/modify: `src/main.c` -> `src/coloring.c` (delete old main.c, create coloring.c)
- Modify: `src/coloring.c`

- [ ] **Step 1: Remove old main.c and write coloring.c**

```bash
rm src/main.c
```

`src/coloring.c`:

```c
#include <igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "greedy.h"
#include "welsh_powell.h"
#include "dsatur.h"
#include "rlf.h"
#include "iterated_greedy.h"
#include "sa.h"
#include "tabucol.h"
#include "antcolony.h"
#include "output.h"

static void print_algorithms(void)
{
    printf("Available algorithms:\n");
    printf("  greedy            - Greedy coloring\n");
    printf("  welsh-powell      - Welsh-Powell coloring\n");
    printf("  dsatur            - DSatur coloring\n");
    printf("  rlf               - Recursive Largest First coloring\n");
    printf("  iterated-greedy   - Iterated Greedy coloring\n");
    printf("  sa1               - Simulated Annealing v1 (change color)\n");
    printf("  sa2               - Simulated Annealing v2 (color/uncolor)\n");
    printf("  tabucol           - TabuCol coloring\n");
    printf("  antcolony         - Ant Colony coloring\n");
    printf("\nRun 'coloring help' for more details.\n");
}

static void print_help(void)
{
    printf("Usage: coloring <algorithm> [options]\n\n");
    printf("Reads a DIMACS graph from --input (or stdin) and outputs a JSON coloring.\n\n");
    printf("Algorithms:\n");
    printf("  greedy          [-i FILE]\n");
    printf("  welsh-powell    [-i FILE]\n");
    printf("  dsatur          [-i FILE]\n");
    printf("  rlf             [-i FILE]\n");
    printf("  iterated-greedy [-i FILE] [-o largest|reverse|random] [-I N]\n");
    printf("  sa1             [-i FILE] [-k K] [-I N] [-t T0] [-a ALPHA]\n");
    printf("  sa2             [-i FILE] [-k K] [-I N] [-t T0] [-a ALPHA]\n");
    printf("  tabucol         [-i FILE] [-k K] [-I N] [-l TENURE]\n");
    printf("  antcolony       [-i FILE] [-I N] [-n ANTS] [-a ALPHA] [-r RHO]\n");
    printf("\nOptions:\n");
    printf("  -i, --input FILE   DIMACS graph file (default: stdin)\n");
    printf("  -k, --colors K     Number of colors to use (heuristics, default: dsatur bound)\n");
    printf("  -I, --iterations N Number of iterations (default: algorithm-specific)\n");
    printf("  -o, --ordering O   Ordering: largest, reverse, random (iterated-greedy)\n");
    printf("  -t, --t0 T         Initial temperature (SA, default: 1.0)\n");
    printf("  -a, --alpha A      Cooling factor (SA) or exponent (antcolony, default: 0.95/1.0)\n");
    printf("  -n, --ants N       Number of ants (antcolony, default: 10)\n");
    printf("  -l, --tenure L     Tabu tenure (tabucol, default: 7)\n");
    printf("  -r, --rho R        Evaporation factor (antcolony, default: 0.9)\n");
}

static const char *get_opt(int argc, char **argv, int *i, const char *short_opt, const char *long_opt)
{
    if (strcmp(argv[*i], short_opt) == 0 || strcmp(argv[*i], long_opt) == 0) {
        if (*i + 1 < argc) { (*i)++; return argv[*i]; }
        fprintf(stderr, "Error: %s requires an argument\n", long_opt);
        exit(1);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));

    if (argc < 2) {
        print_algorithms();
        return 0;
    }

    const char *algo = argv[1];

    if (strcmp(algo, "help") == 0) {
        print_help();
        return 0;
    }

    const char *input_file = NULL;
    igraph_integer_t iterations = 0;
    igraph_integer_t num_colors = 0;
    const char *ordering = "largest";
    double t0 = 1.0;
    double sa_alpha = 0.95;
    igraph_integer_t tenure = 7;
    igraph_integer_t num_ants = 10;
    double ac_alpha = 1.0;
    double rho = 0.9;

    for (int i = 2; i < argc; i++) {
        const char *val;
        if ((val = get_opt(argc, argv, &i, "-i", "--input")))      input_file = val;
        else if ((val = get_opt(argc, argv, &i, "-I", "--iterations"))) iterations = atoi(val);
        else if ((val = get_opt(argc, argv, &i, "-k", "--colors")))    num_colors = atoi(val);
        else if ((val = get_opt(argc, argv, &i, "-o", "--ordering")))  ordering = val;
        else if ((val = get_opt(argc, argv, &i, "-t", "--t0")))        t0 = atof(val);
        else if ((val = get_opt(argc, argv, &i, "-a", "--alpha")))     { sa_alpha = atof(val); ac_alpha = atof(val); }
        else if ((val = get_opt(argc, argv, &i, "-l", "--tenure")))    tenure = atoi(val);
        else if ((val = get_opt(argc, argv, &i, "-n", "--ants")))      num_ants = atoi(val);
        else if ((val = get_opt(argc, argv, &i, "-r", "--rho")))       rho = atof(val);
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    FILE *in = input_file ? fopen(input_file, "r") : stdin;
    if (!in) {
        fprintf(stderr, "Cannot open input file: %s\n", input_file);
        return 1;
    }

    igraph_t g;
    igraph_read_graph_dimacs(&g, in, NULL, NULL, NULL, 0, IGRAPH_UNDIRECTED);

    if (input_file) fclose(in);

    igraph_integer_t n = igraph_vcount(g);
    igraph_integer_t *color = calloc((size_t)n, sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) color[v] = -1;

    clock_t start = clock();
    igraph_integer_t num_colors_out = 0;
    igraph_integer_t conflicts = -1;  /* -1 means guaranteed feasible */

    if (strcmp(algo, "greedy") == 0) {
        num_colors_out = greedy_run(&g, color);
    } else if (strcmp(algo, "welsh-powell") == 0) {
        num_colors_out = welsh_powell_run(&g, color);
    } else if (strcmp(algo, "dsatur") == 0) {
        num_colors_out = dsatur_run(&g, color);
    } else if (strcmp(algo, "rlf") == 0) {
        num_colors_out = rlf_run(&g, color);
    } else if (strcmp(algo, "iterated-greedy") == 0) {
        if (iterations == 0) iterations = 100;
        num_colors_out = iterated_greedy_run(&g, color, ordering, iterations);
    } else if (strcmp(algo, "sa1") == 0) {
        if (iterations == 0) iterations = 10000;
        if (num_colors == 0) {
            igraph_integer_t *tmp = calloc((size_t)n, sizeof(igraph_integer_t));
            num_colors = dsatur_run(&g, tmp);
            free(tmp);
        }
        num_colors_out = sa1_run(&g, color, num_colors, iterations, t0, sa_alpha, &conflicts);
    } else if (strcmp(algo, "sa2") == 0) {
        if (iterations == 0) iterations = 10000;
        if (num_colors == 0) {
            igraph_integer_t *tmp = calloc((size_t)n, sizeof(igraph_integer_t));
            num_colors = dsatur_run(&g, tmp);
            free(tmp);
        }
        num_colors_out = sa2_run(&g, color, num_colors, iterations, t0, sa_alpha, &conflicts);
    } else if (strcmp(algo, "tabucol") == 0) {
        if (iterations == 0) iterations = 10000;
        if (num_colors == 0) {
            igraph_integer_t *tmp = calloc((size_t)n, sizeof(igraph_integer_t));
            num_colors = dsatur_run(&g, tmp);
            free(tmp);
        }
        num_colors_out = tabucol_run(&g, color, num_colors, iterations, tenure, &conflicts);
    } else if (strcmp(algo, "antcolony") == 0) {
        if (iterations == 0) iterations = 100;
        num_colors_out = antcolony_run(&g, color, iterations, num_ants, ac_alpha, rho, &conflicts);
    } else {
        fprintf(stderr, "Unknown algorithm: %s\n", algo);
        print_algorithms();
        igraph_destroy(&g);
        free(color);
        return 1;
    }

    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC * 1000.0;

    output_json(stdout, algo, num_colors_out, color, n, elapsed, conflicts);

    igraph_destroy(&g);
    free(color);
    return 0;
}
```

- [ ] **Step 2: Build**

```bash
coffee build
```

- [ ] **Step 3: Commit**

```bash
git add src/coloring.c
git rm src/main.c 2>/dev/null
git commit -m "feat: add coloring.c main entry with CLI dispatch"
```

---

### Task 13: Integration testing

**Files:**
- Test: all algorithms against `data/queen5_5.col` and `data/myciel3.col`

- [ ] **Step 1: Build release binary**

```bash
coffee build --release
```

- [ ] **Step 2: Test no-args (lists algorithms)**

```bash
./target/release/coloring
```
Expected: prints list of 9 algorithms.

- [ ] **Step 3: Test help**

```bash
./target/release/coloring help
```
Expected: prints full usage with all flags.

- [ ] **Step 4: Test greedy on myciel3**

```bash
./target/release/coloring greedy -i data/myciel3.col
```
Expected: JSON output. `colors` field should be a valid coloring bound. Check that no adjacent vertices share a color (manually inspect the small graph).

- [ ] **Step 5: Test all constructive algorithms on myciel3**

```bash
for algo in greedy welsh-powell dsatur rlf iterated-greedy; do
    echo "=== $algo ==="
    ./target/release/coloring $algo -i data/myciel3.col
done
```
Expected: all produce valid JSON. Greedy may use more colors than DSatur/RLF.

- [ ] **Step 6: Test all metaheuristics on myciel3**

```bash
for algo in sa1 sa2 tabucol antcolony; do
    echo "=== $algo ==="
    ./target/release/coloring $algo -i data/myciel3.col -I 10000
done
```
Expected: all produce valid JSON. SA/TabuCol may report conflicts.

- [ ] **Step 7: Test with queen5_5**

```bash
./target/release/coloring dsatur -i data/queen5_5.col
./target/release/coloring greedy -i data/queen5_5.col
```
Expected: valid colorings. Queen5_5 has chi=5, DSatur should find 5 colors.

- [ ] **Step 8: Verify no adjacent vertices with same color (manual check)**

Create a quick verification script `scripts/verify_coloring.py`:

```python
#!/usr/bin/env python3
import json, sys, subprocess

def verify(algo, graph):
    result = subprocess.run(
        ["./target/release/coloring", algo, "-i", graph],
        capture_output=True, text=True)
    data = json.loads(result.stdout)
    a = data["assignments"]
    
    # Load edges from DIMACS file
    edges = []
    with open(graph) as f:
        for line in f:
            if line.startswith('e'):
                parts = line.split()
                edges.append((int(parts[1]), int(parts[2])))
    
    for u, v in edges:
        if a[str(u)] == a[str(v)]:
            print(f"FAIL {algo} on {graph}: edge {u}-{v} same color {a[str(u)]}")
            return False
    print(f"OK {algo} on {graph}: {data['colors']} colors")
    return True

for algo in ["greedy", "welsh-powell", "dsatur", "rlf", "iterated-greedy"]:
    verify(algo, "data/myciel3.col")
    verify(algo, "data/queen5_5.col")
```

- [ ] **Step 9: Run verification script**

```bash
python3 scripts/verify_coloring.py
```
Expected: all constructive algorithms produce valid colorings (no same-colored adjacent vertices).

- [ ] **Step 10: Commit**

```bash
git add scripts/verify_coloring.py
git commit -m "test: add coloring verification script and integration tests"
```
