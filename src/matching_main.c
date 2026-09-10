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

static int cmp_real(const void *a, const void *b)
{
    igraph_real_t x = *(const igraph_real_t *)a;
    igraph_real_t y = *(const igraph_real_t *)b;
    return (x > y) - (x < y);
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
        /* cost matrix from the "weight" attribute (default 1); the
         * assignment problem needs a square matrix, so we pad with zeros
         * up to the larger side of the bipartition (here: up to n). */
        igraph_real_t *cost = xcalloc((size_t)n * n, sizeof(igraph_real_t));
        igraph_vector_int_t elist;
        igraph_vector_int_init(&elist, 0);
        igraph_get_edgelist(&g, &elist, false);
        for (igraph_integer_t e = 0; e < (igraph_integer_t)igraph_ecount(&g); e++) {
            igraph_integer_t u = VECTOR(elist)[2 * e];
            igraph_integer_t v = VECTOR(elist)[2 * e + 1];
            igraph_real_t w = igraph_cattribute_EAN(&g, "weight", e);
            if (u < n && v < n && (w < cost[u * n + v] || cost[u * n + v] == 0.0))
                cost[u * n + v] = cost[v * n + u] = w;
        }
        igraph_vector_int_destroy(&elist);
        igraph_destroy(&g);

        igraph_integer_t *assignment = xmalloc((size_t)n * sizeof(igraph_integer_t));
        igraph_real_t total = hungarian_solve(cost, n, assignment);
        printf("{\"algorithm\": \"hungarian\", \"n\": %" IGRAPH_PRId
               ", \"total\": %g}\n", n, total);
        printf("\"assignment\": [");
        for (igraph_integer_t i = 0; i < n; i++)
            printf("%s%" IGRAPH_PRId, i ? ", " : "", assignment[i]);
        printf("]\n");
        free(assignment);
        free(cost);
        return 0;
    }

    csr_t c;
    csr_build(&c, &g, false);
    igraph_vector_bool_t types;
    igraph_vector_bool_init(&types, n);
    igraph_is_bipartite(&g, NULL, &types);
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

    printf("{\"algorithm\": \"%s\", \"n\": %" IGRAPH_PRId ", \"size\": %" IGRAPH_PRId "}\n",
           algo, n, size);
    printf("\"matching\": [");
    for (igraph_integer_t i = 0; i < n; i++)
        printf("%s%" IGRAPH_PRId, i ? ", " : "", ml[i]);
    printf("]\n");

    igraph_vector_bool_destroy(&types);
    free(side); free(ml); free(mr);
    csr_destroy(&c);
    return 0;
}
