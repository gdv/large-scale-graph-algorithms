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

    FILE *in = input_file ? fopen(input_file, "r") : stdin;
    if (!in) {
        fprintf(stderr, "Cannot open input file: %s\n", input_file);
        return 1;
    }

    igraph_t g;
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
        fprintf(stderr, "Invalid DIMACS file: missing p edge line\n");
        if (input_file) fclose(in);
        return 1;
    }
    igraph_empty(&g, n, IGRAPH_UNDIRECTED);
    while (fgets(line, sizeof(line), in)) {
        if (line[0] == 'c') continue;
        igraph_integer_t u, v;
        if (sscanf(line, "e %" IGRAPH_PRId " %" IGRAPH_PRId, &u, &v) == 2) {
            igraph_add_edge(&g, u - 1, v - 1);
        }
    }

    if (input_file) fclose(in);

    igraph_integer_t *color = calloc((size_t)n, sizeof(igraph_integer_t));
    if (!color) { fprintf(stderr, "calloc failed\n"); igraph_destroy(&g); return 1; }
    for (igraph_integer_t v = 0; v < n; v++) color[v] = -1;

    clock_t start = clock();
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
            igraph_integer_t *tmp = calloc((size_t)n, sizeof(igraph_integer_t));
            num_colors = dsatur_run(&g, tmp);
            free(tmp);
        }
        num_colors_out = sa1_run(&g, color, num_colors, iterations, t0, sa_alpha, &conflicts);
    } else if (strcmp(algo, "sa2") == 0) {
        if (iterations == 0) iterations = 10000;
        if (num_colors == 0) {
            igraph_integer_t *tmp = calloc((size_t)n, sizeof(igraph_integer_t));
            num_colors = dsatur_run(&g, tmp);
            free(tmp);
        }
        num_colors_out = sa2_run(&g, color, num_colors, iterations, t0, sa_alpha, &conflicts);
    } else if (strcmp(algo, "tabucol") == 0) {
        if (iterations == 0) iterations = 10000;
        if (num_colors == 0) {
            igraph_integer_t *tmp = calloc((size_t)n, sizeof(igraph_integer_t));
            num_colors = dsatur_run(&g, tmp);
            free(tmp);
        }
        num_colors_out = tabucol_run(&g, color, num_colors, iterations, tenure, &conflicts);
    } else if (strcmp(algo, "antcolony") == 0) {
        if (iterations == 0) iterations = 100;
        num_colors_out = antcolony_run(&g, color, iterations, num_ants, ac_alpha, rho, &conflicts);
    } else {
        fprintf(stderr, "Unknown algorithm: %s\n", algo);
        print_algorithms();
        igraph_destroy(&g);
        free(color);
        return 1;
    }

    double elapsed = (double)(clock() - start) / CLOCKS_PER_SEC * 1000.0;

    output_json(stdout, algo, num_colors_out, color, n, elapsed, conflicts);

    igraph_destroy(&g);
    free(color);
    return 0;
}
