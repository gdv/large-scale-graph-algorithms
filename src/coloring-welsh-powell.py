#!/usr/bin/env python3

import networkx as nx
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("filename", help="graph_filename")
args = parser.parse_args()

g = nx.read_edgelist(args.filename, nodetype=int)
n = len(g.nodes())
todo = n

color = {}
num_colors = 0
sorted_vertices = [x[0] for x in sorted(g.degree(), key=lambda x: x[1], reverse=True)]

for v in sorted_vertices:
    neighbor_colors = [color[w] for w in nx.neighbors(g, v) if w in color]
    color[v] = min(set(range(num_colors + 1)) - set(neighbor_colors))
    num_colors = max(num_colors, color[v] + 1)
    if num_colors != len(set(color.values())):
        print(f"vertex: {best}")
        print(f"neighbors : {list(nx.neighbors(g, best))}")
        print(f"colors: {neighbor_colors}")
        print(f"new color: {color[best]}")
        print(
            f"list of colors: {set(color.values())}, {num_colors}={len(set(color.values()))}"
        )
        quit()
    todo -= 1
    if todo % 100 == 0:
        print(f"still {todo} ({int(todo / n * 100)})")

print(f"Colors used: {len(set(color.values()))}")
