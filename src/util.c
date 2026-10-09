#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---------------------------------------------------------------------------
 * Counting allocator
 *
 * Every block handed out by xmalloc/xcalloc/xrealloc is prefixed with a tag
 * holding its size, so mem_live() is the exact number of bytes our data
 * structures occupy -- no RSS sampling, no guessing.  The tag is a union
 * with max_align_t so the payload keeps the alignment of the strictest
 * fundamental type.
 *
 * igraph's own allocations are intentionally invisible here: the point of
 * the benchmark is to attribute space to the structures the course teaches
 * (CSR vs linked lists vs hash tables), not to the I/O library.
 * ------------------------------------------------------------------------- */

typedef union {
    struct { size_t size; } s;
    max_align_t align;
} mem_hdr_t;

#define PHASE_CLOSED ((size_t)-1)

static size_t mem_live_bytes, mem_peak_bytes, mem_total_bytes, mem_allocs;

static mem_phase_t phases[MEM_MAX_PHASES];
static size_t      n_phases   = 0;
static size_t      phase_open = PHASE_CLOSED;
static double      phase_t0   = 0.0;

/* The live-byte high-water mark is always reached right after an allocation,
 * so refreshing it on every change suffices -- no sampling thread needed. */
static void mem_note_peak(void)
{
    if (mem_live_bytes > mem_peak_bytes)
        mem_peak_bytes = mem_live_bytes;
    if (phase_open != PHASE_CLOSED && mem_live_bytes > phases[phase_open].live_peak)
        phases[phase_open].live_peak = mem_live_bytes;
}

static void *mem_alloc(size_t n)
{
    mem_hdr_t *h = malloc(sizeof(mem_hdr_t) + n);
    if (!h) { fprintf(stderr, "out of memory\n"); exit(1); }
    h->s.size = n;
    mem_live_bytes += n;
    mem_total_bytes += n;
    mem_allocs++;
    mem_note_peak();
    return h + 1;
}

void *xmalloc(size_t n)
{
    return mem_alloc(n ? n : 1);
}

void *xcalloc(size_t n, size_t size)
{
    size_t bytes = n * size;
    void *p = mem_alloc(bytes ? bytes : 1);
    memset(p, 0, bytes ? bytes : 1);
    return p;
}

void *xrealloc(void *p, size_t n)
{
    if (!p) return xmalloc(n);
    n = n ? n : 1;
    mem_hdr_t *h = (mem_hdr_t *)p - 1;
    size_t old = h->s.size;
    mem_hdr_t *q = realloc(h, sizeof(mem_hdr_t) + n);
    if (!q) { fprintf(stderr, "out of memory\n"); exit(1); }
    q->s.size = n;
    mem_live_bytes += n - old;   /* signed: shrink must not underflow */
    mem_total_bytes += n;
    mem_allocs++;
    mem_note_peak();
    return q + 1;
}

void xfree(void *p)
{
    if (!p) return;
    mem_hdr_t *h = (mem_hdr_t *)p - 1;
    mem_live_bytes -= h->s.size;
    free(h);
}

size_t mem_live(void)  { return mem_live_bytes; }
size_t mem_peak(void)  { return mem_peak_bytes; }
size_t mem_total(void) { return mem_total_bytes; }
size_t mem_count(void) { return mem_allocs; }

void mem_reset(void)
{
    mem_live_bytes = mem_peak_bytes = mem_total_bytes = mem_allocs = 0;
    n_phases = 0;
    phase_open = PHASE_CLOSED;
}

/* ---------------------------------------------------------------------------
 * Phase timing: split a run into "read" / "build" / "solve" so that the cost
 * of constructing a data structure can be reported apart from the cost of
 * using it.
 * ------------------------------------------------------------------------- */

double now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

void phase_begin(const char *name)
{
    if (phase_open != PHASE_CLOSED) phase_end();      /* defensive: close a leak */
    if (n_phases >= MEM_MAX_PHASES) {
        fprintf(stderr, "metrics: too many phases (max %d)\n", MEM_MAX_PHASES);
        exit(1);
    }
    mem_phase_t *p = &phases[n_phases];
    snprintf(p->name, MEM_PHASE_NAME, "%s", name ? name : "?");
    p->live_before = mem_live_bytes;
    p->live_peak   = mem_live_bytes;
    p->ms          = 0.0;
    phase_t0       = now_ms();
    phase_open     = n_phases;
    n_phases++;
}

void phase_end(void)
{
    if (phase_open == PHASE_CLOSED) return;
    mem_phase_t *p = &phases[phase_open];
    p->ms         = now_ms() - phase_t0;
    p->live_after = mem_live_bytes;
    mem_note_peak();
    phase_open = PHASE_CLOSED;
}

size_t phase_count(void)
{
    return n_phases;
}

const mem_phase_t *phase_at(size_t i)
{
    return (i < n_phases) ? &phases[i] : NULL;
}

/* ---------------------------------------------------------------------------
 * SplitMix64 PRNG: tiny, seedable, deterministic
 * ------------------------------------------------------------------------- */

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