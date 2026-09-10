#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitio.h"
#include "codes.h"
#include "graphcode.h"
#include "huffman.h"
#include "mtf.h"

static int passed = 0;
static int failed = 0;

static void check(const char *name, int ok)
{
    if (ok) { passed++; printf("  PASS %s\n", name); }
    else    { failed++; printf("  FAIL %s\n", name); }
}

static void finish_and_copy(bitwriter_t *w, uint8_t *buf, size_t *n_bytes, size_t cap)
{
    bitwriter_finish(w);
    *n_bytes = w->n_bytes;
    if (*n_bytes > cap) *n_bytes = cap;
    memcpy(buf, w->buf, *n_bytes);
    bitwriter_destroy(w);
}

static void test_bitio(void)
{
    bitwriter_t w;
    bitwriter_init(&w);
    bitwriter_write_bits(&w, 0b1011001, 7);
    bitwriter_write(&w, 1);
    uint8_t buf[64];
    size_t nb;
    finish_and_copy(&w, buf, &nb, sizeof(buf));

    bitreader_t r;
    bitreader_init(&r, buf, nb);
    uint64_t v = bitreader_read_bits(&r, 8);
    check("bitio: 0b10110011 == 179", v == 0b10110011);
    check("bitio: eof after 8 bits", bitreader_eof(&r));
}

static void test_gamma(void)
{
    /* gamma(1) = "1" */
    bitwriter_t w;
    bitwriter_init(&w);
    elias_gamma_encode(&w, 1);
    bitwriter_finish(&w);
    check("gamma(1) length == 1 bit", w.n_bytes == 1 && (w.buf[0] >> 7) == 1);
    bitwriter_destroy(&w);

    bitwriter_init(&w);
    elias_gamma_encode(&w, 2);
    elias_gamma_encode(&w, 3);
    bitwriter_finish(&w);
    /* "010" + "011" = 010011 */
    check("gamma(2) gamma(3) bits 010011",
          (w.buf[0] >> 6) == 0b01 && ((w.buf[0] >> 2) & 0xF) == 0b0011);
    bitwriter_destroy(&w);

    /* round trips 1..10000 via a tiny inline xorshift64 LCG */
    for (int pass = 0; pass < 3; pass++) {
        uint64_t state = 42 + pass;
        for (int i = 0; i < 2000; i++) {
            state ^= state << 13; state ^= state >> 7; state ^= state << 17;
            uint64_t x = (state % 10000) + 1;
            bitwriter_t w2;
            bitwriter_init(&w2);
            elias_gamma_encode(&w2, x);
            bitwriter_finish(&w2);
            bitreader_t r2;
            bitreader_init(&r2, w2.buf, w2.n_bytes);
            uint64_t y = elias_gamma_decode(&r2);
            if (x != y) { check("gamma roundtrip", 0); break; }
            bitwriter_destroy(&w2);
        }
    }
    check("gamma roundtrip 1..10000", 1);
}

static void test_delta_nibble_minbin(void)
{
    /* delta(2) = gamma(2) + one bit: "010" "0" */
    bitwriter_t w;
    bitwriter_init(&w);
    elias_delta_encode(&w, 2);
    bitwriter_finish(&w);
    check("delta(2) prefix 0100", (w.buf[0] >> 4) == 0b0100);
    bitwriter_destroy(&w);

    /* nibble(5) = "1101", nibble(1) = "1001" */
    bitwriter_init(&w);
    nibble_encode(&w, 5);
    bitwriter_finish(&w);
    check("nibble(5) == 1101", (w.buf[0] >> 4) == 0b1101);
    bitwriter_destroy(&w);

    bitwriter_init(&w);
    nibble_encode(&w, 1);
    bitwriter_finish(&w);
    check("nibble(1) == 1001", (w.buf[0] >> 4) == 0b1001);
    bitwriter_destroy(&w);

    /* minbin over z=5: values 0,1,2 -> 2 bits; 3,4 -> 3 bits (x+p) */
    bitwriter_init(&w);
    minbin_encode(&w, 3, 5);
    bitwriter_finish(&w);
    check("minbin(3,z=5) == 110", (w.buf[0] >> 5) == 0b110);
    bitwriter_destroy(&w);

    /* big round trip over all codecs */
    bitwriter_init(&w);
    for (uint64_t x = 0; x < 1000; x++) nibble_encode(&w, x);
    for (uint64_t x = 1; x < 1000; x++) elias_delta_encode(&w, x);
    for (uint64_t x = 0; x < 17; x++)   minbin_encode(&w, x, 17);
    for (uint64_t x = 1; x < 500; x++)  zeta_encode(&w, x, 2);
    for (uint64_t x = 1; x < 500; x++)  zeta_encode(&w, x, 3);
    bitwriter_finish(&w);

    bitreader_t r;
    bitreader_init(&r, w.buf, w.n_bytes);
    int ok = 1;
    for (uint64_t x = 0; x < 1000; x++) if (nibble_decode(&r) != x) ok = 0;
    for (uint64_t x = 1; x < 1000; x++) if (elias_delta_decode(&r) != x) ok = 0;
    for (uint64_t x = 0; x < 17; x++)   if (minbin_decode(&r, 17) != x) ok = 0;
    for (uint64_t x = 1; x < 500; x++)  if (zeta_decode(&r, 2) != x) ok = 0;
    for (uint64_t x = 1; x < 500; x++)  if (zeta_decode(&r, 3) != x) ok = 0;
    check("codes roundtrip", ok);
    bitwriter_destroy(&w);
}

