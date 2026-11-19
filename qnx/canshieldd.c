/* canshieldd — CANSHIELD for QNX 8.0: deterministic CAN intrusion detection + optional CAN firewall.
 *
 *  - RX thread (SCHED_FIFO) blocks on a QNX CAN device (devctl CAN_DEVCTL_RX_FRAME_RAW_BLOCK),
 *    classifies every frame with the hardened logic-gate model (fixed gate count, no allocation),
 *    measures per-frame detection time with ClockCycles(), and optionally forwards clean frames to a
 *    second bus (gateway / firewall mode) while dropping flagged ones.
 *  - Resource manager at /dev/canshield:
 *        cat /dev/canshield                     -> human-readable status + latency histogram
 *        devctl(CS_DEVCTL_GET_STATS)            -> binary stats struct
 *        devctl(CS_DEVCTL_NOTIFY, sigevent)     -> client is notified (MsgDeliverEvent) on every alarm
 *  - Alarms are logged to slog2.
 *
 *  - Optional tap (-t FIFO): every accepted frame is also written, non-blocking, as a raw struct can_msg
 *    to a FIFO, so a local consumer (e.g. the Kuksa CAN feeder) only ever sees frames that passed the firewall.
 *
 *  - Modes: enforce (default, firewall) or monitor (-n; detect and alarm, forward everything). Switch at run
 *    time with  echo monitor > /dev/canshield  /  echo enforce > /dev/canshield.
 *
 * usage: canshieldd -p profile.bin [-i /dev/can0/rx0] [-o /dev/can1/tx0] [-t /tmp/cs_accepted] [-n] [-P priority] [-s slow] [-v]
 *   -s N: time dilation when the input is replayed N times slower (emulator demos); detection logic unchanged
 * SPDX-License-Identifier: Apache-2.0 */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <devctl.h>
#include <sys/can_dcmd.h>
#include <sys/dispatch.h>
#include <sys/iofunc.h>
#include <sys/neutrino.h>
#include <sys/slog2.h>
#include <sys/syspage.h>
#include <sys/procmgr.h>
#include <sys/stat.h>
#include "../core/cs_detector.h"
#include "canshield_api.h"

static cs_profile_t   g_prof;
static cs_detector_t  g_det;
static cs_stats_t     g_st;
static pthread_mutex_t g_mx = PTHREAD_MUTEX_INITIALIZER;
static uint64_t       g_cps;               /* ClockCycles per second */
static int            g_verbose;
static slog2_buffer_t g_log;
static uint64_t       g_slow = 1;          /* time dilation for slow emulators: replay N x slower, clock / N */
static struct { int rcvid; struct sigevent ev; int armed; } g_notify[CS_MAX_NOTIFY];
static char           g_last_alarm[160];
static uint8_t        g_id_on[CS_NIDS];   /* per-id alarm state (event = rising edge) */
static uint8_t        g_bus_on;           /* bus alarm state */

/* latency histogram buckets (ns): <250, <500, <1k, <2k, <5k, <10k, <20k, <50k, <100k, >=100k */
static const uint32_t BUCKET_NS[CS_HIST_BUCKETS - 1] = { 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000 };

static int bucket_of(uint64_t ns) { int b = 0; while (b < CS_HIST_BUCKETS - 1 && ns >= BUCKET_NS[b]) b++; return b; }

static void notify_clients(void)
{
    for (int i = 0; i < CS_MAX_NOTIFY; i++)
        if (g_notify[i].armed && MsgDeliverEvent(g_notify[i].rcvid, &g_notify[i].ev) == -1)
            g_notify[i].armed = 0;                       /* client went away */
}

typedef struct { const char *in, *out, *tap; int prio; int fin, fout, ftap; } rx_args_t;
static uint64_t       g_tap_sent, g_tap_dropped;   /* accepted-frame tap (FIFO) for local consumers, e.g. a Kuksa feeder */
static volatile int   g_enforce = 1;               /* 1 = firewall (drop flagged frames), 0 = monitor (detect only);
                                                      switch at run time: echo monitor|enforce > /dev/canshield */

