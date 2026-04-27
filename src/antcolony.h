#pragma once
#include <igraph.h>

igraph_integer_t antcolony_run(const igraph_t *g, igraph_integer_t *color,
                                igraph_integer_t iterations,
                                igraph_integer_t num_ants,
                                double alpha,
                                double rho,
                                igraph_integer_t *conflicts_out);
