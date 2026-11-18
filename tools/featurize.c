/* Turn a candump log into labelled CANSHIELD feature records.
 * usage: featurize profile.bin in.log out.bin [t_start t_end id|any pattern]
 *   pattern: 16 hex chars, X = wildcard (e.g. XXXXXXXXXXFFXXXX). Times are seconds from the first frame.
 *   Without the 4 label args every frame is labelled benign (0).
 * record: { uint64 feat; uint16 id; uint8 label; uint8 dlc; uint32 t_ms } = 16 bytes
 * SPDX-License-Identifier: Apache-2.0 */
#include "../core/canshield.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#pragma pack(push, 1)
typedef struct { uint64_t feat; uint16_t id; uint8_t label; uint8_t dlc; uint32_t t_ms; } rec_t;
#pragma pack(pop)

int main(int argc, char **argv)
{
    if (argc != 4 && argc != 8) { fprintf(stderr, "usage: %s profile.bin in.log out.bin [t0 t1 id|any pattern]\n", argv[0]); return 1; }
    static cs_profile_t P; FILE *pf = fopen(argv[1], "rb");
    if (!pf || fread(&P, sizeof P, 1, pf) != 1 || P.magic != CS_PROFILE_MAGIC) { fprintf(stderr, "bad profile\n"); return 1; }
    fclose(pf);
    int labelled = (argc == 8);
    double t0 = 0, t1 = 0; long lid = -1; int pat_on[8] = {0}; uint8_t pat[8] = {0};
    if (labelled) {
        t0 = atof(argv[4]); t1 = atof(argv[5]);
        lid = strcmp(argv[6], "any") ? strtol(argv[6], NULL, 16) : -1;
        const char *s = argv[7];
        for (int i = 0; i < 8 && s[2*i] && s[2*i+1]; i++) {
            if (toupper((unsigned char)s[2*i]) == 'X') continue;
            char h[3] = { s[2*i], s[2*i+1], 0 }; pat[i] = (uint8_t)strtol(h, NULL, 16); pat_on[i] = 1;
        }
    }
    FILE *in = fopen(argv[2], "r"), *out = fopen(argv[3], "wb");
    if (!in || !out) { perror("open"); return 1; }
    static cs_state_t st; cs_state_reset(&st);
    char line[512]; cs_frame_t f; uint64_t first = 0; int have_first = 0;
    uint64_t n = 0, pos = 0;
    while (fgets(line, sizeof line, in)) {
        if (!cs_parse_candump(line, &f)) continue;
        if (!have_first) { first = f.t_us; have_first = 1; }
        rec_t r; r.feat = cs_features(&P, &st, &f); r.id = (uint16_t)(f.id & 0xFFFF); r.dlc = f.dlc;
        double rel = (double)(f.t_us - first) / 1e6; r.t_ms = (uint32_t)(rel * 1000.0);
        r.label = 0;
        if (labelled && rel >= t0 && rel <= t1 && (lid < 0 || (long)f.id == lid)) {
            int m = 1; for (int i = 0; i < 8; i++) if (pat_on[i] && f.data[i] != pat[i]) { m = 0; break; }
            r.label = (uint8_t)m;
        }
        pos += r.label; n++;
        fwrite(&r, sizeof r, 1, out);
    }
    fclose(in); fclose(out);
    fprintf(stderr, "%s: %llu frames, %llu attack\n", argv[2], (unsigned long long)n, (unsigned long long)pos);
    return 0;
}
