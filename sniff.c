/* sniff.c — Autocom CDP+ (0403:d6da) USB sniffer
 *
 * Pure C. No libpcap. Reads /dev/usbmonN binary mon stream directly.
 * Kill-resilient (unbuffered binary writes). Hotplug-aware (-w).
 *
 * Build:  gcc -O2 -Wall -o sniff sniff.c
 * Usage:  sudo ./sniff               capture сега (трябва dongle plugged)
 *         sudo ./sniff -w            wait for hotplug, after capture
 *         sudo ./sniff -o NAME       override output basename
 *
 * Output: <script_dir>/captures/<basename>.bin  (raw mon stream)
 *         <script_dir>/captures/<basename>.txt  (ASCII transcript)
 *
 * Stage 1 obd project — see CLAUDE.md, PLAN_*.md
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <dirent.h>
#include <libgen.h>
#include <limits.h>
#include <sys/stat.h>

#define VID 0x0403
#define PID 0xd6da
#define HDR_SIZE 64
#define POLL_MS 200

/* mon_bin_hdr_v1 layout — 64 bytes, little-endian on x86 */
struct mon_hdr {
    uint64_t id;
    uint8_t  type;        /* 'S'=submit, 'C'=complete, 'E'=error */
    uint8_t  xfer_type;   /* 0=ISO, 1=INT, 2=CTL, 3=BLK */
    uint8_t  epnum;       /* endpoint | (IN ? 0x80 : 0) */
    uint8_t  devnum;      /* device address on bus */
    uint16_t busnum;
    int8_t   flag_setup;
    int8_t   flag_data;
    int64_t  ts_sec;
    int32_t  ts_usec;
    int32_t  status;
    uint32_t length;      /* URB length */
    uint32_t len_cap;     /* captured length (data follows) */
    uint8_t  setup[8];    /* setup or iso descriptor */
    int32_t  interval;
    int32_t  start_frame;
    uint32_t xfer_flags;
    uint32_t ndesc;
} __attribute__((packed));

static volatile sig_atomic_t stop_requested = 0;
static int bin_fd = -1;
static FILE *txt_fp = NULL;
static unsigned long pkt_count = 0, bulk_count = 0;

static void on_sigint(int sig) {
    (void)sig;
    stop_requested = 1;
}

/* Read trimmed line from a file. Returns 0 on success. */
static int read_line(const char *path, char *out, size_t outsz) {
    FILE *f = fopen(path, "r");
    if (!f) return -1;
    if (!fgets(out, outsz, f)) { fclose(f); return -1; }
    fclose(f);
    size_t n = strlen(out);
    while (n && (out[n-1] == '\n' || out[n-1] == '\r' || out[n-1] == ' '))
        out[--n] = 0;
    return 0;
}

/* Scan /sys/bus/usb/devices for VID/PID. Skip interface entries (contain ':').
 * On match, fill *out_bus, *out_dev. Returns 1 if found, 0 otherwise. */
static int find_dongle(int *out_bus, int *out_dev) {
    DIR *d = opendir("/sys/bus/usb/devices");
    if (!d) return 0;
    struct dirent *de;
    int found = 0;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.') continue;
        if (strchr(de->d_name, ':')) continue;  /* interface */
        char path[PATH_MAX], buf[32];
        snprintf(path, sizeof path, "/sys/bus/usb/devices/%s/idVendor", de->d_name);
        if (read_line(path, buf, sizeof buf) < 0) continue;
        unsigned vid = (unsigned)strtoul(buf, NULL, 16);
        if (vid != VID) continue;
        snprintf(path, sizeof path, "/sys/bus/usb/devices/%s/idProduct", de->d_name);
        if (read_line(path, buf, sizeof buf) < 0) continue;
        unsigned pid = (unsigned)strtoul(buf, NULL, 16);
        if (pid != PID) continue;
        /* match */
        snprintf(path, sizeof path, "/sys/bus/usb/devices/%s/busnum", de->d_name);
        if (read_line(path, buf, sizeof buf) < 0) continue;
        *out_bus = atoi(buf);
        snprintf(path, sizeof path, "/sys/bus/usb/devices/%s/devnum", de->d_name);
        if (read_line(path, buf, sizeof buf) < 0) continue;
        *out_dev = atoi(buf);
        found = 1;
        break;
    }
    closedir(d);
    return found;
}