static void *rx_thread(void *arg)
{
    rx_args_t *a = arg;                              /* devices were opened and priority set before privileges were dropped */
    int fin = a->fin, fout = a->fout, ftap = a->ftap;
    struct can_msg m; cs_frame_t f; cs_verdict_t v;
    for (;;) {
        memset(&m, 0, sizeof m);
        int r = devctl(fin, CAN_DEVCTL_RX_FRAME_RAW_BLOCK, &m, sizeof m, NULL);
        if (r != EOK) { if (r == EINTR) continue; fprintf(stderr, "rx devctl: %s\n", strerror(r)); sleep(1); continue; }
        uint64_t c0 = ClockCycles();
        f.id = m.ext.is_extended_mid ? (m.mid & 0x1FFFFFFF) : (m.mid > 0x7FF ? (m.mid >> 18) & 0x7FF : m.mid);
        f.dlc = m.len > 8 ? 8 : m.len;
        memset(f.data, 0, 8); memcpy(f.data, m.dat, f.dlc);
        f.t_us = c0 * 1000000ull / g_cps / g_slow;
        cs_detect(&g_prof, &g_det, &f, &v);
        uint64_t c1 = ClockCycles();
        int drop = (v.flagged || v.id_alarm || v.bus_alarm) && g_enforce;   /* monitor mode: detect + alarm only */
        if (fout >= 0 && !drop) devctl(fout, CAN_DEVCTL_TX_FRAME_RAW, &m, sizeof m, NULL);
        uint64_t c2 = ClockCycles();
        /* tap: non-blocking, atomic (88 B < PIPE_BUF) write of each accepted frame; a slow reader never stalls RX */
        if (ftap >= 0 && !drop) { if (write(ftap, &m, sizeof m) == (ssize_t)sizeof m) g_tap_sent++; else g_tap_dropped++; }
        uint64_t det_ns = (c1 - c0) * 1000000000ull / g_cps, gw_ns = (c2 - c0) * 1000000000ull / g_cps;

        pthread_mutex_lock(&g_mx);
        g_st.frames++; g_st.flagged += v.flagged;
        if (drop && fout >= 0) g_st.dropped++;
        if (fout >= 0 && !drop) g_st.forwarded++;
        g_st.det_hist[bucket_of(det_ns)]++; g_st.gw_hist[bucket_of(gw_ns)]++;
        if (det_ns > g_st.det_max_ns) g_st.det_max_ns = det_ns;
        if (gw_ns > g_st.gw_max_ns) g_st.gw_max_ns = gw_ns;
        g_st.det_sum_ns += det_ns;
        uint32_t aid = f.id < CS_NIDS ? f.id : CS_NIDS - 1;
        int new_alarm = (v.id_alarm && !g_id_on[aid]) || (v.bus_alarm && !g_bus_on);
        g_id_on[aid] = v.id_alarm; g_bus_on = v.bus_alarm;
        if (v.id_alarm || v.bus_alarm) { g_st.alarm_frames++; g_st.last_alarm_id = f.id; g_st.last_alarm_frame = g_st.frames; }
        if (new_alarm) {
            g_st.alarm_events++;
            snprintf(g_last_alarm, sizeof g_last_alarm, "id=0x%03X %s votes=%d/16 features=0x%09llX det=%lluns",
                     (unsigned)f.id, v.bus_alarm ? "BUS(unknown-id flood)" : "ID", v.votes, (unsigned long long)v.features,
                     (unsigned long long)det_ns);
            notify_clients();
        }
        pthread_mutex_unlock(&g_mx);
        if (new_alarm) {
            slog2f(g_log, 0, SLOG2_WARNING, "canshieldd ALARM %s", g_last_alarm);
            if (g_verbose) printf("ALARM %s\n", g_last_alarm);
        }
    }
    return NULL;
}

/* ---------- resource manager /dev/canshield ---------- */
static resmgr_connect_funcs_t cfuncs; static resmgr_io_funcs_t iofuncs; static iofunc_attr_t attr;

static int render(char *buf, size_t n)
{
    cs_stats_t s; pthread_mutex_lock(&g_mx); s = g_st; char la[160]; strcpy(la, g_last_alarm); pthread_mutex_unlock(&g_mx);
    int k = snprintf(buf, n,
        "CANSHIELD (QNX) — deterministic CAN intrusion detection\n"
        "model: hardened logic-gate network, threshold %d/16, alarm: >=%d of last 8 per id | bus: >=%d unknown-id of last 64\n"
        "frames %llu | flagged %llu | alarm frames %llu | alarm events %llu | forwarded %llu | dropped %llu\n"
        "detect time: mean %llu ns, max %llu ns | detect+forward max %llu ns\n"
        "last alarm: %s\nhistogram (detect / detect+forward):\n",
        cs_model_threshold, CS_ALARM_K, CS_BUS_K,
        (unsigned long long)s.frames, (unsigned long long)s.flagged, (unsigned long long)s.alarm_frames,
        (unsigned long long)s.alarm_events, (unsigned long long)s.forwarded, (unsigned long long)s.dropped,
        (unsigned long long)(s.frames ? s.det_sum_ns / s.frames : 0), (unsigned long long)s.det_max_ns,
        (unsigned long long)s.gw_max_ns, la[0] ? la : "none");
    static const char *lbl[CS_HIST_BUCKETS] = { "<250ns", "<500ns", "<1us", "<2us", "<5us", "<10us", "<20us", "<50us", "<100us", ">=100us" };
    for (int b = 0; b < CS_HIST_BUCKETS && k < (int)n; b++)
        k += snprintf(buf + k, n - k, "  %-8s %10llu %10llu\n", lbl[b], (unsigned long long)s.det_hist[b], (unsigned long long)s.gw_hist[b]);
    if (k < (int)n)
        k += snprintf(buf + k, n - k, "tap: %llu accepted frames delivered, %llu skipped (reader slow or absent)\n"
                      "mode: %s\n", (unsigned long long)g_tap_sent, (unsigned long long)g_tap_dropped,
                      g_enforce ? "enforce (firewall)" : "monitor (detect only)");
    return k;
}

