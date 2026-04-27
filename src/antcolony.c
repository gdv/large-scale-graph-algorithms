#include "antcolony.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

igraph_integer_t antcolony_run(const igraph_t *g, igraph_integer_t *color,
                                igraph_integer_t iterations,
                                igraph_integer_t num_ants,
                                double alpha,
                                double rho,
                                igraph_integer_t *conflicts_out)
{
    igraph_integer_t n = igraph_vcount(g);
    igraph_integer_t k = n;

    double *trail = malloc((size_t)(n * n) * sizeof(double));
    if (!trail) { fprintf(stderr, "malloc failed\n"); exit(1); }
    for (igraph_integer_t i = 0; i < n * n; i++) trail[i] = 1.0;

    igraph_integer_t *best_sol = malloc((size_t)n * sizeof(igraph_integer_t));
    if (!best_sol) { fprintf(stderr, "malloc failed\n"); exit(1); }
    igraph_integer_t best_feasible = 0;
    igraph_integer_t best_colors = n;

    double *delta = malloc((size_t)(n * n) * sizeof(double));
    igraph_integer_t *ant_sol = malloc((size_t)n * sizeof(igraph_integer_t));
    if (!delta || !ant_sol) { fprintf(stderr, "malloc failed\n"); exit(1); }

    igraph_vector_int_t neigh;
    igraph_vector_int_init(&neigh, 0);

    for (igraph_integer_t iter = 0; iter < iterations; iter++) {
        for (igraph_integer_t p = 0; p < n * n; p++) delta[p] = 0.0;
        igraph_integer_t iter_best_colors = k + 1;
        igraph_integer_t *iter_best = NULL;

        for (igraph_integer_t ant = 0; ant < num_ants; ant++) {
            for (igraph_integer_t v = 0; v < n; v++) ant_sol[v] = -1;

            for (igraph_integer_t v = 0; v < n; v++) {
                double *probs = malloc((size_t)k * sizeof(double));
                if (!probs) { fprintf(stderr, "malloc failed\n"); exit(1); }
                double sum = 0.0;

                if (igraph_neighbors(g, &neigh, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
                    fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
                }
                igraph_integer_t deg = igraph_vector_int_size(&neigh);

                for (igraph_integer_t c = 0; c < k; c++) {
                    double f = 0.0;
                    igraph_integer_t class_size = 0;
                    for (igraph_integer_t w = 0; w < n; w++)
                        if (ant_sol[w] == c) class_size++;

                    if (class_size > 0) {
                        double trail_sum = 0.0;
                        for (igraph_integer_t i = 0; i < deg; i++) {
                            igraph_integer_t w = VECTOR(neigh)[i];
                            trail_sum += trail[v * n + w];
                        }
                        f = trail_sum / (double)class_size;
                    } else {
                        f = 1.0;
                    }

                    probs[c] = pow(f, alpha);
                    sum += probs[c];
                }

                if (sum > 0) {
                    double r = (double)rand() / RAND_MAX;
                    double cum = 0.0;
                    igraph_integer_t chosen = 0;
                    for (igraph_integer_t c = 0; c < k; c++) {
                        cum += probs[c] / sum;
                        if (r <= cum) { chosen = c; break; }
                    }
                    ant_sol[v] = chosen;
                } else {
                    ant_sol[v] = rand() % k;
                }
                free(probs);
            }

            igraph_integer_t conflicts = 0;
            for (igraph_integer_t v = 0; v < n; v++) {
                if (igraph_neighbors(g, &neigh, v, IGRAPH_ALL, IGRAPH_NO_LOOPS, false) != IGRAPH_SUCCESS) {
                    fprintf(stderr, "igraph_neighbors failed\n"); exit(1);
                }
                igraph_integer_t deg = igraph_vector_int_size(&neigh);
                for (igraph_integer_t i = 0; i < deg; i++) {
                    igraph_integer_t w = VECTOR(neigh)[i];
                    if (w > v && ant_sol[w] == ant_sol[v]) conflicts++;
                }
            }

            igraph_integer_t used_colors = 0;
            for (igraph_integer_t v = 0; v < n; v++)
                if (ant_sol[v] >= used_colors) used_colors = ant_sol[v] + 1;

            for (igraph_integer_t u = 0; u < n; u++) {
                for (igraph_integer_t v = u + 1; v < n; v++) {
                    if (ant_sol[u] == ant_sol[v]) {
                        double contrib = conflicts == 0 ? 1.0 / (double)used_colors : 0.01 / (double)(conflicts + 1);
                        delta[u * n + v] += contrib;
                        delta[v * n + u] += contrib;
                    }
                }
            }

            if (conflicts == 0 && used_colors < iter_best_colors) {
                if (!iter_best) {
                    iter_best = malloc((size_t)n * sizeof(igraph_integer_t));
                    if (!iter_best) { fprintf(stderr, "malloc failed\n"); exit(1); }
                }
                memcpy(iter_best, ant_sol, (size_t)n * sizeof(igraph_integer_t));
                iter_best_colors = used_colors;
            }
        }

        for (igraph_integer_t p = 0; p < n * n; p++) {
            trail[p] = rho * trail[p] + delta[p];
        }

        if (iter_best && iter_best_colors < best_colors) {
            best_colors = iter_best_colors;
            memcpy(best_sol, iter_best, (size_t)n * sizeof(igraph_integer_t));
            best_feasible = 1;
            k = best_colors - 1;
            if (k < 1) k = 1;
        }

        free(iter_best);
        iter_best = NULL;
    }

    igraph_vector_int_destroy(&neigh);

    if (best_feasible) {
        memcpy(color, best_sol, (size_t)n * sizeof(igraph_integer_t));
        *conflicts_out = 0;
    } else {
        for (igraph_integer_t v = 0; v < n; v++) color[v] = rand() % k;
        *conflicts_out = n;
    }

    free(trail);
    free(delta);
    free(ant_sol);
    free(best_sol);
    return best_feasible ? best_colors : k;
}
