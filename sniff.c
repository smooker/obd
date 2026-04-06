/* sniff.c — Autocom CDP+ (0403:d6da) full-payload USB sniffer
 *
 * Уловя bulk IN/OUT върху usbmonN, парсва pcap_usb_header_mmapped и печата
 * ASCII payload-а на host→dongle / dongle→host. БЕЗ truncation (binary
 * usbmon ring чете пълните данни). По избор пише и pcap файл за Wireshark.
 *
 * Build:  gcc -O2 -Wall -o sniff sniff.c -lpcap
 * Usage:  sudo ./sniff               # стандартен output
 *         sudo ./sniff -w out.pcap   # + pcap файл
 *
 * Намира bus/device чрез lsusb (popen). Ако не е намерен — чака на bus 1.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <pcap.h>
#include <pcap/usb.h>

#define VID 0x0403
#define PID 0xd6da

static pcap_t *handle = NULL;
static pcap_dumper_t *dumper = NULL;
static int target_dev = -1;

static void on_sigint(int sig) {
    (void)sig;
    if (handle) pcap_breakloop(handle);
}

static void hex_ascii(const unsigned char *p, int n) {
    /* печатай само printable и \r → виждаме *cmd\r ясно */
    putchar('\'');
    for (int i = 0; i < n; i++) {
        unsigned c = p[i];
        if (c == '\r') printf("\\r");
        else if (c == '\n') printf("\\n");
        else if (c >= 0x20 && c < 0x7f) putchar(c);
        else printf("\\x%02x", c);
    }
    putchar('\'');
}

static void on_packet(u_char *user, const struct pcap_pkthdr *h,
                      const u_char *bytes) {
    (void)user;
    if (dumper) pcap_dump((u_char *)dumper, h, bytes);

    /* pcap_usb_header_mmapped е първите 64 байта (или 48 за legacy);
     * libpcap използва "_mmapped" за usbmon DLT_USB_LINUX_MMAPPED. */
    const pcap_usb_header_mmapped *uh = (const pcap_usb_header_mmapped *)bytes;
    if (h->caplen < sizeof(*uh)) return;

    /* филтрирай по device */
    if (target_dev > 0 && uh->device_address != target_dev) return;

    /* само bulk transfers (xfer_type 3) */
    if (uh->transfer_type != 3) return;

    int data_off = sizeof(*uh);
    int data_len = h->caplen - data_off;
    if (data_len < 0) data_len = 0;
    const unsigned char *data = bytes + data_off;

    int is_in = (uh->endpoint_number & 0x80) ? 1 : 0;
    char tt = uh->event_type; /* 'S' submit / 'C' complete */

    /* интересува ни:
     *   S Bo (host→dongle)  — командата е в payload-а
     *   C Bi (dongle→host)  — отговорът е в payload-а, скип първите 2 modem-status байта
     */
    if (tt == 'S' && !is_in) {
        printf("%lu.%06u → ", (unsigned long)uh->ts_sec, uh->ts_usec);
        hex_ascii(data, data_len);
        printf("  (len=%u cap=%d)\n", uh->urb_len, data_len);
    } else if (tt == 'C' && is_in && data_len >= 2) {
        printf("%lu.%06u        ← ", (unsigned long)uh->ts_sec, uh->ts_usec);
        hex_ascii(data + 2, data_len - 2);
        printf("  (len=%u cap=%d)\n", uh->urb_len, data_len);
    }
    fflush(stdout);
}

static int find_autocom(int *out_bus, int *out_dev) {
    FILE *fp = popen("lsusb", "r");
    if (!fp) return 0;
    char line[512];
    int bus, dev, vid, pid;
    int found = 0;
    while (fgets(line, sizeof line, fp)) {
        if (sscanf(line, "Bus %d Device %d: ID %x:%x", &bus, &dev, &vid, &pid) == 4) {
            if (vid == VID && pid == PID) {
                *out_bus = bus;
                *out_dev = dev;
                found = 1;
                break;
            }
        }
    }
    pclose(fp);
    return found;
}

int main(int argc, char **argv) {
    const char *outfile = NULL;
    int opt;
    while ((opt = getopt(argc, argv, "w:")) != -1) {
        if (opt == 'w') outfile = optarg;
        else { fprintf(stderr, "usage: %s [-w file.pcap]\n", argv[0]); return 1; }
    }

    int bus = 0;
    if (!find_autocom(&bus, &target_dev)) {
        fprintf(stderr, "Autocom %04x:%04x не е намерен. lsusb го няма.\n"
                        "Ще слушам на usbmon1 без device филтър.\n", VID, PID);
        bus = 1;
        target_dev = -1;
    } else {
        fprintf(stderr, "Намерен: bus=%d device=%d → usbmon%d\n", bus, target_dev, bus);
    }

    char ifname[32];
    snprintf(ifname, sizeof ifname, "usbmon%d", bus);

    char errbuf[PCAP_ERRBUF_SIZE];
    handle = pcap_create(ifname, errbuf);
    if (!handle) { fprintf(stderr, "pcap_create(%s): %s\n", ifname, errbuf); return 1; }
    pcap_set_snaplen(handle, 65535);   /* full payload, no truncation */
    pcap_set_promisc(handle, 1);
    pcap_set_timeout(handle, 100);
    pcap_set_buffer_size(handle, 16*1024*1024);
    int rc = pcap_activate(handle);
    if (rc < 0) { fprintf(stderr, "pcap_activate: %s\n", pcap_geterr(handle)); return 1; }
    if (rc > 0) fprintf(stderr, "warn: %s\n", pcap_geterr(handle));

    if (outfile) {
        dumper = pcap_dump_open(handle, outfile);
        if (!dumper) { fprintf(stderr, "pcap_dump_open: %s\n", pcap_geterr(handle)); return 1; }
        fprintf(stderr, "pcap → %s\n", outfile);
    }

    signal(SIGINT, on_sigint);
    fprintf(stderr, "слушам... Ctrl-C за стоп.\n");
    pcap_loop(handle, -1, on_packet, NULL);

    if (dumper) pcap_dump_close(dumper);
    pcap_close(handle);
    return 0;
}
