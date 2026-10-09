import glob
import os
import sys
import pandas as pd

# pandas/matplotlib live in the project pixi env, which is NOT necessarily the
# env snakemake itself was installed in. Prefer the project interpreter and
# fall back to snakemake's own.
PY = (".pixi/envs/default/bin/python"
      if os.path.exists(".pixi/envs/default/bin/python") else sys.executable)
from itertools import product

######################################################
# We need to read the file "data/datasets.csv" which contains the list of
# datasets that we want to process and the *groups* of program that we want to run
#
# Each file must be in a csv or tsv format, possibly compressed with gzip
######################################################

graphs = pd.read_csv("data/datasets.csv").set_index("graph", drop=False)
graphs["full_graph"] = graphs["graph"] + "." + graphs["ext"] + '.gz'
graphs_dict = graphs.to_dict('index')
#graphs["exe"] = graphs["exe"].apply(lambda x: x if x is None else "ren.sh")

# The *programs* list contains all programs that we have developed and we want
# to run on some dataset
# Each program must have the following fields:
#     "name": the unique ID
#     "exe":  full path to the program
#     "group": each program belongs to exactly one group
#     "directed": List with 0=undirected, 1=directed graph.
#     "weighted": List with 0=unweighted, 1=edge weighted graph.
programs = [
    # {
    #     "name": "dfspy",
    #     "exe": "src/dfs.py",
    #     "group": "connectivity",
    #     "directed": [0],
    #     "weighted": [0]
    # },
    {
        "name": "dfsrs",
        "exe": "src/target/release/dfs",
        "group": "connectivity",
        "directed": [0],
        "weighted": [0]
    },
    {
        "name": "bfsrs",
        "exe": "src/target/release/bfs",
        "group": "connectivity",
        "directed": [0],
        "weighted": [0]
    },
    {
        "name": "shortestnx",
        "exe": "src/shortest-path.py",
        "group": "shortest",
        "directed": [1],
        "weighted": [1]
    },
    {
        "name": "st_shortest_path_bidirectional",
        "exe": "src/target/release/st-shortest-path-bidirectional",
        "group": "shortest",
        "directed": [1],
        "weighted": [1]
    },
    {
        "name": "st_shortest_path",
        "exe": "src/target/release/st-shortest-path",
        "group": "shortest",
        "directed": [1],
        "weighted": [1]
    },
    {
        "name": "st_shortest_path_petgraph",
        "exe": "src/target/release/st-shortest-path-petgraph",
        "group": "shortest",
        "directed": [1],
        "weighted": [1]
    },
    {
        "name": "dijkstra-igraph",
        "exe": "bin/dijkstra-igraph",
        "group": "shortest",
        "directed": [1],
        "weighted": [0, 1]
    },
]

# Generate all adj × pq combinations for the pluggable Dijkstra
adj_backends = ["array", "sorted", "linked", "hash"]
pq_backends = ["binary", "unsorted", "dary"]
for adj in adj_backends:
    for pq in pq_backends:
        programs.append({
            "name": f"dijkstra-{adj}-{pq}",
            "exe": "bin/dijkstra",
            "group": "shortest",
            "directed": [1],
            "weighted": [0, 1]
        })

# Local graphs for the flow / matching / compress / randomized families.
# They are generated, not downloaded: deterministic, tiny, and they cover the
# structures the public datasets lack (bipartite, capacitated, clustered).
local_bench_graphs = pd.DataFrame([
    {"graph": "bip-grid-32", "Nodes": 1024, "Edges": 4008, "Directed": 0, "Weighted": 1,
     "ext": "ncol", "url": "", "Description": "bipartite 32x32 grid + chords",
     "Origin": "local", "Website": "", "exe": "", "connectivity": 0, "coloring": 0,
     "flow": 0, "shortest": 0, "matching": 1, "compress": 0, "randomized": 0,
     "source": "", "target": ""},
    {"graph": "flow-layered-8x64", "Nodes": 386, "Edges": 1078, "Directed": 1, "Weighted": 1,
     "ext": "ncol", "url": "", "Description": "layered capacitated network",
     "Origin": "local", "Website": "", "exe": "", "connectivity": 0, "coloring": 0,
     "flow": 1, "shortest": 0, "matching": 0, "compress": 0, "randomized": 0,
     "source": 0, "target": 385},
    {"graph": "clustered-64x256", "Nodes": 16384, "Edges": 143916, "Directed": 0, "Weighted": 0,
     "ext": "ncol", "url": "", "Description": "64 clusters of 256 vertices",
     "Origin": "local", "Website": "", "exe": "", "connectivity": 1, "coloring": 1,
     "flow": 0, "shortest": 0, "matching": 0, "compress": 1, "randomized": 1,
     "source": 0, "target": ""},
    {"graph": "dense-256", "Nodes": 256, "Edges": 9947, "Directed": 0, "Weighted": 0,
     "ext": "ncol", "url": "", "Description": "random graph, p=0.3",
     "Origin": "local", "Website": "", "exe": "", "connectivity": 1, "coloring": 1,
     "flow": 0, "shortest": 0, "matching": 0, "compress": 1, "randomized": 1,
     "source": 0, "target": ""},
])