static void test_huffman(void)
{
    const char *text = "the quick brown fox jumps over the lazy dog";
    size_t len = strlen(text);
    uint64_t freq[256] = {0};
    for (size_t i = 0; i < len; i++) freq[(uint8_t)text[i]]++;

    bitwriter_t w;
    bitwriter_init(&w);
    int ok = huffman_encode((const uint8_t *)text, len, freq, &w);
    check("huffman: encodes", ok);
    bitwriter_finish(&w);

    uint8_t *out = NULL;
    size_t out_len = 0;
    int dek = huffman_decode(w.buf, w.n_bytes, &out, &out_len);
    check("huffman: decodes", dek && out_len == len && memcmp(out, text, len) == 0);
    free(out);
    bitwriter_destroy(&w);

    /* single-symbol input */
    uint64_t f1[256] = {0};
    f1['a'] = 10;
    bitwriter_init(&w);
    ok = huffman_encode((const uint8_t *)"aaaa", 4, f1, &w);
    bitwriter_finish(&w);
    dek = huffman_decode(w.buf, w.n_bytes, &out, &out_len);
    check("huffman: single symbol roundtrip", ok && dek && out_len == 4 && out[0] == 'a');
    free(out);
    bitwriter_destroy(&w);
}

static void test_mtf_rle(void)
{
    const char *text = "bananaaa";
    size_t len = strlen(text);
    bitwriter_t w;
    bitwriter_init(&w);
    mtf_encode((const uint8_t *)text, len, &w);
    bitwriter_finish(&w);
    uint8_t *out = NULL;
    size_t out_len = 0;
    int ok = mtf_decode(w.buf, w.n_bytes, &out, &out_len);
    check("mtf: roundtrip", ok && out_len == len && memcmp(out, text, len) == 0);
    free(out);
    bitwriter_destroy(&w);

    /* RLE on the byte 0b00001111 (8 bits: 0-run of 4, 1-run of 4) */
    uint8_t data = 0b00001111;
    bitwriter_init(&w);
    rle_encode(&data, 1, &w);
    bitwriter_finish(&w);
    ok = rle_decode(w.buf, w.n_bytes, &out, &out_len);
    check("rle: roundtrip", ok && out_len == 1 && out[0] == data);
    free(out);
    bitwriter_destroy(&w);

    /* checkerboard */
    uint8_t cb = 0b10101010;
    bitwriter_init(&w);
    rle_encode(&cb, 1, &w);
    bitwriter_finish(&w);
    ok = rle_decode(w.buf, w.n_bytes, &out, &out_len);
    check("rle: checkerboard", ok && out_len == 1 && out[0] == cb);
    free(out);
    bitwriter_destroy(&w);
}

static void test_graphcodes(void)
{
    /* gap: slide example v=3, adj=[1,2,4,5] -> gaps -2,1,2,1 */
    igraph_integer_t adj[] = {1, 2, 4, 5};
    bitwriter_t w;
    bitwriter_init(&w);
    gap_encode_list(&w, 3, adj, 4);
    bitwriter_finish(&w);
    bitreader_t r;
    bitreader_init(&r, w.buf, w.n_bytes);
    igraph_integer_t dec[4];
    gap_decode_list(&r, 3, 4, dec);
    check("gap: slide example", dec[0] == 1 && dec[1] == 2 && dec[2] == 4 && dec[3] == 5);
    bitwriter_destroy(&w);

    /* reference: ref = adj(2) = [1,3,4], cur = adj(4) = [2,3,5,6] */
    igraph_integer_t ref[] = {1, 3, 4};
    igraph_integer_t cur[] = {2, 3, 5, 6};
    bitwriter_init(&w);
    ref_encode_list(&w, 2, 4, ref, 3, cur, 4);
    bitwriter_finish(&w);
    bitreader_init(&r, w.buf, w.n_bytes);
    igraph_integer_t dec2[8];
    igraph_integer_t deg2 = 0;
    ref_decode_list(&r, 2, 4, ref, 3, dec2, &deg2);
    check("reference: slide example",
          deg2 == 4 && dec2[0] == 2 && dec2[1] == 3 && dec2[2] == 5 && dec2[3] == 6);
    bitwriter_destroy(&w);

    /* interval: [1,2,3,5,6,9] -> [1,3],[5,6],[9,9] */
    igraph_integer_t iv[] = {1, 2, 3, 5, 6, 9};
    bitwriter_init(&w);
    interval_encode_list(&w, 4, iv, 6);
    bitwriter_finish(&w);
    bitreader_init(&r, w.buf, w.n_bytes);
    igraph_integer_t dec3[8];
    igraph_integer_t deg3 = 0;
    interval_decode_list(&r, 4, dec3, &deg3);
    check("interval: roundtrip",
          deg3 == 6 && dec3[0] == 1 && dec3[1] == 2 && dec3[2] == 3 &&
          dec3[3] == 5 && dec3[4] == 6 && dec3[5] == 9);
    bitwriter_destroy(&w);
}

int main(void)
{
    printf("test_compress\n");
    test_bitio();
    test_gamma();
    test_delta_nibble_minbin();
    test_huffman();
    test_mtf_rle();
    test_graphcodes();
    printf("%d passed, %d failed\n", passed, failed);
    return failed == 0 ? 0 : 1;
}