/* write "monitor" or "enforce" to switch modes at run time (no restart: the RX thread keeps its device) */
static int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *vocb)
{
    iofunc_ocb_t *ocb = vocb;
    int st = iofunc_write_verify(ctp, msg, ocb, NULL); if (st != EOK) return st;
    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) return ENOSYS;
    size_t n = _IO_WRITE_GET_NBYTES(msg); char b[16] = {0};
    resmgr_msgread(ctp, b, n < sizeof b - 1 ? n : sizeof b - 1, sizeof msg->i);
    if (!strncmp(b, "monitor", 7)) g_enforce = 0;
    else if (!strncmp(b, "enforce", 7)) g_enforce = 1;
    else return EINVAL;
    if (g_log) slog2f(g_log, 0, SLOG2_INFO, "canshieldd: mode -> %s", g_enforce ? "enforce" : "monitor");
    _IO_SET_WRITE_NBYTES(ctp, n);
    return EOK;
}

static int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *vocb)
{
    iofunc_ocb_t *ocb = vocb;
    int st = iofunc_read_verify(ctp, msg, ocb, NULL); if (st != EOK) return st;
    static char buf[2048]; int len = render(buf, sizeof buf);
    off_t off = ocb->offset; if (off >= len) { _IO_SET_READ_NBYTES(ctp, 0); return _RESMGR_NPARTS(0); }
    int nb = (int)((size_t)(len - off) < (size_t)msg->i.nbytes ? (size_t)(len - off) : msg->i.nbytes);
    ocb->offset += nb; _IO_SET_READ_NBYTES(ctp, nb);
    SETIOV(ctp->iov, buf + off, nb); return _RESMGR_NPARTS(1);
}

static int io_devctl(resmgr_context_t *ctp, io_devctl_t *msg, RESMGR_OCB_T *vocb)
{
    iofunc_ocb_t *ocb = vocb;
    int st = iofunc_devctl_default(ctp, msg, ocb); if (st != _RESMGR_DEFAULT) return st;
    void *data = _DEVCTL_DATA(msg->i);
    switch (msg->i.dcmd) {
    case CS_DEVCTL_GET_STATS:
        pthread_mutex_lock(&g_mx); memcpy(data, &g_st, sizeof g_st); pthread_mutex_unlock(&g_mx);
        memset(&msg->o, 0, sizeof msg->o); msg->o.nbytes = sizeof g_st;
        return _RESMGR_PTR(ctp, &msg->o, sizeof msg->o + sizeof g_st);
    case CS_DEVCTL_NOTIFY: {
        struct sigevent *ev = data;
        pthread_mutex_lock(&g_mx);
        int slot = -1;
        for (int i = 0; i < CS_MAX_NOTIFY; i++) if (!g_notify[i].armed) { slot = i; break; }
        if (slot >= 0) { g_notify[slot].rcvid = ctp->rcvid; g_notify[slot].ev = *ev; g_notify[slot].armed = 1; }
        pthread_mutex_unlock(&g_mx);
        if (slot < 0) return EAGAIN;
        memset(&msg->o, 0, sizeof msg->o); return _RESMGR_PTR(ctp, &msg->o, sizeof msg->o);
    }
    default: return ENOSYS;
    }
}

