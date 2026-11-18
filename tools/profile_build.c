/* Build a CANSHIELD normal-behaviour profile from benign candump logs.
 * usage: profile_build out.bin benign1.log [benign2.log ...]
 * SPDX-License-Identifier: Apache-2.0 */
#include "../core/canshield.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RES 40000
typedef struct { uint32_t n_dt; uint64_t seen_dt; uint32_t *dt; uint32_t dlc_hist[9]; uint64_t frames; } acc_t;

static uint64_t rng = 88172645463325252ull;
static uint64_t xr(void) { rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17; return rng; }
static int cmpu(const void *a, const void *b) { uint32_t x = *(const uint32_t*)a, y = *(const uint32_t*)b; return x < y ? -1 : x > y; }

int main(int argc, char **argv)
{
    if (argc < 3) { fprintf(stderr, "usage: %s out.bin benign.log...\n", argv[0]); return 1; }
    static cs_profile_t P; memset(&P, 0, sizeof P); P.magic = CS_PROFILE_MAGIC;
    static acc_t A[CS_NIDS];
    for (unsigned i = 0; i < CS_NIDS; i++) { memset(P.id[i].byte_min, 0xFF, 8); }
    char line[512]; cs_frame_t f;
    for (int a = 2; a < argc; a++) {
        FILE *fp = fopen(argv[a], "r"); if (!fp) { perror(argv[a]); return 1; }
        static cs_state_t st; cs_state_reset(&st);
        uint64_t n = 0;
        while (fgets(line, sizeof line, fp)) {
            if (!cs_parse_candump(line, &f) || f.id >= CS_NIDS) continue;
            cs_idprof_t *pr = &P.id[f.id]; acc_t *ac = &A[f.id]; cs_idstate_t *s = &st.id[f.id];
            pr->known = 1; ac->frames++; ac->dlc_hist[f.dlc]++;
            for (int i = 0; i < f.dlc; i++) {
                if (f.data[i] < pr->byte_min[i]) pr->byte_min[i] = f.data[i];
                if (f.data[i] > pr->byte_max[i]) pr->byte_max[i] = f.data[i];
            }
            if (s->seen) {
                uint64_t dt = f.t_us - s->last_t;
                if (!ac->dt) ac->dt = malloc(RES * sizeof(uint32_t));
                uint32_t d32 = dt > 0xFFFFFFFFull ? 0xFFFFFFFFu : (uint32_t)dt;
                if (ac->n_dt < RES) ac->dt[ac->n_dt++] = d32;
                else { uint64_t j = xr() % (ac->seen_dt + 1); if (j < RES) ac->dt[j] = d32; }
                ac->seen_dt++;
                for (int i = 0; i < 8; i++) {
                    uint8_t v = f.data[i], pv = s->last[i];
                    pr->toggled[i] |= (uint8_t)(v ^ pv);
                    int d = v > pv ? v - pv : pv - v;
                    if (d > pr->max_delta[i]) pr->max_delta[i] = (uint8_t)d;
                }
            }
            s->seen = 1; s->last_t = f.t_us; memcpy(s->last, f.data, 8); n++;
        }
        fclose(fp);
        fprintf(stderr, "%s: %llu frames\n", argv[a], (unsigned long long)n);
    }
    for (unsigned i = 0; i < CS_NIDS; i++) {
        cs_idprof_t *pr = &P.id[i]; acc_t *ac = &A[i];
        if (!pr->known) continue;
        P.n_known++;
        int best = 0; for (int d = 1; d < 9; d++) if (ac->dlc_hist[d] > ac->dlc_hist[best]) best = d;
        pr->dlc = (uint8_t)best;
        for (int b = 0; b < 8; b++) if (pr->byte_min[b] > pr->byte_max[b]) { pr->byte_min[b] = 0; pr->byte_max[b] = 0; }
        if (ac->n_dt >= 20) {
            qsort(ac->dt, ac->n_dt, sizeof(uint32_t), cmpu);
            pr->dt_p01 = ac->dt[ac->n_dt / 100];
            pr->dt_p50 = ac->dt[ac->n_dt / 2];
            pr->dt_p99 = ac->dt[ac->n_dt - 1 - ac->n_dt / 100];
        }
        free(ac->dt);
    }
    FILE *o = fopen(argv[1], "wb"); if (!o) { perror(argv[1]); return 1; }
    fwrite(&P, sizeof P, 1, o); fclose(o);
    fprintf(stderr, "profile: %u known ids -> %s (%zu bytes)\n", P.n_known, argv[1], sizeof P);
    return 0;
}
