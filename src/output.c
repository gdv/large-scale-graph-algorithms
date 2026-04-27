#include "output.h"

void output_json(FILE *out,
                 const char *algorithm,
                 igraph_integer_t num_colors,
                 const igraph_integer_t *color,
                 igraph_integer_t n,
                 double time_ms,
                 igraph_integer_t conflicts)
{
    fprintf(out, "{\n");
    fprintf(out, "  \"algorithm\": \"%s\",\n", algorithm);
    fprintf(out, "  \"colors\": %" IGRAPH_PRId ",\n", num_colors);
    fprintf(out, "  \"assignments\": {");
    for (igraph_integer_t v = 0; v < n; v++) {
        if (v > 0) fprintf(out, ", ");
        fprintf(out, "\"%" IGRAPH_PRId "\": %" IGRAPH_PRId, v, color[v]);
    }
    fprintf(out, "},\n");
    fprintf(out, "  \"time_ms\": %.3f", time_ms);
    if (conflicts >= 0) {
        fprintf(out, ",\n  \"conflicts\": %" IGRAPH_PRId, conflicts);
    }
    fprintf(out, "\n}\n");
}
