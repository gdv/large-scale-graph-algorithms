#include <igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flow.h"
#include "graph_io.h"
#include "output.h"
#include "util.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s -i FILE -s SOURCE -t TARGET -a ALGO\n"
        "  ALGO: ff | ek | dinic | preflowpush\n", prog);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *input = NULL, *algo = NULL;
    igraph_integer_t source = 0, target = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc) input = argv[++i];
        else if (!strcmp(argv[i], "-a") && i + 1 < argc) algo = argv[++i];
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) source = atoll(argv[++i]);
        else if (!strcmp(argv[i], "-t") && i + 1 < argc) target = atoll(argv[++i]);
        else usage(argv[0]);
    }
    if (!input || !algo) usage(argv[0]);

    igraph_set_attribute_table(&igraph_cattribute_table);
    phase_begin("read");
    igraph_t g = read_graph_or_die(input, 0, true);
    ensure_directed(&g);
    ensure_weight_attr(&g);   /* "weight" doubles as the capacity */
    igraph_integer_t n = (igraph_integer_t)igraph_vcount(&g);
    igraph_integer_t m = (igraph_integer_t)igraph_ecount(&g);
    phase_end();
    phase_begin("build");            /* residual network: 2 paired arcs per edge */
    flow_t f;
    flow_init(&f, n, m);
    igraph_vector_int_t elist;
    igraph_vector_int_init(&elist, 0);
    igraph_get_edgelist(&g, &elist, false);
    for (igraph_integer_t k = 0; k < m; k++) {
        igraph_real_t cap = igraph_cattribute_EAN(&g, "weight", k);
        flow_add_edge(&f, VECTOR(elist)[2 * k], VECTOR(elist)[2 * k + 1], cap);
    }
    igraph_vector_int_destroy(&elist);
    igraph_destroy(&g);
    phase_end();

    if (source < 0 || source >= n || target < 0 || target >= n || source == target) {
        fprintf(stderr,
                "invalid s-t pair (%" IGRAPH_PRId ", %" IGRAPH_PRId ") for n=%" IGRAPH_PRId "\n",
                source, target, n);
        flow_destroy(&f);
        return 1;
    }

    igraph_real_t (*run)(flow_t *, igraph_integer_t, igraph_integer_t, igraph_real_t *) = NULL;
    if (!strcmp(algo, "ff"))                   run = flow_ford_fulkerson;
    else if (!strcmp(algo, "ek"))              run = flow_edmonds_karp;
    else if (!strcmp(algo, "dinic"))           run = flow_dinic;
    else if (!strcmp(algo, "preflowpush"))     run = flow_preflow_push;
    else usage(argv[0]);

    igraph_real_t *flow = xmalloc((size_t)m * sizeof(igraph_real_t));
    phase_begin("solve");
    igraph_real_t value = run(&f, source, target, flow);
    phase_end();

    bool *side = flow_mincut_side(&f, source);
    /* Cut capacity = sum of the ORIGINAL capacity of the edges leaving the
     * s side. Two traps here, both worth writing down:
     *  - only the S -> T direction of an edge crosses the cut. Summing over
     *    "endpoints on different sides" would also count the T -> S arcs;
     *  - the original capacity is cap[2k] + cap[2k+1] (residual + flow).
     * Iterating residual arcs instead double-counts the antiparallel pairs
     * that ensure_directed() creates from an undirected input. */
    igraph_real_t cut_cap = 0.0;
    for (igraph_integer_t k = 0; k < f.m; k++) {
        if (side[f.to[2 * k + 1]] && !side[f.to[2 * k]])
            cut_cap += f.cap[2 * k] + f.cap[2 * k + 1];
    }

    printf("{\"algorithm\": \"%s\", \"source\": %" IGRAPH_PRId
           ", \"target\": %" IGRAPH_PRId ", \"value\": %g, \"cut_capacity\": %g",
           algo, source, target, value, cut_cap);
    metrics_json(stdout);
    printf("\n}\n");

    xfree(side);
    xfree(flow);
    flow_destroy(&f);
    return 0;
}
