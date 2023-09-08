#!/usr/bin/env python3

import networkx as nx
import argparse
import pathlib

parser = argparse.ArgumentParser()
parser.add_argument(
    "-g", "--graph", required=True, type=pathlib.Path, help="graph_filename"
)
parser.add_argument("-s", "--source", required=True, type=int, help="source vertex")
parser.add_argument("-t", "--target", required=True, type=int, help="target vertex")
args = parser.parse_args()

g = nx.read_weighted_edgelist(
    args.graph, nodetype=int, delimiter="\t", create_using=nx.DiGraph
)
n = len(g.nodes())

print(f"Number of vertices: {n}")
print(f"Number of edges: {len(g.edges())}")
path_nodes = nx.dijkstra_path(g, source=args.source, target=args.target)
path_edges = nx.utils.pairwise(path_nodes)

tot_cost = 0
for v, w in path_edges:
    d = g[v][w]["weight"]
    tot_cost += d
    print(f"{v} -> {w} with cost {d} at distance {tot_cost}")
print(f"Total cost: {tot_cost}")
