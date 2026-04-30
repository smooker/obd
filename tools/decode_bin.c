/*
 * decode_bin.c — Decode usbmon binary captures от sniff.c
 *
 * Чете .bin файл (64-byte mon_hdr + payload records), извлича BULK
 * трансфери до/от DS150E, показва ASCII протокол с timestamps.
 *
 * Build:  gcc -Wall -O2 -o decode_bin decode_bin.c
 * Usage:  decode_bin <file.bin> [options]
 *         decode_bin <file.bin> -t          text transcript (default)
 *         decode_bin <file.bin> -p          parsed DS150E commands only
 *         decode_bin <file.bin> -s <str>    grep: показва само редове с <str>
 *         decode_bin <file.bin> -d          debug: hex dump на всеки payload
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#define HDR_SIZE 64

struct mon_hdr {
    uint64_t id;
    uint8_t  type;
    uint8_t  xfer_type;
    uint8_t  epnum;
    uint8_t  devnum;
    uint16_t busnum;
    int8_t   flag_setup;
    int8_t   flag_data;
    int64_t  ts_sec;
    int32_t  ts_usec;
    int32_t  status;
    uint32_t length;
    uint32_t len_cap;
    uint8_t  setup[8];
    int32_t  interval;
    int32_t  start_frame;
    uint32_t xfer_flags;
    uint32_t ndesc;
} __attribute__((packed));

static void hexdump(const uint8_t *d, int n)
{
    for (int i = 0; i < n; i++) {
        printf("%02X ", d[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    if (n % 16) printf("\n");
}

static void print_ascii_escaped(const uint8_t *d, int n)
{
    putchar('\'');
    for (int i = 0; i < n; i++) {
        unsigned c = d[i];
        if (c == '\r')       fputs("\\r", stdout);
        else if (c == '\n')  fputs("\\n", stdout);
        else if (c >= 0x20 && c < 0x7f) putchar(c);
        else                 printf("\\x%02x", c);
    }
    putchar('\'');
}

/* Извлича DS150E команди/отговори от payload.
 * Команди: '*...\r' (OUT direction)
 * Отговори: ASCII bytes с '\r' (IN direction)
 * Returns printable string in buf. */
static int parse_ds150e(const uint8_t *data, int dlen, int is_in,
                        char *out, int outsz)
{
    if (dlen <= 0) return 0;

    /* IN bulk от FTDI: 2-байтов modem status prefix */
    if (is_in && dlen >= 2) {
        data += 2;
        dlen -= 2;
    }
    if (dlen <= 0) return 0;

    /* Само printable + CR/LF */
    int has_content = 0;
    for (int i = 0; i < dlen; i++) {
        if (data[i] >= 0x20 || data[i] == '\r' || data[i] == '\n') {
            has_content = 1;
            break;
        }
    }
    if (!has_content) return 0;

    int pos = 0;
    for (int i = 0; i < dlen && pos < outsz - 1; i++) {
        uint8_t c = data[i];
        if (c == '\r' || c == '\n')  { out[pos++] = ' '; }
        else if (c >= 0x20 && c < 0x7f) out[pos++] = c;
        else                         { pos += snprintf(out + pos, outsz - pos, "\\x%02x", c); }
    }
    out[pos] = 0;
    return pos;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s <file.bin> [-t|-p|-d] [-s filter] [-b bus] [-n dev]\n"
        "  -t        text transcript (default)\n"
        "  -p        parsed DS150E protocol commands only\n"
        "  -d        hex dump всеки payload\n"
        "  -s STR    показва само редове съдържащи STR\n"
        "  -b BUS    филтрира по USB bus номер\n"
        "  -n DEV    филтрира по USB device номер\n"
        "  -a        показва всички transfer типове (не само BULK)\n",
        prog);
}