# Add local DIMACS .col graph files for coloring
local_col_graphs = pd.DataFrame([
    {"graph": "queen5_5", "Nodes": 25, "Edges": 160, "Directed": 0, "Weighted": 0,
     "ext": "col", "url": "", "Description": "5x5 queen graph", "Origin": "local",
     "Website": "", "exe": "", "connectivity": 0, "coloring": 1, "flow": 0, "shortest": 0,
     "source": "", "target": ""},
    {"graph": "myciel3", "Nodes": 11, "Edges": 20, "Directed": 0, "Weighted": 0,
     "ext": "col", "url": "", "Description": "Mycielski graph on C5", "Origin": "local",
     "Website": "", "exe": "", "connectivity": 0, "coloring": 1, "flow": 0, "shortest": 0,
        "source": "", "target": ""},
])
graphs = pd.concat([graphs, local_col_graphs, local_bench_graphs], ignore_index=True)

# Coloring algorithms
coloring_algos = [
    ("coloring-greedy", "greedy"),
    ("coloring-welsh-powell", "welsh-powell"),
    ("coloring-dsatur", "dsatur"),
    ("coloring-rlf", "rlf"),
    ("coloring-iterated-greedy", "iterated-greedy"),
    ("coloring-sa1", "sa1"),
    ("coloring-sa2", "sa2"),
    ("coloring-tabucol", "tabucol"),
    ("coloring-antcolony", "antcolony"),
]
for name, algo in coloring_algos:
    programs.append({
        "name": name,
        "exe": "bin/coloring",
        "group": "coloring",
        "directed": [0],
        "weighted": [0]
    })

# Traversal & connectivity (C, on CSR)
search_algos = ["bfs", "dfs", "astar", "ap", "biconnected", "bridges", "edge2"]
for algo in search_algos:
    programs.append({
        "name": f"search-{algo}",
        "exe": "bin/search",
        "group": "connectivity",
        "directed": [0],
        "weighted": [0, 1]
    })
programs.append({   # SCC is the one algorithm here that needs real directions
    "name": "search-scc",
    "exe": "bin/search",
    "group": "connectivity",
    "directed": [1],
    "weighted": [0, 1]
})

# Max flow
for algo, key in [("ff", "ff"), ("ek", "ek"), ("dinic", "dinic"),
                  ("preflowpush", "preflowpush")]:
    programs.append({
        "name": f"flow-{algo}",
        "exe": "bin/flow",
        "group": "flow",
        "directed": [1],
        "weighted": [1]
    })

# Bipartite matching
for algo in ["hopcroftkarp", "viaflow", "hungarian"]:
    programs.append({
        "name": f"matching-{algo}",
        "exe": "bin/matching",
        "group": "matching",
        "directed": [0],
        "weighted": [0, 1]
    })

# Graph compression
for algo in ["gaps", "reference", "interval", "huffman", "mtf", "rle",
             "eliasg", "eliasd", "nibble", "minbin", "zetak"]:
    programs.append({
        "name": f"compress-{algo}",
        "exe": "bin/compress",
        "group": "compress",
        "directed": [0, 1],
        "weighted": [0, 1]
    })

# Randomized algorithms
for algo in ["maxcut", "karger", "kargerstein"]:
    programs.append({
        "name": f"randomized-{algo}",
        "exe": "bin/randomized",
        "group": "randomized",
        "directed": [0],
        "weighted": [0, 1]
    })

