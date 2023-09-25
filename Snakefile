import pandas as pd
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
]

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
        source = lambda g: int(graphs[graphs.graph == g.graph].source.iloc[0]),
        target = lambda g: int(graphs[graphs.graph == g.graph].target.iloc[0]),
    shell:
        "/usr/bin/time -v src/shortest-path.py --graph={input}  --source={params.source}  --target={params.target} 2> {output}"

rule st_shortest_path_petgraph:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-st_shortest_path_petgraph.txt"
    params:
        source = lambda g: int(graphs[graphs.graph == g.graph].source.iloc[0]),
        target = lambda g: int(graphs[graphs.graph == g.graph].target.iloc[0]),
    shell:
        "/usr/bin/time -v src/target/release/st-shortest-path-petgraph --graph={input}  --source={params.source}  --target={params.target} 2> {output}"

rule st_shortest_path:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-st_shortest_path.txt"
    params:
        source = lambda g: int(graphs[graphs.graph == g.graph].source.iloc[0]),
        target = lambda g: int(graphs[graphs.graph == g.graph].target.iloc[0]),
    shell:
        "/usr/bin/time -v src/target/release/st-shortest-path --graph={input}  --source={params.source}  --target={params.target} 2> {output}"

rule st_shortest_path_bidirectional:
    input:
        "data/{graph}.csv.gz",
    output:
        "results/{graph}-st_shortest_path_bidirectional.txt"
    params:
        source = lambda g: int(graphs[graphs.graph == g.graph].source.iloc[0]),
        target = lambda g: int(graphs[graphs.graph == g.graph].target.iloc[0]),
    shell:
        "/usr/bin/time -v src/target/release/st-shortest-path-bidirectional --graph={input}  --source={params.source}  --target={params.target} 2> {output}"

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