int main(int argc, char **argv)
{
    const char *binfile = NULL;
    const char *search = NULL;
    int mode = 't';   /* t=text, p=parsed, d=debug */
    int filter_bus = -1, filter_dev = -1;
    int all_xfer = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
            case 't': mode = 't'; break;
            case 'p': mode = 'p'; break;
            case 'd': mode = 'd'; break;
            case 'a': all_xfer = 1; break;
            case 's': if (++i < argc) search = argv[i]; break;
            case 'b': if (++i < argc) filter_bus = atoi(argv[i]); break;
            case 'n': if (++i < argc) filter_dev = atoi(argv[i]); break;
            case 'h': usage(argv[0]); return 0;
            default:  fprintf(stderr, "Unknown option -%c\n", argv[i][1]); return 1;
            }
        } else {
            binfile = argv[i];
        }
    }

    if (!binfile) {
        usage(argv[0]);
        return 1;
    }

    FILE *f = fopen(binfile, "rb");
    if (!f) {
        perror(binfile);
        return 1;
    }

    struct mon_hdr hdr;
    uint8_t payload[65536];
    unsigned long pkt = 0, shown = 0;

    while (fread(&hdr, HDR_SIZE, 1, f) == 1) {
        pkt++;

        uint32_t cap = hdr.len_cap;
        if (cap > sizeof(payload)) cap = sizeof(payload);

        size_t pn = 0;
        if (cap > 0)
            pn = fread(payload, 1, cap, f);

        /* Филтри */
        if (!all_xfer && hdr.xfer_type != 3) continue;  /* BULK only */
        if (filter_bus >= 0 && hdr.busnum != (uint16_t)filter_bus) continue;
        if (filter_dev >= 0 && hdr.devnum != (uint8_t)filter_dev) continue;

        int is_in = (hdr.epnum & 0x80) ? 1 : 0;
        long long ts_us = (long long)hdr.ts_sec * 1000000 + hdr.ts_usec;

        if (mode == 't') {
            /* ASCII transcript — като .txt файла от sniff.c */
            const uint8_t *data = payload;
            int dlen = (int)pn;
            if (is_in && dlen >= 2) { data += 2; dlen -= 2; }

            char linebuf[4096];
            int pos = snprintf(linebuf, sizeof(linebuf),
                "%lld.%06lld %s ",
                ts_us / 1000000, ts_us % 1000000,
                is_in ? "       ←" : "→");
            /* append ascii escaped */
            pos += snprintf(linebuf + pos, sizeof(linebuf) - pos, "'");
            for (int i = 0; i < dlen && pos < (int)sizeof(linebuf) - 8; i++) {
                uint8_t c = data[i];
                if (c == '\r')      pos += snprintf(linebuf+pos, sizeof(linebuf)-pos, "\\r");
                else if (c == '\n') pos += snprintf(linebuf+pos, sizeof(linebuf)-pos, "\\n");
                else if (c >= 0x20 && c < 0x7f) linebuf[pos++] = c;
                else                pos += snprintf(linebuf+pos, sizeof(linebuf)-pos, "\\x%02x", c);
            }
            pos += snprintf(linebuf + pos, sizeof(linebuf) - pos,
                "'  (len=%u cap=%u)", hdr.length, hdr.len_cap);

            if (search && !strstr(linebuf, search)) continue;
            puts(linebuf);
            shown++;

        } else if (mode == 'p') {
            /* Parsed DS150E: само значими команди/отговори */
            char parsed[2048];
            int plen = parse_ds150e(payload, (int)pn, is_in, parsed, sizeof(parsed));
            if (plen <= 0) continue;
            if (search && !strstr(parsed, search)) continue;

            printf("%lld.%06lld %s %s\n",
                ts_us / 1000000, ts_us % 1000000,
                is_in ? "←" : "→",
                parsed);
            shown++;

        } else if (mode == 'd') {
            /* Debug: пълен hex dump */
            char info[128];
            snprintf(info, sizeof(info),
                "[%lu] ts=%lld.%06lld type=%c xfer=%d ep=0x%02x dev=%d len=%u cap=%u",
                pkt, ts_us / 1000000, ts_us % 1000000,
                hdr.type, hdr.xfer_type, hdr.epnum, hdr.devnum,
                hdr.length, hdr.len_cap);
            if (search && !strstr(info, search)) continue;
            puts(info);
            if (pn > 0) hexdump(payload, (int)pn);
            shown++;
        }
    }

    fclose(f);
    fprintf(stderr, "# %lu packets read, %lu shown\n", pkt, shown);
    return 0;
}
