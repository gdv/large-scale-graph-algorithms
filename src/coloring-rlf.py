#!/usr/bin/env python3

import networkx as nx
import argparse
import collections

parser = argparse.ArgumentParser()
parser.add_argument("filename", help="graph_filename")
args = parser.parse_args()

g = nx.read_edgelist(args.filename, nodetype=int)

color = {}
num_colors = 0
# The RLS algorithm finds a maximal indepenedent set and assigns a new color to
# all vertices in such set
# The independent set must contain the vertex with largest degree

n = len(g.nodes())
todo = n

while len(g.nodes()) > 0:
    (v, d) = max(nx.degree(g), key=lambda x: x[1])
    independent_set = nx.maximal_independent_set(g, [v])
    for w in independent_set:
        color[w] = num_colors
    g.remove_nodes_from(independent_set)
    print(f"Removing {independent_set}")
    num_colors += 1

    todo -= len(independent_set)
    print(f"still {todo} ({int(todo / n * 100)})")

    if num_colors != len(set(color.values())):
        print(f"vertex: {best}")
        print(f"neighbors : {list(nx.neighbors(g, best))}")
        print(f"colors: {neighbor_colors}")
        print(f"new color: {color[best]}")
        print(
            f"list of colors: {set(color.values())}, {num_colors}={len(set(color.values()))}"
        )
        quit()

print(f"Colors used: {len(set(color.values()))}")
