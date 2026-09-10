#include "huffman.h"

#include <stdlib.h>

#include "codes.h"
#include "util.h"

#define NSYM 256

/* Build the Huffman tree from freq[] and fill lens[sym] (0 = unused).
 * Returns 1 on success, 0 on empty input, -1 if a code would exceed the
 * 5-bit header (depth > 30). */
static int build_lengths(const uint64_t freq[256], uint8_t lens[256])
{
    for (int s = 0; s < NSYM; s++) lens[s] = 0;

    /* node pool: ids 0..active-1 are leaves, internal nodes follow */
    uint64_t w[2 * NSYM];
    int32_t  lc[2 * NSYM], rc[2 * NSYM];
    int32_t  leaf_sym[NSYM];            /* leaf id -> symbol */
    int32_t  active = 0;

    for (int s = 0; s < NSYM; s++) {
        if (freq[s] > 0) {
            w[active] = freq[s];
            lc[active] = rc[active] = -1;
            leaf_sym[active] = s;
            active++;
        }
    }
    if (active == 0) return 0;
    if (active == 1) {
        lens[leaf_sym[0]] = 1;          /* single symbol: code "0" */
        return 1;
    }

    /* binary min-heap over (weight, node id) */
    uint64_t hw[2 * NSYM];
    int32_t  hn[2 * NSYM];
    int64_t hsize = 0;

    for (int32_t i = 0; i < active; i++) {
        hw[hsize] = w[i]; hn[hsize] = i; hsize++;
        int64_t k = hsize - 1;
        while (k > 0) {
            int64_t p = (k - 1) / 2;
            if (hw[p] <= hw[k]) break;
            uint64_t tw = hw[p]; hw[p] = hw[k]; hw[k] = tw;
            int32_t tn = hn[p]; hn[p] = hn[k]; hn[k] = tn;
            k = p;
        }
    }

    int32_t next = active;
    while (hsize > 1) {
        /* pop min twice, then merge */
        uint64_t w1 = hw[0]; int32_t n1 = hn[0];
        hw[0] = hw[hsize - 1]; hn[0] = hn[hsize - 1]; hsize--;
        int64_t k = 0;
        for (;;) {
            int64_t l = 2 * k + 1, r = 2 * k + 2, m = k;
            if (l < hsize && hw[l] < hw[m]) m = l;
            if (r < hsize && hw[r] < hw[m]) m = r;
            if (m == k) break;
            uint64_t tw = hw[m]; hw[m] = hw[k]; hw[k] = tw;
            int32_t tn = hn[m]; hn[m] = hn[k]; hn[k] = tn;
            k = m;
        }

        uint64_t w2 = hw[0]; int32_t n2 = hn[0];
        hw[0] = hw[hsize - 1]; hn[0] = hn[hsize - 1]; hsize--;
        k = 0;
        for (;;) {
            int64_t l = 2 * k + 1, r = 2 * k + 2, m = k;
            if (l < hsize && hw[l] < hw[m]) m = l;
            if (r < hsize && hw[r] < hw[m]) m = r;
            if (m == k) break;
            uint64_t tw = hw[m]; hw[m] = hw[k]; hw[k] = tw;
            int32_t tn = hn[m]; hn[m] = hn[k]; hn[k] = tn;
            k = m;
        }

        w[next] = w1 + w2;
        lc[next] = n1;
        rc[next] = n2;
        hw[hsize] = w[next]; hn[hsize] = next; hsize++;
        k = hsize - 1;
        while (k > 0) {
            int64_t p = (k - 1) / 2;
            if (hw[p] <= hw[k]) break;
            uint64_t tw = hw[p]; hw[p] = hw[k]; hw[k] = tw;
            int32_t tn = hn[p]; hn[p] = hn[k]; hn[k] = tn;
            k = p;
        }
        next++;
    }
    int32_t root = hn[0];

    /* depths via iterative stack; the 5-bit header caps lengths at 30 */
    typedef struct { int32_t node; unsigned depth; } stk_t;
    stk_t stk[2 * NSYM];
    int64_t sp = 0;
    stk[sp++] = (stk_t){root, 0};
    while (sp > 0) {
        stk_t fr = stk[--sp];
        if (lc[fr.node] == -1) {                     /* leaf */
            if (fr.depth > 30) return -1;            /* would not fit header */
            lens[leaf_sym[fr.node]] = (uint8_t)fr.depth;
        } else {
            stk[sp++] = (stk_t){lc[fr.node], fr.depth + 1};
            stk[sp++] = (stk_t){rc[fr.node], fr.depth + 1};
        }
    }
    return 1;
}

