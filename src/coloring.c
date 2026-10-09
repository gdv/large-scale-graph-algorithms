#include "util.h"
#include <igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "greedy.h"
#include "welsh_powell.h"
#include "dsatur.h"
#include "rlf.h"
#include "iterated_greedy.h"
#include "sa.h"
#include "tabucol.h"
#include "antcolony.h"
#include "output.h"

static igraph_t read_graph_or_die(const char *filename)
{
    igraph_t g;

    if (filename) {
        const char *dot = strrchr(filename, '.');
        int is_gz = (dot && strcmp(dot, ".gz") == 0);
        char *inner = NULL;
        FILE *in;
        int use_pclose = 0;

        if (is_gz) {
            char cmd[1024];
            snprintf(cmd, sizeof(cmd), "gzip -dc \"%s\"", filename);
            in = popen(cmd, "r");
            if (!in) { fprintf(stderr, "Cannot open: %s\n", filename); exit(1); }
            use_pclose = 1;
            inner = strdup(filename);
            size_t len = strlen(inner);
            if (len > 3) inner[len - 3] = '\0';
            dot = strrchr(inner, '.');
        } else {
            in = fopen(filename, "r");
            if (!in) { fprintf(stderr, "Cannot open: %s\n", filename); exit(1); }
        }

        if (dot && strcmp(dot, ".col") == 0) {
            igraph_integer_t n = 0;
            char line[256];
            while (fgets(line, sizeof(line), in)) {
                if (line[0] == 'c') continue;
                if (line[0] == 'p') {
                    sscanf(line, "p edge %" IGRAPH_PRId " %*d", &n);
                    break;
                }
            }
            if (n == 0) {
                fprintf(stderr, "Invalid DIMACS file: %s\n", filename);
                if (use_pclose) pclose(in); else fclose(in);
                xfree(inner);
                exit(1);
            }
            igraph_empty(&g, n, IGRAPH_UNDIRECTED);
            while (fgets(line, sizeof(line), in)) {
                if (line[0] == 'c') continue;
                igraph_integer_t u, v;
                if (sscanf(line, "e %" IGRAPH_PRId " %" IGRAPH_PRId, &u, &v) == 2) {
                    igraph_add_edge(&g, u - 1, v - 1);
                }
            }
        } else {
            if (igraph_read_graph_ncol(&g, in, NULL, 1, IGRAPH_ADD_WEIGHTS_NO, 0)) {
                fprintf(stderr, "Failed to read edge list: %s\n", filename);
                if (use_pclose) pclose(in); else fclose(in);
                xfree(inner);
                exit(1);
            }
        }

        if (use_pclose) pclose(in); else fclose(in);
        xfree(inner);
    } else {
        FILE *in = stdin;
        igraph_integer_t n = 0;
        char line[256];
        while (fgets(line, sizeof(line), in)) {
            if (line[0] == 'c') continue;
            if (line[0] == 'p') {
                sscanf(line, "p edge %" IGRAPH_PRId " %*d", &n);
                break;
            }
        }
        if (n == 0) {
            fprintf(stderr, "Invalid DIMACS file from stdin\n");
            exit(1);
        }
        igraph_empty(&g, n, IGRAPH_UNDIRECTED);
        while (fgets(line, sizeof(line), in)) {
            if (line[0] == 'c') continue;
            igraph_integer_t u, v;
            if (sscanf(line, "e %" IGRAPH_PRId " %" IGRAPH_PRId, &u, &v) == 2) {
                igraph_add_edge(&g, u - 1, v - 1);
            }
        }
    }

    return g;
}

static void print_algorithms(void)
{
    printf("Available algorithms:\n");
    printf("  greedy            - Greedy coloring\n");
    printf("  welsh-powell      - Welsh-Powell coloring\n");
    printf("  dsatur            - DSatur coloring\n");
    printf("  rlf               - Recursive Largest First coloring\n");
    printf("  iterated-greedy   - Iterated Greedy coloring\n");
    printf("  sa1               - Simulated Annealing v1 (change color)\n");
    printf("  sa2               - Simulated Annealing v2 (color/uncolor)\n");
    printf("  tabucol           - TabuCol coloring\n");
    printf("  antcolony         - Ant Colony coloring\n");
    printf("\nRun 'coloring help' for more details.\n");
}

static void print_help(void)
{
    printf("Usage: coloring <algorithm> [options]\n\n");
    printf("Reads a DIMACS graph from --input (or stdin) and outputs a JSON coloring.\n\n");
    printf("Algorithms:\n");
    printf("  greedy          [-i FILE]\n");
    printf("  welsh-powell    [-i FILE]\n");
    printf("  dsatur          [-i FILE]\n");
    printf("  rlf             [-i FILE]\n");
    printf("  iterated-greedy [-i FILE] [-o largest|reverse|random] [-I N]\n");
    printf("  sa1             [-i FILE] [-k K] [-I N] [-t T0] [-a ALPHA]\n");
    printf("  sa2             [-i FILE] [-k K] [-I N] [-t T0] [-a ALPHA]\n");
    printf("  tabucol         [-i FILE] [-k K] [-I N] [-l TENURE]\n");
    printf("  antcolony       [-i FILE] [-I N] [-n ANTS] [-a ALPHA] [-r RHO]\n");
    printf("\nOptions:\n");
    printf("  -i, --input FILE   DIMACS graph file (default: stdin)\n");
    printf("  -k, --colors K     Number of colors to use (heuristics, default: dsatur bound)\n");
    printf("  -I, --iterations N Number of iterations (default: algorithm-specific)\n");
    printf("  -o, --ordering O   Ordering: largest, reverse, random (iterated-greedy)\n");
    printf("  -t, --t0 T         Initial temperature (SA, default: 1.0)\n");
    printf("  -a, --alpha A      Cooling factor (SA) or exponent (antcolony, default: 0.95/1.0)\n");
    printf("  -n, --ants N       Number of ants (antcolony, default: 10)\n");
    printf("  -l, --tenure L     Tabu tenure (tabucol, default: 7)\n");
    printf("  -r, --rho R        Evaporation factor (antcolony, default: 0.9)\n");
}

