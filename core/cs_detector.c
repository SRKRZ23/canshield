/* SPDX-License-Identifier: Apache-2.0 */
#include "cs_detector.h"
#include <string.h>

void cs_detector_reset(cs_detector_t *d) { memset(d, 0, sizeof(*d)); }

void cs_detect(const cs_profile_t *p, cs_detector_t *d, const cs_frame_t *f, cs_verdict_t *v)
{
    v->features = cs_features(p, &d->feat, f);
    v->votes = cs_model_votes(v->features);
    v->flagged = (uint8_t)(v->votes >= cs_model_threshold);
    uint32_t id = f->id < CS_NIDS ? f->id : CS_NIDS - 1;
    d->win[id] = (uint8_t)((d->win[id] << 1) | v->flagged);
    v->id_alarm = (uint8_t)(__builtin_popcount(d->win[id]) >= CS_ALARM_K);
    d->bus = (d->bus << 1) | (uint64_t)(v->flagged & (uint8_t)(v->features & 1u));
    v->bus_alarm = (uint8_t)(__builtin_popcountll(d->bus) >= CS_BUS_K);
}
