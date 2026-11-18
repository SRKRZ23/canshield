/* Public client API of canshieldd (/dev/canshield). SPDX-License-Identifier: Apache-2.0 */
#ifndef CANSHIELD_API_H
#define CANSHIELD_API_H
#include <stdint.h>
#include <devctl.h>
#include <sys/siginfo.h>

#define CS_PATH         "/dev/canshield"
#define CS_HIST_BUCKETS 10
#define CS_MAX_NOTIFY   8

typedef struct {
    uint64_t frames, flagged, alarm_frames, alarm_events, forwarded, dropped;
    uint64_t det_sum_ns, det_max_ns, gw_max_ns;
    uint64_t det_hist[CS_HIST_BUCKETS], gw_hist[CS_HIST_BUCKETS];
    uint32_t last_alarm_id; uint64_t last_alarm_frame;
} cs_stats_t;

#define CS_DEVCTL_GET_STATS __DIOF(_DCMD_MISC, 0x70, cs_stats_t)
#define CS_DEVCTL_NOTIFY    __DIOT(_DCMD_MISC, 0x71, struct sigevent)
#endif
