#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* --- Memory helpers: abort on OOM, keep teaching code free of error paths ---
 *
 * Every allocation of the course data structures goes through these four
 * functions, and every block carries a size tag, so `mem_live()` is the
 * exact byte footprint of our own data structures at any point in time.
 * igraph allocates on its own and is deliberately NOT counted: we want to
 * measure the structures the course talks about, not the library's.
 */
void *xmalloc(size_t n);
void *xcalloc(size_t n, size_t size);
void *xrealloc(void *p, size_t n);
void  xfree(void *p);

/* --- Memory accounting (the "space" axis of the benchmarks) --- */
size_t mem_live(void);    /* bytes currently held by x*alloc */
size_t mem_peak(void);    /* high-water mark of mem_live() */
size_t mem_total(void);   /* cumulative bytes ever allocated */
size_t mem_count(void);   /* cumulative number of allocations */
void   mem_reset(void);   /* zero the counters (tests only) */

/* --- Phases: separate "building a structure" from "using it" ---
 *
 * A run is split into named phases; each records its wall time plus the
 * live bytes it left behind. That is what lets a benchmark attribute a
 * time/space difference to a data structure instead of to the process.
 */
#define MEM_MAX_PHASES 8
#define MEM_PHASE_NAME 16

typedef struct {
    char   name[MEM_PHASE_NAME];
    double ms;
    size_t live_before;   /* live bytes when the phase started */
    size_t live_after;    /* live bytes when the phase ended */
    size_t live_peak;     /* high-water mark reached during the phase */
} mem_phase_t;

void               phase_begin(const char *name);
void               phase_end(void);
size_t             phase_count(void);
const mem_phase_t *phase_at(size_t i);
double             now_ms(void);   /* monotonic milliseconds */

/* --- SplitMix64 PRNG: tiny, seedable, deterministic --- */
typedef struct {
    uint64_t state;
} rng_t;

void rng_seed(rng_t *r, uint64_t seed);
uint64_t rng_next(rng_t *r);             /* uniform in [0, 2^64) */
double   rng_uniform(rng_t *r);          /* uniform in [0, 1)   */
uint64_t rng_choice(rng_t *r, uint64_t n); /* uniform in [0, n) */

#endif