/* Resolve script's directory via /proc/self/exe. */
static int resolve_script_dir(char *out, size_t outsz) {
    char self[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", self, sizeof(self) - 1);
    if (n < 0) return -1;
    self[n] = 0;
    char *d = dirname(self);
    snprintf(out, outsz, "%s", d);
    return 0;
}

/* Make sure captures/ dir exists. */
static int ensure_captures_dir(const char *base) {
    char path[PATH_MAX + 16];
    int n = snprintf(path, sizeof path, "%s/captures", base);
    if (n < 0 || (size_t)n >= sizeof path) {
        fprintf(stderr, "captures path too long\n");
        return -1;
    }
    if (mkdir(path, 0775) < 0 && errno != EEXIST) {
        fprintf(stderr, "mkdir %s: %s\n", path, strerror(errno));
        return -1;
    }
    return 0;
}

/* Print payload as ASCII with \r/\n/\xNN escapes. */
static void print_ascii(FILE *fp, const uint8_t *p, int n) {
    fputc('\'', fp);
    for (int i = 0; i < n; i++) {
        unsigned c = p[i];
        if (c == '\r') fputs("\\r", fp);
        else if (c == '\n') fputs("\\n", fp);
        else if (c >= 0x20 && c < 0x7f) fputc(c, fp);
        else fprintf(fp, "\\x%02x", c);
    }
    fputc('\'', fp);
}

static void cleanup(void) {
    if (txt_fp) {
        fprintf(txt_fp, "\n# total: %lu packets, %lu BULK\n", pkt_count, bulk_count);
        fclose(txt_fp);
        txt_fp = NULL;
    }
    if (bin_fd >= 0) {
        close(bin_fd);
        bin_fd = -1;
    }
}

int main(int argc, char **argv) {
    int wait_mode = 0;
    const char *override_name = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "wo:h")) != -1) {
        switch (opt) {
            case 'w': wait_mode = 1; break;
            case 'o': override_name = optarg; break;
            case 'h':
            default:
                fprintf(stderr, "usage: %s [-w] [-o name]\n"
                                "  -w       wait for hotplug of %04x:%04x\n"
                                "  -o name  output basename (default: timestamp)\n",
                                argv[0], VID, PID);
                return opt == 'h' ? 0 : 1;
        }
    }

    /* find dongle (or wait for it) */
    int bus = 0, dev = 0;
    if (wait_mode) {
        fprintf(stderr, "armed, waiting for %04x:%04x...\n", VID, PID);
        while (!find_dongle(&bus, &dev)) {
            if (stop_requested) return 0;
            usleep(POLL_MS * 1000);
        }
        fprintf(stderr, "hotplug: bus=%d device=%d\n", bus, dev);
    } else {
        if (!find_dongle(&bus, &dev)) {
            fprintf(stderr, "ERR: %04x:%04x not found. Use -w to wait for hotplug.\n", VID, PID);
            return 1;
        }
        fprintf(stderr, "found: bus=%d device=%d\n", bus, dev);
    }

    /* open usbmon */
    char usbmon_path[64];
    snprintf(usbmon_path, sizeof usbmon_path, "/dev/usbmon%d", bus);
    int mon_fd = open(usbmon_path, O_RDONLY);
    if (mon_fd < 0) {
        fprintf(stderr, "open %s: %s\n", usbmon_path, strerror(errno));
        return 1;
    }
    fprintf(stderr, "opened %s\n", usbmon_path);

    /* output paths */
    char script_dir[PATH_MAX];
    if (resolve_script_dir(script_dir, sizeof script_dir) < 0) {
        fprintf(stderr, "ERR: cannot resolve /proc/self/exe\n");
        return 1;
    }
    if (ensure_captures_dir(script_dir) < 0) return 1;

    char ts[32];
    if (override_name) {
        snprintf(ts, sizeof ts, "%s", override_name);
    } else {
        time_t now = time(NULL);
        struct tm tm_buf;
        localtime_r(&now, &tm_buf);
        strftime(ts, sizeof ts, "%Y%m%d_%H%M%S", &tm_buf);
    }
    char bin_path[PATH_MAX + 64], txt_path[PATH_MAX + 64];
    snprintf(bin_path, sizeof bin_path, "%s/captures/%s.bin", script_dir, ts);
    snprintf(txt_path, sizeof txt_path, "%s/captures/%s.txt", script_dir, ts);

    bin_fd = open(bin_path, O_WRONLY | O_CREAT | O_TRUNC, 0664);
    if (bin_fd < 0) {
        fprintf(stderr, "open %s: %s\n", bin_path, strerror(errno));
        return 1;
    }
    txt_fp = fopen(txt_path, "w");
    if (!txt_fp) {
        fprintf(stderr, "fopen %s: %s\n", txt_path, strerror(errno));
        return 1;
    }
    setvbuf(txt_fp, NULL, _IOLBF, 0);  /* line-buffered */

    fprintf(stderr, "bin: %s\n", bin_path);
    fprintf(stderr, "txt: %s\n", txt_path);
    fprintf(stderr, "filtering bus=%d device=%d, BULK only. Ctrl-C to stop.\n", bus, dev);

    /* signal handlers */
    struct sigaction sa = {0};
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /* capture loop */
    struct mon_hdr hdr;
    uint8_t payload[65536];

    while (!stop_requested) {
        ssize_t n = read(mon_fd, &hdr, HDR_SIZE);
        if (n < 0) {
            if (errno == EINTR) continue;
            fprintf(stderr, "read header: %s\n", strerror(errno));
            break;
        }
        if (n == 0) break;
        if (n != HDR_SIZE) {
            fprintf(stderr, "short header: %zd\n", n);
            continue;
        }

        uint32_t cap = hdr.len_cap;
        if (cap > sizeof payload) cap = sizeof payload;
        ssize_t pn = 0;
        if (cap > 0) {
            pn = read(mon_fd, payload, cap);
            if (pn < 0) {
                if (errno == EINTR) continue;
                fprintf(stderr, "read payload: %s\n", strerror(errno));
                break;
            }
        }

        /* Write hdr + payload to bin file IMMEDIATELY (no userspace buffer).
         * write(2) goes through kernel page cache but on fsync/sync
         * survives userspace kill. We accept page cache risk for speed. */
        ssize_t wn = write(bin_fd, &hdr, HDR_SIZE);
        if (wn != HDR_SIZE) {
            fprintf(stderr, "write hdr: %s\n", strerror(errno));
            break;
        }
        if (pn > 0) {
            wn = write(bin_fd, payload, pn);
            if (wn != pn) {
                fprintf(stderr, "write payload: %s\n", strerror(errno));
                break;
            }
        }

        pkt_count++;

        /* Filter for transcript */
        if (hdr.busnum != bus) continue;
        if (hdr.devnum != dev) continue;
        if (hdr.xfer_type != 3) continue;  /* BULK only */

        bulk_count++;

        int is_in = (hdr.epnum & 0x80) ? 1 : 0;
        const uint8_t *data = payload;
        int dlen = (int)pn;

        /* FTDI: skip 2 byte modem-status prefix on IN bulk */
        if (is_in && dlen >= 2) {
            data += 2;
            dlen -= 2;
        }

        long long ts_us = (long long)hdr.ts_sec * 1000000 + hdr.ts_usec;
        const char *arrow = is_in ? "←" : "→";
        const char *pad = is_in ? "       " : "";
        fprintf(txt_fp, "%lld.%06lld %s%s ", ts_us / 1000000, ts_us % 1000000, pad, arrow);
        print_ascii(txt_fp, data, dlen);
        fprintf(txt_fp, "  (len=%u cap=%u)\n", hdr.length, hdr.len_cap);
    }

    fprintf(stderr, "\nstopped. %lu packets, %lu BULK\n", pkt_count, bulk_count);
    cleanup();
    close(mon_fd);
    return 0;
}
