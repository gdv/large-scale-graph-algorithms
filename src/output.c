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
    metrics_json(out);
    fprintf(out, "\n}\n");
}

void metrics_json(FILE *out)
{
    fprintf(out, ",\n  \"metrics\": {\n");
    fprintf(out, "    \"peak_bytes\": %zu,\n", mem_peak());
    fprintf(out, "    \"live_bytes\": %zu,\n", mem_live());
    fprintf(out, "    \"total_alloc_bytes\": %zu,\n", mem_total());
    fprintf(out, "    \"alloc_count\": %zu,\n", mem_count());
    fprintf(out, "    \"phases\": [");
    for (size_t i = 0; i < phase_count(); i++) {
        const mem_phase_t *p = phase_at(i);
        fprintf(out, "%s\n      {\"name\": \"%s\", \"ms\": %.3f, "
                     "\"bytes_before\": %zu, \"bytes_after\": %zu, \"bytes_peak\": %zu}",
                i ? "," : "", p->name, p->ms, p->live_before, p->live_after, p->live_peak);
    }
    fprintf(out, "\n    ]\n  }");
}

void metrics_json_begin(FILE *out)
{
    fprintf(out, "{");
    metrics_json(out);
    fprintf(out, "\n}\n");
}
