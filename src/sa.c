#include "sa.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

static igraph_integer_t count_conflicts(const igraph_t *g,
                                         const igraph_integer_t *color)
{
    igraph_integer_t n = igraph_vcount(g);
    igraph_vector_int_t neigh;
    igraph_vector_int_init(&neigh, 0);
    igraph_integer_t conflicts = 0;
    for (igraph_integer_t v = 0; v < n; v++) {
        if (color[v] < 0) continue;
        if (igraph_neighbors(g, &neigh, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
            fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
        }
        igraph_integer_t deg = igraph_vector_int_size(&neigh);
        for (igraph_integer_t i = 0; i < deg; i++) {
            igraph_integer_t w = VECTOR(neigh)[i];
            if (w > v && color[w] >= 0 && color[v] == color[w])
                conflicts++;
        }
    }
    igraph_vector_int_destroy(&neigh);
    return conflicts;
}

igraph_integer_t sa1_run(const igraph_t *g, igraph_integer_t *color,
                          igraph_integer_t num_colors,
                          igraph_integer_t iterations,
                          double t0, double alpha,
                          igraph_integer_t *conflicts_out)
{
    igraph_integer_t n = igraph_vcount(g);

    for (igraph_integer_t v = 0; v < n; v++)
        color[v] = rand() % num_colors;

    igraph_integer_t *best = malloc((size_t)n * sizeof(igraph_integer_t));
    if (!best) { fprintf(stderr, "malloc failed\n"); exit(1); }
    memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));

    igraph_integer_t current_conflicts = count_conflicts(g, color);
    igraph_integer_t best_conflicts = current_conflicts;
    double T = t0;

    for (igraph_integer_t iter = 0; iter < iterations && current_conflicts > 0; iter++) {
        igraph_integer_t v = rand() % n;
        igraph_integer_t old_c = color[v];
        igraph_integer_t new_c = rand() % num_colors;
        if (new_c == old_c) continue;

        color[v] = new_c;
        igraph_integer_t new_conflicts = count_conflicts(g, color);

        igraph_integer_t delta = new_conflicts - current_conflicts;
        if (delta <= 0 || ((double)rand() / RAND_MAX) < exp(-delta / T)) {
            current_conflicts = new_conflicts;
            if (current_conflicts < best_conflicts) {
                best_conflicts = current_conflicts;
                memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));
            }
        } else {
            color[v] = old_c;
        }
        T *= alpha;
    }

    memcpy(color, best, (size_t)n * sizeof(igraph_integer_t));
    *conflicts_out = best_conflicts;
    free(best);
    return num_colors;
}

static igraph_integer_t count_uncolored(const igraph_integer_t *color, igraph_integer_t n)
{
    igraph_integer_t count = 0;
    for (igraph_integer_t v = 0; v < n; v++)
        if (color[v] < 0) count++;
    return count;
}

igraph_integer_t sa2_run(const igraph_t *g, igraph_integer_t *color,
                          igraph_integer_t num_colors,
                          igraph_integer_t iterations,
                          double t0, double alpha,
                          igraph_integer_t *conflicts_out)
{
    igraph_integer_t n = igraph_vcount(g);

    for (igraph_integer_t v = 0; v < n; v++)
        color[v] = rand() % num_colors;

    igraph_integer_t *best = malloc((size_t)n * sizeof(igraph_integer_t));
    if (!best) { fprintf(stderr, "malloc failed\n"); exit(1); }
    memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));

    igraph_integer_t current_uncolored = 0;
    igraph_integer_t best_uncolored = current_uncolored;
    double T = t0;

    igraph_vector_int_t neigh;
    igraph_vector_int_init(&neigh, 0);

    for (igraph_integer_t iter = 0; iter < iterations && best_uncolored > 0; iter++) {
        igraph_integer_t v = rand() % n;
        if (color[v] < 0) {
            color[v] = rand() % num_colors;
        } else {
            igraph_integer_t old_c = color[v];
            igraph_integer_t new_c;
            do { new_c = rand() % num_colors; } while (new_c == old_c);
            color[v] = new_c;

            if (igraph_neighbors(g, &neigh, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
                fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
            }
            igraph_integer_t deg = igraph_vector_int_size(&neigh);
            for (igraph_integer_t i = 0; i < deg; i++) {
                igraph_integer_t w = VECTOR(neigh)[i];
                if (color[w] == new_c) color[w] = -1;
            }
        }

        igraph_integer_t new_uncolored = count_uncolored(color, n);
        igraph_integer_t delta = new_uncolored - current_uncolored;

        if (delta <= 0 || ((double)rand() / RAND_MAX) < exp(-delta / T)) {
            current_uncolored = new_uncolored;
            if (current_uncolored < best_uncolored) {
                best_uncolored = current_uncolored;
                memcpy(best, color, (size_t)n * sizeof(igraph_integer_t));
            }
        }
        T *= alpha;
    }

    igraph_vector_int_destroy(&neigh);
    memcpy(color, best, (size_t)n * sizeof(igraph_integer_t));
    *conflicts_out = best_uncolored;
    free(best);
    return num_colors;
}
