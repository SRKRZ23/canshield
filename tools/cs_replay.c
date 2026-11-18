/* Replay a candump log through the C detector.
 *   cs_replay profile.bin parity.bin            -> C model == Python hardened model on every pattern
 *   cs_replay profile.bin in.log [--bench]      -> per-frame verdicts summary + ns/frame timing
 * SPDX-License-Identifier: Apache-2.0 */
#include "../core/cs_detector.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_ns(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec * 1e9 + t.tv_nsec; }

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: %s profile.bin (parity.bin | in.log [--bench])\n", argv[0]); return 1; }
    static cs_profile_t P; FILE *pf = fopen(argv[1], "rb");
    if (!pf || fread(&P, sizeof P, 1, pf) != 1 || P.magic != CS_PROFILE_MAGIC) { fprintf(stderr, "bad profile\n"); return 1; }
    fclose(pf);
    const char *in = argv[2]; size_t L = strlen(in);
    if (L > 4 && !strcmp(in + L - 4, ".bin")) {             /* parity mode: records {u64 feat; i32 votes} */
        FILE *f = fopen(in, "rb"); if (!f) { perror(in); return 1; }
        uint64_t feat; int32_t want; long n = 0, bad = 0;
        while (fread(&feat, 8, 1, f) == 1 && fread(&want, 4, 1, f) == 1) { n++; if (cs_model_votes(feat) != want) bad++; }
        fclose(f);
        printf("parity: %ld patterns, %ld mismatches -> %s\n", n, bad, bad ? "FAIL" : "OK (C == hardened Python model)");
        return bad != 0;
    }
    int bench = argc > 3 && !strcmp(argv[3], "--bench");
    FILE *f = fopen(in, "r"); if (!f) { perror(in); return 1; }
    static cs_frame_t *fr; size_t cap = 1 << 20, n = 0; fr = malloc(cap * sizeof *fr);
    char line[512];
    while (fgets(line, sizeof line, f)) { if (n == cap) { cap *= 2; fr = realloc(fr, cap * sizeof *fr); } if (cs_parse_candump(line, &fr[n])) n++; }
    fclose(f);
    static cs_detector_t D; cs_detector_reset(&D);
    cs_verdict_t v; long flagged = 0, ida = 0, busa = 0, first_alarm = -1;
    double worst = 0, t_all = now_ns();
    for (size_t i = 0; i < n; i++) {
        double t0 = bench ? now_ns() : 0;
        cs_detect(&P, &D, &fr[i], &v);
        if (bench) { double dt = now_ns() - t0; if (dt > worst) worst = dt; }
        flagged += v.flagged; ida += v.id_alarm; busa += v.bus_alarm;
        if ((v.id_alarm || v.bus_alarm) && first_alarm < 0) first_alarm = (long)i;
    }
    double tot = now_ns() - t_all;
    printf("%zu frames | flagged %ld | id-alarm frames %ld | bus-alarm frames %ld | first alarm at frame %ld (t=%.3fs)\n",
           n, flagged, ida, busa, first_alarm, first_alarm >= 0 ? (fr[first_alarm].t_us - fr[0].t_us) / 1e6 : -1.0);
    printf("throughput: %.1f ns/frame mean%s\n", tot / (double)n, bench ? "" : " (incl. no per-frame timer)");
    if (bench) printf("worst single frame (incl. timer overhead): %.0f ns\n", worst);
    free(fr);
    return 0;
}
