#include "rlf.h"
#include <stdlib.h>
#include <stdbool.h>

igraph_integer_t rlf_run(const igraph_t *g, igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);
    for (igraph_integer_t v = 0; v < n; v++) color[v] = -1;

    bool *colored = calloc((size_t)n, sizeof(bool));
    bool *adj_to_I = calloc((size_t)n, sizeof(bool));
    if (!colored || !adj_to_I) { fprintf(stderr, "calloc failed\n"); exit(1); }
    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);
    igraph_integer_t current_color = 0;

    while (1) {
        igraph_integer_t best_v = -1;
        igraph_integer_t best_deg = -1;
        for (igraph_integer_t v = 0; v < n; v++) {
            if (!colored[v]) {
                igraph_integer_t d;
                if (igraph_degree_1(g, &d, v, IGRAPH_ALL, IGRAPH_NO_LOOPS) != IGRAPH_SUCCESS) {
                    fprintf(stderr, "igraph_degree_1 failed\n"); exit(1);
                }
                if (d > best_deg) { best_deg = d; best_v = v; }
            }
        }
        if (best_v < 0) break;

        for (igraph_integer_t i = 0; i < n; i++) adj_to_I[i] = false;
        colored[best_v] = true;
        color[best_v] = current_color;

        if (igraph_neighbors(g, &neighbors, best_v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
            fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
        }
        igraph_integer_t deg = igraph_vector_int_size(&neighbors);
        for (igraph_integer_t i = 0; i < deg; i++) adj_to_I[VECTOR(neighbors)[i]] = true;

        while (1) {
            igraph_integer_t best_cand = -1;
            igraph_integer_t best_adj = -1;
            igraph_integer_t best_mdeg = n + 1;
            for (igraph_integer_t v = 0; v < n; v++) {
                if (!colored[v] && !adj_to_I[v]) {
                    igraph_integer_t adj_to_I_count = 0;
                    if (igraph_neighbors(g, &neighbors, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
                        fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
                    }
                    deg = igraph_vector_int_size(&neighbors);
                    for (igraph_integer_t i = 0; i < deg; i++) {
                        if (adj_to_I[VECTOR(neighbors)[i]]) adj_to_I_count++;
                    }
                    igraph_integer_t mdeg = deg;
                    if (adj_to_I_count > best_adj || (adj_to_I_count == best_adj && mdeg < best_mdeg)) {
                        best_adj = adj_to_I_count;
                        best_mdeg = mdeg;
                        best_cand = v;
                    }
                }
            }
            if (best_cand < 0) break;
            colored[best_cand] = true;
            color[best_cand] = current_color;
            if (igraph_neighbors(g, &neighbors, best_cand, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
                fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
            }
            deg = igraph_vector_int_size(&neighbors);
            for (igraph_integer_t i = 0; i < deg; i++) adj_to_I[VECTOR(neighbors)[i]] = true;
        }
        current_color++;
    }

    igraph_vector_int_destroy(&neighbors);
    free(colored);
    free(adj_to_I);
    return current_color;
}
