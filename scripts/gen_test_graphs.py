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