int main(int argc, char **argv)
{
    const char *prof = NULL; rx_args_t a = { "/dev/can0/rx0", NULL, NULL, 40, -1, -1, -1 }; int opt;
    while ((opt = getopt(argc, argv, "p:i:o:t:P:s:nv")) != -1) {
        if (opt == 'p') prof = optarg; else if (opt == 'i') a.in = optarg; else if (opt == 'o') a.out = optarg;
        else if (opt == 't') a.tap = optarg; else if (opt == 'n') g_enforce = 0;
        else if (opt == 'P') a.prio = atoi(optarg); else if (opt == 'v') g_verbose = 1;
        else if (opt == 's') g_slow = (uint64_t)atoi(optarg) > 0 ? (uint64_t)atoi(optarg) : 1;
    }
    if (!prof) { fprintf(stderr, "usage: %s -p profile.bin [-i /dev/can0/rx0] [-o /dev/can1/tx0] [-P prio] [-v]\n", argv[0]); return 1; }
    FILE *pf = fopen(prof, "rb");
    if (!pf || fread(&g_prof, sizeof g_prof, 1, pf) != 1 || g_prof.magic != CS_PROFILE_MAGIC) { fprintf(stderr, "bad profile %s\n", prof); return 1; }
    fclose(pf);
    cs_detector_reset(&g_det);
    g_cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;

    slog2_buffer_set_config_t cfg = { .buffer_set_name = "canshieldd", .num_buffers = 1, .verbosity_level = SLOG2_INFO };
    cfg.buffer_config[0].buffer_name = "alarms"; cfg.buffer_config[0].num_pages = 4;
    if (slog2_register(&cfg, &g_log, 0) == -1) g_log = NULL;

    dispatch_t *dpp = dispatch_create();
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &cfuncs, _RESMGR_IO_NFUNCS, &iofuncs);
    iofuncs.read = io_read; iofuncs.devctl = io_devctl; iofuncs.write = io_write;
    iofunc_attr_init(&attr, S_IFCHR | 0644, NULL, NULL);    /* only root may switch the mode */
    resmgr_attr_t rattr; memset(&rattr, 0, sizeof rattr); rattr.nparts_max = 1; rattr.msg_max_size = 4096;
    if (resmgr_attach(dpp, &rattr, CS_PATH, _FTYPE_ANY, 0, &cfuncs, &iofuncs, &attr) == -1) { perror("resmgr_attach"); return 1; }

    /* open devices and fix the RX thread's real-time priority BEFORE dropping privileges */
    a.fin = open(a.in, O_RDWR); a.fout = a.out ? open(a.out, O_RDWR) : -1;
    if (a.fin < 0) { fprintf(stderr, "open %s: %s\n", a.in, strerror(errno)); return 1; }
    if (a.out && a.fout < 0) { fprintf(stderr, "open %s: %s\n", a.out, strerror(errno)); return 1; }
    if (a.tap) {   /* O_RDWR keeps the FIFO open without a reader (no SIGPIPE); O_NONBLOCK: full pipe -> skip, never wait */
        if (mkfifo(a.tap, 0644) == -1 && errno != EEXIST) { fprintf(stderr, "mkfifo %s: %s\n", a.tap, strerror(errno)); return 1; }
        a.ftap = open(a.tap, O_RDWR | O_NONBLOCK);
        if (a.ftap < 0) { fprintf(stderr, "open %s: %s\n", a.tap, strerror(errno)); return 1; }
    }
    pthread_attr_t ta; pthread_attr_init(&ta);
    pthread_attr_setinheritsched(&ta, PTHREAD_EXPLICIT_SCHED); pthread_attr_setschedpolicy(&ta, SCHED_FIFO);
    struct sched_param sp = { .sched_priority = a.prio }; pthread_attr_setschedparam(&ta, &sp);
    pthread_t th;
    if (pthread_create(&th, &ta, rx_thread, &a) != EOK) { perror("pthread_create"); return 1; }

    /* least privilege: deny + lock every QNX process-manager ability for root and non-root */
    int drop = procmgr_ability(0, PROCMGR_ADN_ROOT | PROCMGR_ADN_NONROOT | PROCMGR_AOP_DENY | PROCMGR_AOP_LOCK | PROCMGR_AID_EOL);
    int io_denied = (ThreadCtl(_NTO_TCTL_IO, 0) == -1);             /* self-test: hardware I/O privilege must be refused */
    printf("canshieldd: privileges %s (self-test: I/O privilege %s)\n", drop == EOK ? "dropped and locked" : "DROP FAILED",
           io_denied ? "denied - OK" : "STILL GRANTED");
    if (g_log) slog2f(g_log, 0, SLOG2_INFO, "canshieldd: abilities dropped=%d io_denied=%d", drop == EOK, io_denied);
    printf("canshieldd: %u known ids, input %s%s%s, rx priority %d, time dilation 1/%llu, %s ready (cycles/s %llu)\n",
           g_prof.n_known, a.in, a.out ? ", forwarding clean frames to " : "", a.out ? a.out : "", a.prio, (unsigned long long)g_slow, CS_PATH,
           (unsigned long long)g_cps);
    fflush(stdout);
    dispatch_context_t *ctp = dispatch_context_alloc(dpp);
    for (;;) { if ((ctp = dispatch_block(ctp)) == NULL) { perror("dispatch_block"); return 1; } dispatch_handler(ctp); }
}
