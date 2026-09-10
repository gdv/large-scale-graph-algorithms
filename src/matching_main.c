#include <igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "csr.h"
#include "graph_io.h"
#include "hungarian.h"
#include "matching.h"
#include "util.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s -i FILE -a ALGO\n"
        "  ALGO: hopcroftkarp | viaflow | hungarian\n", prog);
    exit(1);
}

int main(int argc, char **argv)
{
    const char *input = NULL, *algo = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc) input = argv[++i];
        else if (!strcmp(argv[i], "-a") && i + 1 < argc) algo = argv[++i];
        else usage(argv[0]);
    }
    if (!input || !algo) usage(argv[0]);
    if (strcmp(algo, "hopcroftkarp") && strcmp(algo, "viaflow") &&
        strcmp(algo, "hungarian"))
        usage(argv[0]);

    igraph_set_attribute_table(&igraph_cattribute_table);
    igraph_t g = read_graph_or_die(input, 0);
    igraph_integer_t n = (igraph_integer_t)igraph_vcount(&g);

    if (!strcmp(algo, "hungarian")) {
        /* cost matrix from the "weight" attribute; edges without the
         * attribute cost 1.  The assignment problem needs a square
         * matrix, so we pad missing pairs with 0 (free). */
        igraph_real_t *cost = xmalloc((size_t)n * n * sizeof(igraph_real_t));
        for (igraph_integer_t i = 0; i < n * n; i++) cost[i] = IGRAPH_INFINITY;

        igraph_vector_int_t elist;
        igraph_vector_int_init(&elist, 0);
        igraph_get_edgelist(&g, &elist, false);
        igraph_bool_t has_weight = igraph_cattribute_has_attr(&g, IGRAPH_ATTRIBUTE_EDGE, "weight");
        for (igraph_integer_t e = 0; e < (igraph_integer_t)igraph_ecount(&g); e++) {
            igraph_integer_t u = VECTOR(elist)[2 * e];
            igraph_integer_t v = VECTOR(elist)[2 * e + 1];
            igraph_real_t w = has_weight ? igraph_cattribute_EAN(&g, "weight", e) : 1.0;
            if (w < cost[u * n + v]) cost[u * n + v] = cost[v * n + u] = w;
        }
        igraph_vector_int_destroy(&elist);
        igraph_destroy(&g);

        /* remaining non-edges become free dummy assignments */
        for (igraph_integer_t i = 0; i < n * n; i++)
            if (cost[i] == IGRAPH_INFINITY) cost[i] = 0.0;

        igraph_integer_t *assignment = xmalloc((size_t)n * sizeof(igraph_integer_t));
        igraph_real_t total = hungarian_solve(cost, n, assignment);
        printf("{\"algorithm\": \"hungarian\", \"n\": %" IGRAPH_PRId ", \"total\": %g,\n",
               n, total);
        printf(" \"assignment\": [");
        for (igraph_integer_t i = 0; i < n; i++)
            printf("%s%" IGRAPH_PRId, i ? ", " : "", assignment[i]);
        printf("]}\n");
        free(assignment);
        free(cost);
        return 0;
    }

    csr_t c;
    csr_build(&c, &g, false);
    igraph_vector_bool_t types;
    igraph_vector_bool_init(&types, n);
    igraph_bool_t bipartite = 0;
    igraph_is_bipartite(&g, &bipartite, &types);
    if (!bipartite)
        fprintf(stderr, "warning: graph is not bipartite; results are meaningless\n");
    bool *side = xmalloc((size_t)n * sizeof(bool));
    for (igraph_integer_t i = 0; i < n; i++) side[i] = VECTOR(types)[i];
    igraph_destroy(&g);

    igraph_integer_t *ml = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t *mr = xmalloc((size_t)n * sizeof(igraph_integer_t));
    igraph_integer_t size = 0;
    if (!strcmp(algo, "hopcroftkarp"))
        size = matching_hopcroft_karp(&c, side, ml, mr);
    else
        size = matching_via_flow(&c, side, ml, mr);

    printf("{\"algorithm\": \"%s\", \"n\": %" IGRAPH_PRId ", \"size\": %" IGRAPH_PRId ",\n",
           algo, n, size);
    printf(" \"matching\": [");
    for (igraph_integer_t i = 0; i < n; i++)
        printf("%s%" IGRAPH_PRId, i ? ", " : "", ml[i]);
    printf("]}\n");

    igraph_vector_bool_destroy(&types);
    free(side); free(ml); free(mr);
    csr_destroy(&c);
    return 0;
}