def _int_or(value, default=0):
    try:
        if value is None or value != value:      # NaN
            return default
        return int(value)
    except (TypeError, ValueError):
        return default

def graph_source(g):
    return _int_or(graphs[graphs.graph == g.graph].source.iloc[0])

def graph_target(g):
    return _int_or(graphs[graphs.graph == g.graph].target.iloc[0])

def bench_input(wc):
    """Local graphs live in data/local/, downloaded ones in data/."""
    local = f"data/local/{wc.graph}.ncol"
    if os.path.exists(local):
        return local
    return f"data/{wc.graph}.csv.gz"


program_names = [p['name'] for p in programs]

print(graphs)
print(graphs_dict)
print(program_names)
print('------------------------------')

def join_graphs_prgs(*args, **kwargs):
        """
        We join the lists of graphs and programs to produce the list of
        result files that we want to produce.
        """
        for (graph, prg) in product(*args, **kwargs):
            g = dict(graphs[graphs['graph'] ==  graph[1]].iloc[0])
            p = [q for q in programs if prg[1] == q["name"]][0]
            # print (g)
            # print(p)
            # print('----------------------')

            # print(g['Directed'] in p['directed'])
            # print(g['Weighted'] in p['weighted'])
            # print(g[p["group"]])
            # print('----------------------')
            if g['Directed'] in p['directed'] and g['Weighted'] in p['weighted'] and g[p["group"]]:
                yield (graph, prg)

rule all:
    input:
        expand("results/{graph}-{prg}.txt", join_graphs_prgs, graph=graphs.graph, prg=program_names)

#  We need a rule for each program, until I find a way to generate the output
#  and the shell parts from the program lists with the following rule.
#
#  rule program:
# input:
#     "data/{graph}.csv.gz",
# output:
#     "results/{graph}-{prg}.txt"
# shell:
#     "/usr/bin/time -v {prg.exe} {input} 2> {output}"

rule dfs_py:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-dfspy.txt"
    shell:
        "/usr/bin/time -v src/dfs.py {input} 2> {output}"

rule dfs_rs:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-dfsrs.txt"
    shell:
        "/usr/bin/time -v src/target/release/dfs --graph={input} 2> {output}"

rule bfs_rs:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-bfsrs.txt"
    shell:
        "/usr/bin/time -v src/target/release/bfs --graph={input} 2> {output}"

rule shortest_nx:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-shortestnx.txt"
    params:
        source = graph_source,
        target = graph_target,
    shell:
        "/usr/bin/time -v src/shortest-path.py --graph={input}  --source={params.source}  --target={params.target} 2> {output}"

rule st_shortest_path_petgraph:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-st_shortest_path_petgraph.txt"
    params:
        source = graph_source,
        target = graph_target,
    shell:
        "/usr/bin/time -v src/target/release/st-shortest-path-petgraph --graph={input}  --source={params.source}  --target={params.target} 2> {output}"

rule st_shortest_path:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-st_shortest_path.txt"
    params:
        source = graph_source,
        target = graph_target,
    shell:
        "/usr/bin/time -v src/target/release/st-shortest-path --graph={input}  --source={params.source}  --target={params.target} 2> {output}"

rule st_shortest_path_bidirectional:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-st_shortest_path_bidirectional.txt"
    params:
        source = graph_source,
        target = graph_target,
    shell:
        "/usr/bin/time -v src/target/release/st-shortest-path-bidirectional --graph={input}  --source={params.source}  --target={params.target} 2> {output}"

rule dijkstra_pluggable:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-dijkstra-{adj}-{pq}.txt"
    params:
        source = graph_source,
    wildcard_constraints:
        adj = "|".join(adj_backends),
        pq = "|".join(pq_backends),
    shell:
        "/usr/bin/time -v bin/dijkstra --graph={wildcards.adj} --pq={wildcards.pq} -i {input} -s {params.source} > {output}.json 2> {output}"

rule dijkstra_igraph:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-dijkstra-igraph.txt"
    params:
        source = graph_source,
    shell:
        "/usr/bin/time -v bin/dijkstra-igraph -i {input} -s {params.source} > {output}.json 2> {output}"

def coloring_input(wc):
    col = f"data/{wc.graph}.col"
    csv = f"data/{wc.graph}.csv.gz"
    if os.path.exists(col):
        return col
    return csv

