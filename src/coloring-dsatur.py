#!/usr/bin/env python3

import networkx as nx
import argparse
import pdb

parser = argparse.ArgumentParser()
parser.add_argument("filename", help="graph_filename")
args = parser.parse_args()

g = nx.read_edgelist(args.filename, nodetype=int)

color = {}
num_colors = 0
# saturation = the number of colors of the neighbors of a vertex
# at the beginning it is 0 for all verties, since no vertex is colored
saturation = dict([(v, 0) for v in g.nodes()])
# uncolored_degree = for each uncolored vertex, the number of its neighbors
# that are uncolored.
# At the beginning it is equal to its degree
uncolored_degree = dict(g.degree())
n = len(g.nodes())

while len(uncolored_degree) > 0:
    # We need to find the uncolored vertex with maximum saturation
    # Among all vertices with maximum saturation, we pick one maximizing the
    # number of neighbors that are uncolored.
    # This is equivalend to maximizing saturation * (N+1) + D, where N is the
    # number of uncolored vertices and D is the number of uncolored neighbors
    max_saturation = -1
    best = -1
    for v in uncolored_degree.keys():
        sat_v = saturation[v] * (len(uncolored_degree.keys()) + 1) + uncolored_degree[v]
        if sat_v > max_saturation:
            best = v
            max_saturation = sat_v
    # update data structures
    neighbor_colors = [color[w] for w in nx.neighbors(g, best) if w in color]
    color[best] = min(set(range(num_colors + 1)) - set(neighbor_colors))
    num_colors = max(num_colors, color[best] + 1)
    for w in nx.neighbors(g, best):
        saturation[w] = len(set([color[z] for z in nx.neighbors(g, w) if z in color]))
        if w in uncolored_degree:
            uncolored_degree[w] -= 1
    del saturation[best]
    del uncolored_degree[best]
    todo = len(uncolored_degree.keys())
    if todo % 100 == 0:
        print(f"colored {best}, still {todo} ({int(todo / n * 100)})")

print(f"Colors used: {num_colors}")