/* Canonical code assignment: symbols sorted by (len, sym); RFC1951-style
 * stepping.  Codes are MSB-first and prefix-free by construction. */
static void canonical_codes(const uint8_t lens[256], uint64_t code[256])
{
    uint8_t order[256];
    int k = 0;
    for (int s = 0; s < NSYM; s++)
        if (lens[s] > 0) order[k++] = (uint8_t)s;
    for (int i = 0; i < k; i++)
        for (int j = i + 1; j < k; j++) {
            int swap = 0;
            if (lens[order[j]] < lens[order[i]]) swap = 1;
            else if (lens[order[j]] == lens[order[i]] && order[j] < order[i]) swap = 1;
            if (swap) { uint8_t t = order[i]; order[i] = order[j]; order[j] = t; }
        }

    uint64_t cur = 0;
    unsigned prev = 0;
    for (int i = 0; i < k; i++) {
        unsigned L = lens[order[i]];
        if (i > 0) cur = (cur + 1) << (L - prev);
        code[order[i]] = cur;
        prev = L;
    }
}

bool huffman_encode(const uint8_t *data, size_t len, const uint64_t freq[256],
                    bitwriter_t *out)
{
    uint8_t lens[NSYM];
    int ok = build_lengths(freq, lens);
    if (ok != 1) return false;

    uint64_t code[NSYM];
    canonical_codes(lens, code);

    /* header: original length, then the code-length table */
    for (int i = 0; i < 8; i++)
        bitwriter_write_bits(out, (len >> (56 - 8 * i)) & 0xFF, 8);
    for (int s = 0; s < NSYM; s++)
        bitwriter_write_bits(out, lens[s], 5);

    for (size_t i = 0; i < len; i++) {
        uint8_t s = data[i];
        bitwriter_write_bits(out, code[s], lens[s]);
    }
    return true;
}

bool huffman_decode(const uint8_t *bits, size_t n_bytes,
                    uint8_t **out, size_t *out_len)
{
    bitreader_t r;
    bitreader_init(&r, bits, n_bytes);

    uint64_t len = 0;
    for (int i = 0; i < 8; i++) len = (len << 8) | bitreader_read_bits(&r, 8);

    uint8_t lens[NSYM];
    for (int s = 0; s < NSYM; s++) lens[s] = (uint8_t)bitreader_read_bits(&r, 5);

    uint64_t code[NSYM];
    canonical_codes(lens, code);

    /* rebuild the decode trie (node 0 = root) from the canonical codes */
    int32_t left[2 * NSYM], right[2 * NSYM];
    uint8_t leaf[2 * NSYM];
    bool is_leaf[2 * NSYM];
    for (int i = 0; i < 2 * NSYM; i++) { left[i] = right[i] = -1; is_leaf[i] = false; }
    int32_t next = 1;

    for (int s = 0; s < NSYM; s++) {
        if (lens[s] == 0) continue;
        int32_t node = 0;
        for (unsigned i = lens[s]; i-- > 0;) {
            int bit = (int)((code[s] >> i) & 1);
            int32_t *slot = bit ? &right[node] : &left[node];
            if (*slot == -1) {
                *slot = next++;
                left[*slot] = right[*slot] = -1;
                is_leaf[*slot] = false;
            }
            node = *slot;
        }
        is_leaf[node] = true;
        leaf[node] = (uint8_t)s;
    }

    uint8_t *res = NULL;
    size_t rcap = 0, rlen = 0;
    int32_t node = 0;
    while (!bitreader_eof(&r) && rlen < len) {
        int bit = (int)bitreader_read(&r);
        node = bit ? right[node] : left[node];
        if (node == -1) break;                      /* corrupted stream */
        if (is_leaf[node]) {
            if (rcap == rlen) {
                rcap = rcap ? 2 * rcap : 64;
                res = xrealloc(res, rcap);
            }
            res[rlen++] = leaf[node];
            node = 0;
        }
    }
    if (rlen != len) { free(res); return false; }
    *out = res;
    *out_len = rlen;
    return true;
}
