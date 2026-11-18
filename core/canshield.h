/* CANSHIELD core — deterministic, allocation-free CAN frame feature extraction.
 * Same code is compiled for macOS (training/eval) and QNX 8.0 (runtime).
 * SPDX-License-Identifier: Apache-2.0 */
#ifndef CANSHIELD_H
#define CANSHIELD_H
#include <stdint.h>

#define CS_NIDS 4096u              /* 12-bit table: 11-bit standard IDs + ROAD marker 0xFFF */
#define CS_NFEAT 36                /* number of binary features in cs_features() */

typedef struct {
    uint32_t id;                   /* 11-bit id (frames with id >= 2048 are treated as unknown) */
    uint8_t  dlc;
    uint8_t  data[8];
    uint64_t t_us;                 /* timestamp, microseconds */
} cs_frame_t;

/* Learned "normal behaviour" of one CAN id (built offline from benign captures). */
typedef struct {
    uint8_t  known;                /* id was seen in benign traffic */
    uint8_t  dlc;                  /* most common DLC */
    uint8_t  byte_min[8], byte_max[8];
    uint8_t  toggled[8];           /* bits that ever changed between consecutive frames */
    uint8_t  max_delta[8];         /* max |byte - previous byte| */
    uint32_t dt_p01, dt_p50, dt_p99; /* inter-arrival percentiles, microseconds */
} cs_idprof_t;

typedef struct {
    uint32_t magic;                /* 'CSP1' */
    uint32_t n_known;
    cs_idprof_t id[CS_NIDS];
} cs_profile_t;

/* Per-id runtime state (fixed size, no allocation). */
typedef struct {
    uint8_t  seen;
    uint8_t  last[8];
    uint64_t last_t;
} cs_idstate_t;

typedef struct {
    cs_idstate_t id[CS_NIDS];
} cs_state_t;

#define CS_PROFILE_MAGIC 0x31505343u /* "CSP1" little-endian */

void     cs_state_reset(cs_state_t *st);
/* Computes the feature bit-vector for one frame and updates the per-id state.
 * Constant work per frame: no loops over history, no allocation. */
uint64_t cs_features(const cs_profile_t *p, cs_state_t *st, const cs_frame_t *f);
/* Human-readable name of feature bit i (for reports). */
const char *cs_feature_name(int i);

/* Parse one candump log line "(1140000000.000001) can0 354#1FFF40000003E380". Returns 1 on success. */
int cs_parse_candump(const char *line, cs_frame_t *f);

#endif
