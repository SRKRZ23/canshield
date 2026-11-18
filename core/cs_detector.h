/* CANSHIELD detector = features (canshield.c) + hardened logic-gate model (cs_model.c) + alarms.
 * Everything is fixed-size and allocation-free; per-frame work is constant.
 * SPDX-License-Identifier: Apache-2.0 */
#ifndef CS_DETECTOR_H
#define CS_DETECTOR_H
#include "canshield.h"

#define CS_ALARM_K 4        /* per-id: >= K flagged among the last 8 frames of that id */
#define CS_BUS_K   2        /* bus: >= 2 flagged unknown-id frames among the last 64 bus frames */

typedef struct {
    cs_state_t feat;                 /* per-id feature state */
    uint8_t    win[CS_NIDS];         /* per-id 8-frame verdict shift register */
    uint64_t   bus;                  /* last 64 bus frames: flagged && unknown id */
} cs_detector_t;

typedef struct {
    uint64_t features;
    int      votes;                  /* 0..16 */
    uint8_t  flagged;                /* frame verdict */
    uint8_t  id_alarm;               /* per-id alarm */
    uint8_t  bus_alarm;              /* bus-level alarm (fuzzing / unknown ids) */
} cs_verdict_t;

int  cs_model_votes(uint64_t feat);  /* generated: build/cs_model.c */
extern const int cs_model_threshold;
void cs_detector_reset(cs_detector_t *d);
void cs_detect(const cs_profile_t *p, cs_detector_t *d, const cs_frame_t *f, cs_verdict_t *v);
#endif
