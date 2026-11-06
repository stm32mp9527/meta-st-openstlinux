/*
 * Battery monitor template for STM32MP1 app development.
 *
 * Reads voltage/current/temperature from sysfs-style files:
 *   BAT_VOLTAGE  default /sys/class/power_supply/battery/voltage_now  (uV)
 *   BAT_CURRENT  default /sys/class/power_supply/battery/current_now  (uA)
 *   BAT_TEMP     default /sys/class/power_supply/battery/temp         (0.1 C)
 *   BAT_INTERVAL default 5 seconds
 *   BAT_LOG      optional CSV path
 *
 * This is a template: adapt paths/scaling/SOC algorithm to the customer's
 * fuel gauge or IIO/hwmon device. It does not write kernel drivers.
 */
#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;

static void on_signal(int sig)
{
    (void)sig;
    g_running = 0;
}

static int read_long_file(const char *path, long *out)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return -1;
    long v;
    int ok = (fscanf(f, "%ld", &v) == 1);
    fclose(f);
    if (!ok)
        return -1;
    *out = v;
    return 0;
}

static double clamp(double v, double lo, double hi)
{
    if (v < lo)
        return lo;
    if (v > hi)
        return hi;
    return v;
}

int main(void)
{
    const char *vpath = getenv("BAT_VOLTAGE");
    const char *ipath = getenv("BAT_CURRENT");
    const char *tpath = getenv("BAT_TEMP");
    const char *logpath = getenv("BAT_LOG");
    int interval = 5;

    if (!vpath)
        vpath = "/sys/class/power_supply/battery/voltage_now";
    if (!ipath)
        ipath = "/sys/class/power_supply/battery/current_now";
    if (!tpath)
        tpath = "/sys/class/power_supply/battery/temp";
    if (getenv("BAT_INTERVAL"))
        interval = atoi(getenv("BAT_INTERVAL"));
    if (interval <= 0)
        interval = 5;

    FILE *log = NULL;
    if (logpath && logpath[0]) {
        log = fopen(logpath, "a");
        if (!log) {
            fprintf(stderr, "cannot open BAT_LOG=%s: %s\n", logpath, strerror(errno));
        } else {
            fseek(log, 0, SEEK_END);
            if (ftell(log) == 0)
                fprintf(log, "timestamp,voltage_v,current_a,temp_c,soc_pct\n");
        }
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (g_running) {
        long uv = 0, ua = 0, t10 = 0;
        int ok_v = (read_long_file(vpath, &uv) == 0);
        int ok_i = (read_long_file(ipath, &ua) == 0);
        int ok_t = (read_long_file(tpath, &t10) == 0);

        double voltage = ok_v ? (double)uv / 1000000.0 : NAN;
        double current = ok_i ? (double)ua / 1000000.0 : NAN;
        double temp = ok_t ? (double)t10 / 10.0 : NAN;

        /* Simple linear SOC placeholder: replace with the customer's
         * coulomb-counting / OCV lookup algorithm. */
        double soc = (ok_v && isfinite(voltage))
                         ? clamp((voltage - 3.0) / (4.2 - 3.0) * 100.0, 0.0, 100.0)
                         : NAN;

        time_t now = time(NULL);
        struct tm tmv;
        char ts[32] = "?";
        if (localtime_r(&now, &tmv))
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);

        printf("%s voltage=%.3fV current=%.3fA temp=%.1fC soc=%.1f%%\n",
               ts, voltage, current, temp, soc);
        if (!ok_v)
            fprintf(stderr, "cannot read %s\n", vpath);
        if (!ok_i)
            fprintf(stderr, "cannot read %s\n", ipath);
        if (!ok_t)
            fprintf(stderr, "cannot read %s\n", tpath);

        if (isfinite(temp) && temp > 45.0)
            printf("ALERT: temperature %.1fC > 45C\n", temp);
        if (isfinite(voltage) && voltage < 3.0)
            printf("ALERT: voltage %.3fV < 3.0V\n", voltage);

        if (log) {
            fprintf(log, "%s,%.3f,%.3f,%.1f,%.1f\n",
                    ts, voltage, current, temp, soc);
            fflush(log);
        }

        next.tv_sec += interval;
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL) == EINTR)
            continue;
    }

    if (log)
        fclose(log);
    return 0;
}
