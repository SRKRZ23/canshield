/* cs_watch — subscribe to canshieldd alarms with a QNX pulse (no polling).
 * usage: cs_watch            (prints one line per alarm event, with detection-time stats)
 * SPDX-License-Identifier: Apache-2.0 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include "canshield_api.h"

int main(void)
{
    int fd = open(CS_PATH, O_RDONLY);
    if (fd < 0) { perror(CS_PATH); return 1; }
    int chid = ChannelCreate(_NTO_CHF_PRIVATE), coid = ConnectAttach(0, 0, chid, _NTO_SIDE_CHANNEL, 0);
    struct sigevent ev; SIGEV_PULSE_INIT(&ev, coid, SIGEV_PULSE_PRIO_INHERIT, _PULSE_CODE_MINAVAIL, 0);
    if (MsgRegisterEvent(&ev, fd) == -1) { perror("MsgRegisterEvent"); return 1; }
    int r = devctl(fd, CS_DEVCTL_NOTIFY, &ev, sizeof ev, NULL);
    if (r != EOK) { fprintf(stderr, "notify: %s\n", strerror(r)); return 1; }
    printf("cs_watch: subscribed to %s alarms (pulse)\n", CS_PATH); fflush(stdout);
    struct _pulse p; uint64_t last_events = 0;
    for (;;) {
        if (MsgReceivePulse(chid, &p, sizeof p, NULL) == -1) { perror("MsgReceivePulse"); return 1; }
        cs_stats_t s; if (devctl(fd, CS_DEVCTL_GET_STATS, &s, sizeof s, NULL) != EOK) continue;
        if (s.alarm_events == last_events) continue;
        last_events = s.alarm_events;
        struct timespec ts; clock_gettime(CLOCK_REALTIME, &ts);
        printf("[%ld.%03ld] ALARM #%llu  id=0x%03X  frames=%llu dropped=%llu  detect max=%lluns\n",
               (long)ts.tv_sec, ts.tv_nsec / 1000000, (unsigned long long)s.alarm_events, s.last_alarm_id,
               (unsigned long long)s.frames, (unsigned long long)s.dropped, (unsigned long long)s.det_max_ns);
        fflush(stdout);
    }
}
