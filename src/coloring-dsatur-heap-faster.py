#!/usr/bin/env python3

import networkx as nx
import argparse
import heapdict
import pdb

parser = argparse.ArgumentParser()
parser.add_argument("filename", help="graph_filename")
args = parser.parse_args()

g = nx.read_edgelist(args.filename, nodetype=int)

color = {}
num_colors = 0
# We need to find the uncolored vertex with maximum saturation
# Among all vertices with maximum saturation, we pick one maximizing the
# number of neighbors that are uncolored.
# This is equivalent to maximizing saturation * N + D, where N is the
# number of vertices and D is the number of uncolored neighbors
criterion = heapdict.heapdict()
for (v, d) in nx.degree(g):
    criterion[v] = -d
n = len(g.nodes())
todo = n

colors_neighbors = dict([(v, set()) for v in g.nodes()])

while len(criterion) > 0:
    best, max_criterion = criterion.popitem()
    # update data structure, considering that best will become colored, so all
    # its neighbors must be updated.
    neighbor_colors = [color[w] for w in nx.neighbors(g, best) if w in color]
    color[best] = min(set(range(num_colors + 1)) - set(neighbor_colors))
    num_colors = max(num_colors, color[best] + 1)
    for w in nx.neighbors(g, best):
        if not w in color:
            if color[best] in colors_neighbors[w]:
                # In this case best has the same color as one of the neighbors of w,
                # hence the saturation of w has not changed, only the number of
                # uncolored neighbors
                criterion[w] = criterion[w] + 1
            else:
                # In this case best has a different color from those of the neighbors of w,
                # hence the saturation of w increases by 1,
                # this corresponds to an increase by n of criterion.
                # Moreover, the number of uncolored neighbors decreases by 1
                criterion[w] = criterion[w] + 1 - n
            colors_neighbors[w].add(color[best])

    del colors_neighbors[best]
    todo -= 1
    if todo % 100 == 0:
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
