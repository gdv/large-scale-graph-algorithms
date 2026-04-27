#pragma once
#include <igraph.h>

igraph_integer_t iterated_greedy_run(const igraph_t *g, igraph_integer_t *color,
                                      const char *ordering, igraph_integer_t iterations);
