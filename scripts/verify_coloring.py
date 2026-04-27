#!/usr/bin/env python3
import json, sys, subprocess

def verify(algo, graph):
    result = subprocess.run(
        ["./target/debug/coloring", algo, "-i", graph],
        capture_output=True, text=True)
    if result.returncode != 0:
        print(f"FAIL {algo} on {graph}: exit code {result.returncode}")
        print(result.stderr)
        return False
    data = json.loads(result.stdout)
    a = data["assignments"]
    edges = []
    with open(graph) as f:
        for line in f:
            if line.startswith('e'):
                parts = line.split()
                edges.append((int(parts[1]), int(parts[2])))
    for u, v in edges:
        u0, v0 = str(u - 1), str(v - 1)
        if u0 in a and v0 in a and a[u0] == a[v0]:
            print(f"FAIL {algo} on {graph}: edge {u}-{v} same color {a[u0]}")
            return False
    print(f"OK {algo} on {graph}: {data['colors']} colors")
    return True

for algo in ["greedy", "welsh-powell", "dsatur", "rlf", "iterated-greedy"]:
    verify(algo, "data/myciel3.col")
    verify(algo, "data/queen5_5.col")
