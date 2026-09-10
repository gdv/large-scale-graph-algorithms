#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* --- Memory helpers: abort on OOM, keep teaching code free of error paths --- */
void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t size);
void *xrealloc(void *p, size_t n);

/* --- SplitMix64 PRNG: tiny, seedable, deterministic --- */
typedef struct {
    uint64_t state;
} rng_t;

void rng_seed(rng_t *r, uint64_t seed);
uint64_t rng_next(rng_t *r);             /* uniform in [0, 2^64) */
double   rng_uniform(rng_t *r);          /* uniform in [0, 1)   */
uint64_t rng_choice(rng_t *r, uint64_t n); /* uniform in [0, n) */

#endif
