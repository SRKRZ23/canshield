/* cs_stat — print canshieldd statistics as JSON (one line), optionally every N ms.
 * usage: cs_stat [-w interval_ms]      (uses devctl CS_DEVCTL_GET_STATS on /dev/canshield)
 * SPDX-License-Identifier: Apache-2.0 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "canshield_api.h"

static void print_json(const cs_stats_t *s)
{
    struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
    printf("{\"t\":%ld.%03ld,\"frames\":%llu,\"flagged\":%llu,\"alarm_frames\":%llu,\"alarm_events\":%llu,"
           "\"forwarded\":%llu,\"dropped\":%llu,\"det_mean_ns\":%llu,\"det_max_ns\":%llu,\"gw_max_ns\":%llu,"
           "\"last_alarm_id\":%u,\"det_hist\":[", (long)ts.tv_sec, ts.tv_nsec / 1000000,
           (unsigned long long)s->frames, (unsigned long long)s->flagged, (unsigned long long)s->alarm_frames,
           (unsigned long long)s->alarm_events, (unsigned long long)s->forwarded, (unsigned long long)s->dropped,
           (unsigned long long)(s->frames ? s->det_sum_ns / s->frames : 0), (unsigned long long)s->det_max_ns,
           (unsigned long long)s->gw_max_ns, s->last_alarm_id);
    for (int b = 0; b < CS_HIST_BUCKETS; b++) printf("%s%llu", b ? "," : "", (unsigned long long)s->det_hist[b]);
    printf("]}\n"); fflush(stdout);
}

int main(int argc, char **argv)
{
    int every = 0, opt;
    while ((opt = getopt(argc, argv, "w:")) != -1) if (opt == 'w') every = atoi(optarg);
    for (;;) {
        int fd = open(CS_PATH, O_RDONLY);
        cs_stats_t s; memset(&s, 0, sizeof s);
        if (fd >= 0) { devctl(fd, CS_DEVCTL_GET_STATS, &s, sizeof s, NULL); close(fd); print_json(&s); }
        else { printf("{\"error\":\"%s\"}\n", strerror(errno)); fflush(stdout); }
        if (!every) return 0;
        usleep((useconds_t)every * 1000u);
    }
}