static const char *get_opt(int argc, char **argv, int *i,
                            const char *short_opt, const char *long_opt)
{
    if (strcmp(argv[*i], short_opt) == 0 || strcmp(argv[*i], long_opt) == 0) {
        if (*i + 1 < argc) { (*i)++; return argv[*i]; }
        fprintf(stderr, "Error: %s requires an argument\n", long_opt);
        exit(1);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));

    if (argc < 2) {
        print_algorithms();
        return 0;
    }

    const char *algo = argv[1];

    if (strcmp(algo, "help") == 0) {
        print_help();
        return 0;
    }

    const char *input_file = NULL;
    igraph_integer_t iterations = 0;
    igraph_integer_t num_colors = 0;
    const char *ordering = "largest";
    double t0 = 1.0;
    double sa_alpha = 0.95;
    igraph_integer_t tenure = 7;
    igraph_integer_t num_ants = 10;
    double ac_alpha = 1.0;
    double rho = 0.9;

    for (int i = 2; i < argc; i++) {
        const char *val;
        if ((val = get_opt(argc, argv, &i, "-i", "--input")))
            input_file = val;
        else if ((val = get_opt(argc, argv, &i, "-I", "--iterations")))
            iterations = (igraph_integer_t)atoi(val);
        else if ((val = get_opt(argc, argv, &i, "-k", "--colors")))
            num_colors = (igraph_integer_t)atoi(val);
        else if ((val = get_opt(argc, argv, &i, "-o", "--ordering")))
            ordering = val;
        else if ((val = get_opt(argc, argv, &i, "-t", "--t0")))
            t0 = atof(val);
        else if ((val = get_opt(argc, argv, &i, "-a", "--alpha")))
            { sa_alpha = atof(val); ac_alpha = atof(val); }
        else if ((val = get_opt(argc, argv, &i, "-l", "--tenure")))
            tenure = (igraph_integer_t)atoi(val);
        else if ((val = get_opt(argc, argv, &i, "-n", "--ants")))
            num_ants = (igraph_integer_t)atoi(val);
        else if ((val = get_opt(argc, argv, &i, "-r", "--rho")))
            rho = atof(val);
        else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return 1;
        }
    }

    phase_begin("read");
    igraph_t g = read_graph_or_die(input_file);
    igraph_integer_t n = igraph_vcount(&g);
    igraph_integer_t *color = xcalloc((size_t)n, sizeof(igraph_integer_t));
    for (igraph_integer_t v = 0; v < n; v++) color[v] = -1;
    phase_end();
    phase_begin("solve");
    double t_solve = now_ms();
    igraph_integer_t num_colors_out = 0;
    igraph_integer_t conflicts = -1;

    if (strcmp(algo, "greedy") == 0) {
        num_colors_out = greedy_run(&g, color);
    } else if (strcmp(algo, "welsh-powell") == 0) {
        num_colors_out = welsh_powell_run(&g, color);
    } else if (strcmp(algo, "dsatur") == 0) {
        num_colors_out = dsatur_run(&g, color);
    } else if (strcmp(algo, "rlf") == 0) {
        num_colors_out = rlf_run(&g, color);
    } else if (strcmp(algo, "iterated-greedy") == 0) {
        if (iterations == 0) iterations = 100;
        num_colors_out = iterated_greedy_run(&g, color, ordering, iterations);
    } else if (strcmp(algo, "sa1") == 0) {
        if (iterations == 0) iterations = 10000;
        if (num_colors == 0) {
            igraph_integer_t *tmp = xcalloc((size_t)n, sizeof(igraph_integer_t));
            num_colors = dsatur_run(&g, tmp);
            xfree(tmp);
        }
        num_colors_out = sa1_run(&g, color, num_colors, iterations, t0, sa_alpha, &conflicts);
    } else if (strcmp(algo, "sa2") == 0) {
        if (iterations == 0) iterations = 10000;
        if (num_colors == 0) {
            igraph_integer_t *tmp = xcalloc((size_t)n, sizeof(igraph_integer_t));
            num_colors = dsatur_run(&g, tmp);
            xfree(tmp);
        }
        num_colors_out = sa2_run(&g, color, num_colors, iterations, t0, sa_alpha, &conflicts);
    } else if (strcmp(algo, "tabucol") == 0) {
        if (iterations == 0) iterations = 10000;
        if (num_colors == 0) {
            igraph_integer_t *tmp = xcalloc((size_t)n, sizeof(igraph_integer_t));
            num_colors = dsatur_run(&g, tmp);
            xfree(tmp);
        }
        num_colors_out = tabucol_run(&g, color, num_colors, iterations, tenure, &conflicts);
    } else if (strcmp(algo, "antcolony") == 0) {
        if (iterations == 0) iterations = 100;
        num_colors_out = antcolony_run(&g, color, iterations, num_ants, ac_alpha, rho, &conflicts);
    } else {
        phase_end();
        fprintf(stderr, "Unknown algorithm: %s\n", algo);
        print_algorithms();
        igraph_destroy(&g);
        xfree(color);
        return 1;
    }

    phase_end();

    output_json(stdout, algo, num_colors_out, color, n, now_ms() - t_solve, conflicts);

    igraph_destroy(&g);
    xfree(color);
    return 0;
}
