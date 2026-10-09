#include "util.h"
#include "tabucol.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

igraph_integer_t tabucol_run(const igraph_t *g, igraph_integer_t *color,
                              igraph_integer_t num_colors,
                              igraph_integer_t iterations,
                              igraph_integer_t tenure,
                              igraph_integer_t *conflicts_out)
{
    igraph_integer_t n = igraph_vcount(g);

    for (igraph_integer_t v = 0; v < n; v++)
        color[v] = rand() % num_colors;

    igraph_integer_t *best = xmalloc((size_t)n * sizeof(igraph_integer_t));
    memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));

    igraph_integer_t *tabu = xcalloc((size_t)(n * num_colors), sizeof(igraph_integer_t));

    igraph_vector_int_t neigh;
    igraph_vector_int_init(&neigh, 0);

    igraph_integer_t best_conflicts = n * n;
    igraph_integer_t current_conflicts = n * n;

    for (igraph_integer_t iter = 0; iter < iterations; iter++) {
        igraph_integer_t best_delta = n * n;
        igraph_integer_t best_v = -1;
        igraph_integer_t best_c = -1;

        for (igraph_integer_t v = 0; v < n; v++) {
            if (igraph_neighbors(g, &neigh, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
                fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
            }
            igraph_integer_t deg = igraph_vector_int_size(&neigh);

            igraph_integer_t same_color = 0;
            for (igraph_integer_t i = 0; i < deg; i++) {
                igraph_integer_t w = VECTOR(neigh)[i];
                if (color[w] == color[v]) same_color++;
            }

            if (same_color == 0) continue;

            igraph_integer_t old_c = color[v];
            for (igraph_integer_t c = 0; c < num_colors; c++) {
                if (c == old_c) continue;

                igraph_integer_t new_same = 0;
                for (igraph_integer_t i = 0; i < deg; i++) {
                    igraph_integer_t w = VECTOR(neigh)[i];
                    if (color[w] == c) new_same++;
                }

                igraph_integer_t delta = new_same - same_color;
                igraph_integer_t candidate_conflicts = current_conflicts + delta;
                if (candidate_conflicts < best_conflicts || tabu[v * num_colors + c] <= iter) {
                    if (delta < best_delta) {
                        best_delta = delta;
                        best_v = v;
                        best_c = c;
                    }
                }
            }
        }

        if (best_v < 0) break;

        igraph_integer_t old_c = color[best_v];
        color[best_v] = best_c;
        tabu[best_v * num_colors + old_c] = iter + tenure;

        igraph_integer_t total_conflicts = 0;
        for (igraph_integer_t v = 0; v < n; v++) {
            if (igraph_neighbors(g, &neigh, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
                fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
            }
            igraph_integer_t deg = igraph_vector_int_size(&neigh);
            for (igraph_integer_t i = 0; i < deg; i++) {
                igraph_integer_t w = VECTOR(neigh)[i];
                if (w > v && color[w] == color[v]) total_conflicts++;
            }
        }

        if (total_conflicts < best_conflicts) {
            best_conflicts = total_conflicts;
            memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));
        }
        current_conflicts = total_conflicts;

        if (best_conflicts == 0) break;
    }

    igraph_vector_int_destroy(&neigh);
    memcpy(color, best, (size_t)n * sizeof(igraph_integer_t));
    *conflicts_out = best_conflicts;
    xfree(best);
    xfree(tabu);
    return num_colors;
}
