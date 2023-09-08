import pandas as pd
from itertools import product

graphs = pd.read_csv("data/datasets.csv").set_index("graph", drop=False)
graphs["full_graph"] = graphs["graph"] + graphs["ext"] + '.gz'
#graphs["exe"] = graphs["exe"].apply(lambda x: x if x is None else "ren.sh")


programs = [
    {
        "name": "dfspy",
        "exe": "src/dfs.py",
        "group": "connectivity",
        "directed": [0],
        "weighted": [0]
    },
    {
        "name": "dfsrs",
        "exe": "target/release/dfs",
        "group": "connectivity",
        "directed": [0],
        "weighted": [0]
    },
]
program_names = [p['name'] for p in programs]

print(graphs)
print(program_names)
print('------------------------------')

def join_graphs_prgs(*args, **kwargs):
        for (graph, prg) in product(*args, **kwargs):
            g = dict(graphs[graphs['graph'] ==  graph[1]].iloc[0])
            p = [q for q in programs if prg[1] == q["name"]][0]
            print (g)
            print(p)
            print('----------------------')

            print(g['Directed'] in p['directed'])
            print(g['Weighted'] in p['weighted'])
            print(g[p["group"]])
            print('----------------------')
            if g['Directed'] in p['directed'] and g['Weighted'] in p['weighted'] and g[p["group"]]:
                yield (graph, prg)

rule all:
    input:
        expand("results/{graph}-{prg}.txt", join_graphs_prgs, graph=graphs.graph, prg=program_names)


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

rule downloads:
    output:
        "data/{graph}.csv.gz",
    params:
        exe = lambda g: graphs[graphs.graph == g.graph].exe.iloc[0],
        url = lambda g: graphs[graphs.graph == g.graph].url.iloc[0],
    shell:
        "curl {params.url} | zcat - | data/{params.exe} | pigz > {output}"
# rule from_gr:
#     input:
#         "data/{graph}.gr.gz"
#     output:
#         "data/{graph}.tsv.gz"
#     shell:
#         "..."
