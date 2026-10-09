#pragma once
#include <igraph.h>
#include <stdio.h>

#include "util.h"

void output_json(FILE *out,
                 const char *algorithm,
                 igraph_integer_t num_colors,
                 const igraph_integer_t *color,
                 igraph_integer_t n,
                 double time_ms,
                 igraph_integer_t conflicts);

/* Appends the benchmark metrics as JSON fields. Callers that emit their own
 * object must have printed a comma just before (or not printed "{"). Each
 * phase reports its wall time and the bytes it allocated and kept, which is
 * what separates "how expensive is this structure to build" from "how much
 * space does it hold". */
void metrics_json(FILE *out);

/* Same, but starts the object (useful when a caller has nothing to print
 * before the metrics). */
void metrics_json_begin(FILE *out);
