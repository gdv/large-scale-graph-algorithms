#include <igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "connectivity.h"
#include "csr.h"
#include "graph_io.h"
#include "output.h"
#include "search.h"
#include "util.h"

static void usage(const char *prog)
{
    fprintf(stderr,
        "usage: %s -i FILE -a ALGO [-s SOURCE] [-t TARGET] [--directed]\n"
        "  ALGO: bfs | dfs | astar | scc | ap | biconnected | bridges | edge2\n"
        "  --directed  keep arc directions (needed by scc; the other\n"
        "              algorithms are undirected and ignore the flag)\n",
        prog);
    exit(1);
}

/* A* default heuristic: 0 turns A* into plain Dijkstra on unit weights. */
static igraph_real_t null_heuristic(igraph_integer_t v, igraph_integer_t target, void *ctx)
{
    (void)v; (void)target; (void)ctx;
    return 0.0;
}

int main(int argc, char **argv)
{
    const char *input = NULL, *algo = NULL;
    igraph_integer_t source = 0, target = 0;
    bool directed = false;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-i") && i + 1 < argc) input = argv[++i];
        else if (!strcmp(argv[i], "-a") && i + 1 < argc) algo = argv[++i];
        else if (!strcmp(argv[i], "-s") && i + 1 < argc) source = atoll(argv[++i]);
        else if (!strcmp(argv[i], "-t") && i + 1 < argc) target = atoll(argv[++i]);
        else if (!strcmp(argv[i], "--directed")) directed = true;
        else usage(argv[0]);
    }
    if (!input || !algo) usage(argv[0]);
    if (strcmp(algo, "bfs") && strcmp(algo, "dfs") && strcmp(algo, "astar") &&
        strcmp(algo, "scc") && strcmp(algo, "ap") && strcmp(algo, "biconnected") &&
        strcmp(algo, "bridges") && strcmp(algo, "edge2"))
        usage(argv[0]);

    igraph_set_attribute_table(&igraph_cattribute_table);
    phase_begin("read");
    igraph_t g = read_graph_or_die(input, 0, directed);
    phase_end();
    phase_begin("build");
    csr_t c;
    /* Everything here but scc is an undirected algorithm; on an undirected
     * graph csr_build(_, true) would drop one of the two arcs. */
    csr_build(&c, &g, directed);
    igraph_destroy(&g);
    phase_end();

    if (source < 0 || source >= c.n) {
        fprintf(stderr, "source %" IGRAPH_PRId " out of range [0,%" IGRAPH_PRId ")\n",
                source, c.n);
        csr_destroy(&c);
        return 1;
    }
    if (!strcmp(algo, "astar") && (target < 0 || target >= c.n)) {
        fprintf(stderr, "target %" IGRAPH_PRId " out of range [0,%" IGRAPH_PRId ")\n",
                target, c.n);
        csr_destroy(&c);
        return 1;
    }

    phase_begin("solve");
    printf("{\"algorithm\": \"%s\", \"n\": %" IGRAPH_PRId ",\n", algo, c.n);

    if (!strcmp(algo, "bfs")) {
        bfs_result_t r;
        bfs_run(&c, source, &r);
        printf(" \"reached\": %" IGRAPH_PRId ",\n", r.n_reached);
        printf(" \"dist\": [");
        for (igraph_integer_t v = 0; v < c.n; v++)
            printf("%s%" IGRAPH_PRId, v ? ", " : "", r.dist[v]);
        printf("]");
        bfs_result_destroy(&r);
    } else if (!strcmp(algo, "dfs")) {
        dfs_result_t r;
        dfs_run(&c, source, &r);
        printf(" \"dis\": [");
        for (igraph_integer_t v = 0; v < c.n; v++)
            printf("%s%" IGRAPH_PRId, v ? ", " : "", r.dis[v]);
        printf("],\n \"comp\": [");
        for (igraph_integer_t v = 0; v < c.n; v++)
            printf("%s%" IGRAPH_PRId, v ? ", " : "", r.comp[v]);
        printf("]");
        dfs_result_destroy(&r);
    } else if (!strcmp(algo, "astar")) {
        igraph_real_t *dist = xmalloc((size_t)c.n * sizeof(igraph_real_t));
        igraph_integer_t *parent = xmalloc((size_t)c.n * sizeof(igraph_integer_t));
        bool ok = astar_run(&c, source, target, null_heuristic, NULL, dist, parent);
        printf(" \"reached_target\": %s, \"dist_target\": %g",
               ok ? "true" : "false", ok ? dist[target] : -1.0);
        xfree(dist);
        xfree(parent);
    } else if (!strcmp(algo, "scc")) {
        scc_result_t r;
        scc_run(&c, &r);
        printf(" \"n_comp\": %" IGRAPH_PRId, r.n_comp);
        scc_result_destroy(&r);
    } else if (!strcmp(algo, "ap")) {
        ap_result_t r;
        articulation_points_run(&c, &r);
        printf(" \"articulation_points\": [");
        igraph_integer_t first = 1;
        for (igraph_integer_t v = 0; v < c.n; v++)
            if (r.is_ap[v]) { printf("%s%" IGRAPH_PRId, first ? "" : ", ", v); first = 0; }
        printf("]");
        ap_result_destroy(&r);
    } else if (!strcmp(algo, "biconnected")) {
        biconnected_result_t r;
        biconnected_run(&c, &r);
        printf(" \"n_blocks\": %" IGRAPH_PRId, r.n_blocks);
        biconnected_result_destroy(&r);
    } else if (!strcmp(algo, "bridges")) {
        bridges_result_t r;
        bridges_run(&c, &r);
        printf(" \"bridge_edges\": [");
        igraph_integer_t first = 1;
        for (igraph_integer_t a = 0; a < c.m; a++)
            if (r.is_bridge[a] && a < c.rev[a]) {
                printf("%s[%" IGRAPH_PRId ",%" IGRAPH_PRId "]",
                       first ? "" : ", ", c.targets[c.rev[a]], c.targets[a]);
                first = 0;
            }
        printf("]");
        bridges_result_destroy(&r);
    } else if (!strcmp(algo, "edge2")) {
        edge2_result_t r;
        edge2_components_run(&c, &r);
        printf(" \"n_comp\": %" IGRAPH_PRId, r.n_comp);
        edge2_result_destroy(&r);
    } else {
        usage(argv[0]);
    }

    phase_end();
    metrics_json(stdout);
    printf("\n}\n");

    csr_destroy(&c);
    return 0;
}
