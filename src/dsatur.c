#include "dsatur.h"
#include <stdlib.h>
#include <stdbool.h>

igraph_integer_t dsatur_run(const igraph_t *g, igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);
    for (igraph_integer_t v = 0; v < n; v++) color[v] = -1;

    bool *colored_flag = calloc((size_t)n, sizeof(bool));
    if (!colored_flag) { fprintf(stderr, "calloc failed\n"); exit(1); }

    igraph_vector_int_t neighbors;
    igraph_vector_int_init(&neighbors, 0);

    igraph_integer_t max_color = 0;
    igraph_integer_t uncolored_count = n;

    while (uncolored_count > 0) {
        igraph_integer_t best_v = -1;
        igraph_integer_t best_sat = -1;
        igraph_integer_t best_deg = -1;

        for (igraph_integer_t v = 0; v < n; v++) {
            if (!colored_flag[v]) {
                igraph_integer_t limit = max_color + 2;
                bool *seen_colors = calloc((size_t)limit, sizeof(bool));
                if (!seen_colors) { fprintf(stderr, "calloc failed\n"); exit(1); }
                if (igraph_neighbors(g, &neighbors, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
                    fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
                }
                igraph_integer_t deg = igraph_vector_int_size(&neighbors);
                igraph_integer_t sat = 0;
                for (igraph_integer_t i = 0; i < deg; i++) {
                    igraph_integer_t w = VECTOR(neighbors)[i];
                    if (colored_flag[w] && !seen_colors[color[w]]) {
                        seen_colors[color[w]] = true;
                        sat++;
                    }
                }
                free(seen_colors);

                if (sat > best_sat || (sat == best_sat && deg > best_deg)) {
                    best_sat = sat;
                    best_deg = deg;
                    best_v = v;
                }
            }
        }

        if (igraph_neighbors(g, &neighbors, best_v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
            fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
        }
        igraph_integer_t deg = igraph_vector_int_size(&neighbors);
        igraph_integer_t limit = deg + 1;
        bool *used = calloc((size_t)limit, sizeof(bool));
        if (!used) { fprintf(stderr, "calloc failed\n"); exit(1); }
        for (igraph_integer_t i = 0; i < deg; i++) {
            igraph_integer_t w = VECTOR(neighbors)[i];
            if (color[w] >= 0 && color[w] < limit) used[color[w]] = true;
        }
        igraph_integer_t c = 0;
        while (used[c]) c++;
        color[best_v] = c;
        if (c > max_color) max_color = c;
        colored_flag[best_v] = true;
        uncolored_count--;
        free(used);
    }

    igraph_vector_int_destroy(&neighbors);
    free(colored_flag);
    return max_color + 1;
}
