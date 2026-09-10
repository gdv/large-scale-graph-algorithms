#include "util.h"

#include <stdio.h>
#include <stdlib.h>

void *xmalloc(size_t n)
{
    void *p = malloc(n ? n : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(1); }
    return p;
}

void *xcalloc(size_t n, size_t size)
{
    void *p = calloc(n ? n : 1, size ? size : 1);
    if (!p) { fprintf(stderr, "out of memory\n"); exit(1); }
    return p;
}

void *xrealloc(void *p, size_t n)
{
    void *q = realloc(p, n ? n : 1);
    if (!q) { fprintf(stderr, "out of memory\n"); exit(1); }
    return q;
}

void rng_seed(rng_t *r, uint64_t seed)
{
    r->state = seed;
}

uint64_t rng_next(rng_t *r)
{
    uint64_t z = (r->state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

double rng_uniform(rng_t *r)
{
    return (double)(rng_next(r) >> 11) * (1.0 / 9007199254740992.0);
}

uint64_t rng_choice(rng_t *r, uint64_t n)
{
    return rng_next(r) % n;
}
