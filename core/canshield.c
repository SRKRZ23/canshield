/* SPDX-License-Identifier: Apache-2.0 */
#include "canshield.h"
#include <string.h>
#include <stdlib.h>

void cs_state_reset(cs_state_t *st) { memset(st, 0, sizeof(*st)); }

static const char *NAMES[CS_NFEAT] = {
    "unknown_id", "dlc_mismatch", "first_seen",
    "dt<0.25*p01", "dt<0.5*p01", "dt<0.8*p01", "dt>3*p99",
    "oor0","oor1","oor2","oor3","oor4","oor5","oor6","oor7",
    "dlt0","dlt1","dlt2","dlt3","dlt4","dlt5","dlt6","dlt7",
    "untog0","untog1","untog2","untog3","untog4","untog5","untog6","untog7",
    "all_ff", "repeat_payload", "any_oor", "any_delta", "any_untoggled"
};
const char *cs_feature_name(int i) { return (i >= 0 && i < CS_NFEAT) ? NAMES[i] : "?"; }

uint64_t cs_features(const cs_profile_t *p, cs_state_t *st, const cs_frame_t *f)
{
    uint64_t b = 0;
    if (f->id >= CS_NIDS) return 1ull;                 /* unknown_id */
    const cs_idprof_t *pr = &p->id[f->id];
    cs_idstate_t *s = &st->id[f->id];
    if (!pr->known) b |= 1ull << 0;
    if (pr->known && f->dlc != pr->dlc) b |= 1ull << 1;
    if (!s->seen) b |= 1ull << 2;

    if (s->seen && pr->known && pr->dt_p01) {
        uint64_t dt = f->t_us - s->last_t;
        uint64_t p01 = pr->dt_p01, p99 = pr->dt_p99;
        if (dt * 4  < p01)      b |= 1ull << 3;
        if (dt * 2  < p01)      b |= 1ull << 4;
        if (dt * 10 < p01 * 8)  b |= 1ull << 5;
        if (dt > p99 * 3)       b |= 1ull << 6;
    }
    int any_oor = 0, any_dlt = 0, any_unt = 0, rep = s->seen, ff = (f->dlc == 8);
    for (int i = 0; i < 8; i++) {              /* fixed 8 iterations: constant time */
        uint8_t v = f->data[i];
        if (ff && v != 0xFF) ff = 0;
        if (pr->known && i < f->dlc) {
            if (v < pr->byte_min[i] || v > pr->byte_max[i]) { b |= 1ull << (7 + i); any_oor = 1; }
            if (s->seen) {
                uint8_t prev = s->last[i];
                int d = v > prev ? v - prev : prev - v;
                if (d > pr->max_delta[i]) { b |= 1ull << (15 + i); any_dlt = 1; }
                if ((uint8_t)(v ^ prev) & (uint8_t)~pr->toggled[i]) { b |= 1ull << (23 + i); any_unt = 1; }
            }
        }
        if (s->seen && v != s->last[i]) rep = 0;
    }
    if (ff)      b |= 1ull << 31;
    if (rep)     b |= 1ull << 32;
    if (any_oor) b |= 1ull << 33;
    if (any_dlt) b |= 1ull << 34;
    if (any_unt) b |= 1ull << 35;

    s->seen = 1; s->last_t = f->t_us;
    memcpy(s->last, f->data, 8);
    return b;
}

static int hexv(int c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

int cs_parse_candump(const char *line, cs_frame_t *f)
{
    const char *p = strchr(line, '(');
    if (!p) return 0;
    char *e;
    long long sec = strtoll(p + 1, &e, 10);
    if (*e != '.') return 0;
    long long frac = 0; int nd = 0; e++;
    while (*e >= '0' && *e <= '9') { if (nd < 6) { frac = frac * 10 + (*e - '0'); nd++; } e++; }
    while (nd < 6) { frac *= 10; nd++; }
    f->t_us = (uint64_t)sec * 1000000ull + (uint64_t)frac;
    const char *h = strchr(e, '#');
    if (!h) return 0;
    const char *q = h; while (q > e && q[-1] != ' ') q--;
    uint32_t id = 0;
    for (const char *r = q; r < h; r++) { int v = hexv(*r); if (v < 0) return 0; id = (id << 4) | (uint32_t)v; }
    f->id = id;
    memset(f->data, 0, 8);
    int n = 0; h++;
    while (n < 8) { int a = hexv(h[0]), c = (a >= 0) ? hexv(h[1]) : -1; if (a < 0 || c < 0) break; f->data[n++] = (uint8_t)(a * 16 + c); h += 2; }
    f->dlc = (uint8_t)n;
    return 1;
}
