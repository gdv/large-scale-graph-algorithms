#include <igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "edgegraph.h"
#include "graph_io.h"
#include "karger.h"
#include "output.h"
#include "maxcut.h"
#include "util.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s -i FILE -a ALGO [-I TRIALS] [-s SEED]\n"
        "  ALGO: maxcut | karger | kargerstein\n", prog);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *input = NULL, *algo = NULL;
    igraph_integer_t trials = 100;
    uint64_t seed = 42;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc) input = argv[++i];
        else if (!strcmp(argv[i], "-a") && i + 1 < argc) algo = argv[++i];
        else if (!strcmp(argv[i], "-I") && i + 1 < argc) trials = atoll(argv[++i]);
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) seed = strtoull(argv[++i], NULL, 10);
        else usage(argv[0]);
    }
    if (!input || !algo) usage(argv[0]);
    if (strcmp(algo, "maxcut") && strcmp(algo, "karger") && strcmp(algo, "kargerstein"))
        usage(argv[0]);
    if (trials < 1) {
        fprintf(stderr, "trials must be >= 1\n");
        return 1;
    }

    igraph_set_attribute_table(&igraph_cattribute_table);
    phase_begin("read");
    igraph_t g = read_graph_or_die(input, 0, true);
    edge_graph_t e;
    edge_graph_build(&e, &g);      /* multigraph: parallel edges are the point */
    igraph_destroy(&g);
    phase_end();
    phase_begin("solve");

    rng_t rng;
    rng_seed(&rng, seed);
    bool *side = xmalloc((size_t)e.n * sizeof(bool));

    if (!strcmp(algo, "maxcut")) {
        igraph_integer_t v = maxcut_best(&e, &rng, trials, side);
        printf("{\"algorithm\": \"maxcut\", \"value\": %" IGRAPH_PRId
               ", \"m\": %" IGRAPH_PRId, v, e.m);
    } else if (!strcmp(algo, "karger")) {
        igraph_integer_t c = karger_mincut(&e, &rng, trials, side);
        printf("{\"algorithm\": \"karger\", \"cut\": %" IGRAPH_PRId
               ", \"trials\": %" IGRAPH_PRId, c, trials);
    } else {
        igraph_integer_t c = karger_stein(&e, &rng);
        printf("{\"algorithm\": \"kargerstein\", \"cut\": %" IGRAPH_PRId, c);
    }
    phase_end();
    metrics_json(stdout);
    printf("\n}\n");

    xfree(side);
    edge_graph_destroy(&e);
    return 0;
}