rule coloring:
    input:
        coloring_input
    output:
        "results/{graph}-coloring-{algo}.txt"
    shell:
        "/usr/bin/time -v bin/coloring {wildcards.algo} -i {input} > {output}.json 2> {output}"

rule bench_graphs:
    output:
        expand("data/local/{g}.ncol", g=["bip-grid-32", "flow-layered-8x64",
                                         "clustered-64x256", "dense-256"])
    shell:
        f"{PY} scripts/gen_bench_graphs.py data/local"

# ---------------------------------------------------------------- flow -----
rule flow:
    input:
        bench_input
    output:
        "results/{graph}-flow-{algo}.txt"
    params:
        source = graph_source,
        target = graph_target,
    shell:
        "/usr/bin/time -v bin/flow -i {input} -s {params.source} -t {params.target} -a {wildcards.algo} > {output}.json 2> {output}"

# ------------------------------------------------------------ matching -----
rule matching:
    input:
        bench_input
    output:
        "results/{graph}-matching-{algo}.txt"
    shell:
        "/usr/bin/time -v bin/matching -i {input} -a {wildcards.algo} > {output}.json 2> {output}"

# ------------------------------------------------------------ compress -----
rule compress:
    input:
        bench_input
    output:
        "results/{graph}-compress-{algo}.txt"
    shell:
        "/usr/bin/time -v bin/compress -i {input} -a {wildcards.algo} > {output}.json 2> {output}"

# ----------------------------------------------------------- randomized -----
rule randomized:
    input:
        bench_input
    output:
        "results/{graph}-randomized-{algo}.txt"
    shell:
        "/usr/bin/time -v bin/randomized -i {input} -a {wildcards.algo} -I 20 > {output}.json 2> {output}"

# ------------------------------------------- traversal & connectivity (C) ---
rule search:
    input:
        bench_input
    output:
        "results/{graph}-search-{algo}.txt"
    params:
        source = graph_source,
        directed = lambda g: "--directed" if _int_or(graphs[graphs.graph == g.graph].Directed.iloc[0]) else "",
    shell:
        "/usr/bin/time -v bin/search -i {input} -s {params.source} -t {params.source} -a {wildcards.algo} {params.directed} > {output}.json 2> {output}"

def plot_coloring_input(wildcards):
    import glob
    return sorted(glob.glob("results/*-coloring-*.txt"))

rule plot_coloring:
    input:
        plot_coloring_input
    output:
        "results/coloring_plots.done"
    run:
        shell("/usr/bin/python3 scripts/benchmark_plots.py && touch {output}")

def plot_shortest_input(wildcards):
    import glob
    return sorted(glob.glob("results/*-dijkstra-*.txt"))

rule plot_shortest:
    input:
        plot_shortest_input
    output:
        "results/shortest_plots.done"
    run:
        shell("/usr/bin/python3 scripts/shortest_plots.py && touch {output}")

# One plot script for every family that reports JSON metrics (build vs solve
# time, peak/live bytes of the data structure, bits per edge, ...).
FAMILY_GLOBS = {
    "search":      "results/*-search-*.txt",
    "flow":        "results/*-flow-*.txt",
    "matching":    "results/*-matching-*.txt",
    "compress":    "results/*-compress-*.txt",
    "randomized":  "results/*-randomized-*.txt",
}

rule plot_bench:
    input:
        expand("results/{family}.bench_plots.done", family=FAMILY_GLOBS.keys())
    output:
        "results/bench_plots.done"
    shell:
        "touch {output}"

def bench_plot_input(wildcards):
    return sorted(glob.glob(FAMILY_GLOBS[wildcards.family]))

rule plot_bench_family:
    input:
        bench_plot_input
    output:
        "results/{family}.bench_plots.done"
    wildcard_constraints:
        family = "|".join(FAMILY_GLOBS.keys())
    shell:
        f"{PY} scripts/bench_plots.py --family {{wildcards.family}} && touch {{output}}"

#  The exe field of the datasets table contains the name of a script that
#  receives in stdin the file downloaded and prints the file in csv/tsv format
#  This rule also compresses the file.
rule downloads:
    output:
        "data/{graph}.csv.gz",
    params:
        exe = lambda g: graphs[graphs.graph == g.graph].exe.iloc[0],
        url = lambda g: graphs[graphs.graph == g.graph].url.iloc[0],
    shell:
        "curl {params.url} | zcat - | data/{params.exe} | pigz > {output}"
