/* cs_bench — timing benchmark of the CANSHIELD detector.
 *   cs_bench profile.bin out_prefix log1.log [log2.log ...]
 * Loads all frames into memory, then:
 *   1) throughput: whole stream through cs_detect, total time / frames (no per-frame timer);
 *   2) per-frame latency distribution (each call timed; includes timer overhead);
 *   3) input-independence: mean time on benign frames vs on synthetic random-payload frames vs unknown ids.
 * SPDX-License-Identifier: Apache-2.0 */
#include "../core/cs_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static inline uint64_t now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec; }
static int cmp64(const void *a, const void *b) { uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b; return x < y ? -1 : x > y; }
static uint64_t rng = 0x9E3779B97F4A7C15ull;
static uint64_t xr(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return rng; }

static double run_throughput(const cs_profile_t *P, cs_frame_t *fr, size_t n, int reps)
{
    static cs_detector_t D; cs_verdict_t v; volatile int sink = 0;
    double best = 1e30;
    for (int r = 0; r < reps; r++) {
        cs_detector_reset(&D);
        uint64_t t0 = now_ns();
        for (size_t i = 0; i < n; i++) { cs_detect(P, &D, &fr[i], &v); sink += v.votes; }
        double ns = (double)(now_ns() - t0) / (double)n; if (ns < best) best = ns;
    }
    (void)sink; return best;
}

int main(int argc, char **argv)
{
    if (argc < 4) { fprintf(stderr, "usage: %s profile.bin out_prefix log...\n", argv[0]); return 1; }
    static cs_profile_t P; FILE *pf = fopen(argv[1], "rb");
    if (!pf || fread(&P, sizeof P, 1, pf) != 1 || P.magic != CS_PROFILE_MAGIC) { fprintf(stderr, "bad profile\n"); return 1; }
    fclose(pf);
    size_t cap = 1 << 22, n = 0; cs_frame_t *fr = malloc(cap * sizeof *fr); char line[512];
    for (int a = 3; a < argc; a++) {
        FILE *f = fopen(argv[a], "r"); if (!f) { perror(argv[a]); return 1; }
        while (fgets(line, sizeof line, f)) {
            if (n == cap) { cap *= 2; fr = realloc(fr, cap * sizeof *fr); }
            if (cs_parse_candump(line, &fr[n])) n++;
        }
        fclose(f);
    }
    printf("frames loaded: %zu\n", n);
    double thr = run_throughput(&P, fr, n, 3);
    printf("throughput (best of 3, no per-frame timer): %.1f ns/frame = %.1f M frames/s\n", thr, 1e3 / thr);

    /* per-frame latency distribution */
    uint64_t *lat = malloc(n * sizeof *lat); static cs_detector_t D; cs_detector_reset(&D); cs_verdict_t v;
    uint64_t tovh = now_ns(); for (int i = 0; i < 1000000; i++) (void)now_ns(); tovh = (now_ns() - tovh) / 1000000;
    for (size_t i = 0; i < n; i++) { uint64_t t0 = now_ns(); cs_detect(&P, &D, &fr[i], &v); lat[i] = now_ns() - t0; }
    qsort(lat, n, sizeof *lat, cmp64);
    printf("per-frame (timer overhead ~%llu ns included): p50 %llu  p99 %llu  p99.9 %llu  p99.99 %llu  max %llu ns\n",
           (unsigned long long)tovh, (unsigned long long)lat[n / 2], (unsigned long long)lat[n * 99 / 100],
           (unsigned long long)lat[n * 999 / 1000], (unsigned long long)lat[n * 9999 / 10000], (unsigned long long)lat[n - 1]);
    char path[512]; snprintf(path, sizeof path, "%s_latency_ns.bin", argv[2]);
    FILE *o = fopen(path, "wb"); if (o) { fwrite(lat, sizeof *lat, n, o); fclose(o); }

    /* input independence: same count of frames, three input classes */
    size_t m = n < 2000000 ? n : 2000000;
    cs_frame_t *rnd = malloc(m * sizeof *rnd), *unk = malloc(m * sizeof *unk);
    for (size_t i = 0; i < m; i++) {
        rnd[i] = fr[i]; uint64_t r = xr(); memcpy(rnd[i].data, &r, 8); rnd[i].dlc = 8;      /* known ids, random payload */
        unk[i] = fr[i]; unk[i].id = 0x7F0u + (uint32_t)(xr() % 15u);                        /* unknown ids */
    }
    double tb = run_throughput(&P, fr, m, 3), tr = run_throughput(&P, rnd, m, 3), tu = run_throughput(&P, unk, m, 3);
    printf("input independence (%zu frames each): real traffic %.1f ns | random payloads %.1f ns | unknown ids %.1f ns\n", m, tb, tr, tu);
    free(fr); free(lat); free(rnd); free(unk);
    return 0;
}
