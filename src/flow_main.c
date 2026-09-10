#include <igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flow.h"
#include "graph_io.h"
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
    igraph_t g = read_graph_or_die(input, 0);
    ensure_directed(&g);
    ensure_weight_attr(&g);   /* "weight" doubles as the capacity */

    igraph_integer_t n = (igraph_integer_t)igraph_vcount(&g);
    igraph_integer_t m = (igraph_integer_t)igraph_ecount(&g);
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

    igraph_real_t (*run)(flow_t *, igraph_integer_t, igraph_integer_t, igraph_real_t *) = NULL;
    if (!strcmp(algo, "ff"))                   run = flow_ford_fulkerson;
    else if (!strcmp(algo, "ek"))              run = flow_edmonds_karp;
    else if (!strcmp(algo, "dinic"))           run = flow_dinic;
    else if (!strcmp(algo, "preflowpush"))     run = flow_preflow_push;
    else usage(argv[0]);

    igraph_real_t *flow = xmalloc((size_t)m * sizeof(igraph_real_t));
    igraph_real_t value = run(&f, source, target, flow);

    bool *side = flow_mincut_side(&f, source);
    /* cut capacity: saturated forward arcs leaving the s side */
    igraph_real_t cut_cap = 0.0;
    for (igraph_integer_t u = 0; u < f.n; u++) {
        if (!side[u]) continue;
        for (igraph_integer_t a = f.head[u]; a != -1; a = f.next[a])
            if (!side[f.to[a]] && f.cap[a] == 0.0)
                cut_cap += f.cap[a ^ 1];
    }

    printf("{\"algorithm\": \"%s\", \"source\": %" IGRAPH_PRId
           ", \"target\": %" IGRAPH_PRId ", \"value\": %g, \"cut_capacity\": %g}\n",
           algo, source, target, value, cut_cap);

    free(side);
    free(flow);
    flow_destroy(&f);
    return 0;
}
