/*
 * ds150e_lib.c — DS150E (Autocom CDP+) command library implementation
 */

#include "ds150e_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ctype.h>

#define BAUD B115200
#define BUF_SZ 4096
#define DEFAULT_TIMEOUT_MS 2000

static int fd = -1;

/* ── Serial port ─────────────────────────────────────────── */

int ds_open(const char *dev)
{
	fd = open(dev, O_RDWR | O_NOCTTY);
	if (fd < 0) {
		fprintf(stderr, "ds_open %s: %s\n", dev, strerror(errno));
		return -1;
	}

	struct termios tio;
	tcgetattr(fd, &tio);
	cfmakeraw(&tio);
	cfsetispeed(&tio, BAUD);
	cfsetospeed(&tio, BAUD);

	tio.c_cflag &= ~(CSIZE | PARENB | CSTOPB);
	tio.c_cflag |= CS8 | CLOCAL | CREAD;
	tio.c_cflag &= ~CRTSCTS;
	tio.c_iflag &= ~(IXON | IXOFF | IXANY);

	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = 2; /* 200ms per read() — timeout_ms controls total */

	tcsetattr(fd, TCSANOW, &tio);
	tcflush(fd, TCIOFLUSH);

	/* DTR + RTS high */
	int bits;
	ioctl(fd, TIOCMGET, &bits);
	bits |= TIOCM_DTR | TIOCM_RTS;
	ioctl(fd, TIOCMSET, &bits);

	usleep(100000);
	return fd;
}

void ds_close(void)
{
	if (fd >= 0) {
		close(fd);
		fd = -1;
	}
}

int ds_cmd(const char *cmd, char *buf, int bufsz, int timeout_ms)
{
	if (fd < 0) return -1;
	if (!timeout_ms) timeout_ms = DEFAULT_TIMEOUT_MS;

	char txbuf[512];
	snprintf(txbuf, sizeof(txbuf), "%s\r", cmd);
	tcflush(fd, TCIOFLUSH);

	fprintf(stderr, "  >> %s\n", cmd);

	int n = write(fd, txbuf, strlen(txbuf));
	if (n < 0) return -1;

	usleep(50000); /* 50ms initial wait */

	memset(buf, 0, bufsz);
	int total = 0;
	int retries = timeout_ms / 100;
	if (retries < 5) retries = 5;

	while (total < bufsz - 1 && retries-- > 0) {
		n = read(fd, buf + total, bufsz - 1 - total);
		if (n > 0) {
			total += n;
			if (memchr(buf, '\r', total))
				break;
		} else if (n == 0) {
			usleep(100000);
		} else {
			break;
		}
	}

	while (total > 0 && (buf[total-1] == '\r' || buf[total-1] == '\n'))
		buf[--total] = 0;

	fprintf(stderr, "  << n=%d '%s'\n", total, total > 0 ? buf : "(empty)");
	return total;
}

/* ── Device info ─────────────────────────────────────────── */

int ds_get_name(char *buf, int bufsz)
{
	return ds_cmd("*20A", buf, bufsz, 2000);
}

int ds_get_serial(char *buf, int bufsz)
{
	return ds_cmd("*200", buf, bufsz, 2000);
}

int ds_get_firmware(char *buf, int bufsz)
{
	return ds_cmd("*201", buf, bufsz, 2000);
}

int ds_get_voltage_mv(void)
{
	char buf[256];
	int n = ds_cmd("*203", buf, sizeof(buf), 2000);
	if (n <= 0) return -1;
	return ds_parse_voltage(buf);
}

/* ── Init / handshake ────────────────────────────────────── */

int ds_init_60b(char *buf, int bufsz)
{
	return ds_cmd("*60b", buf, bufsz, 3000);
}

int ds_init_60bc(char *buf, int bufsz)
{
	return ds_cmd("*60bc", buf, bufsz, 3000);
}

/* ── Self-test ───────────────────────────────────────────── */

int ds_selftest(char test_id)
{
	char cmd[16], buf[256];
	snprintf(cmd, sizeof(cmd), "*918%c", test_id);
	int n = ds_cmd(cmd, buf, sizeof(buf), 5000);
	if (n <= 0) return -1;

	char ok_str[32], fail_str[32];
	snprintf(ok_str, sizeof(ok_str), "*918%c_OK", test_id);
	snprintf(fail_str, sizeof(fail_str), "*918%c_FAIL", test_id);

	if (strstr(buf, ok_str)) return 1;
	if (strstr(buf, fail_str)) return 0;
	return -1;
}

int ds_selftest_all(void)
{
	int failures = 0;
	printf("=== DS150E Self-Test ===\n");
	for (char t = 'A'; t <= 'F'; t++) {
		int r = ds_selftest(t);
		const char *status = r == 1 ? "OK" : r == 0 ? "FAIL" : "NO RESPONSE";
		printf("  918%c: %s\n", t, status);
		if (r != 1) failures++;
	}
	return failures;
}

/* ── CAN bus configuration ───────────────────────────────── */

int ds_can_config(int bus, int rate, const char *txid, const char *rxid,
                  char *buf, int bufsz)
{
	char cmd[256];
	/* full *668 sequence from Windows capture: trailing _080_1A_87 required */
	snprintf(cmd, sizeof(cmd), "*668_%d_%d_%s_%s_000_01C_02_3E_02_00_00_00_00_00_080_1A_87",
	         bus, rate, txid, rxid);
	/* *607 closes any active CAN session before reconfiguring */
	char tmp[64];
	ds_cmd("*607", tmp, sizeof(tmp), 2000);

	int n = ds_cmd(cmd, buf, bufsz, 8000);
	if (n <= 0)
		return -1;
	return n;
}

int ds_can_config_obd2(char *buf, int bufsz)
{
	return ds_can_config(0, 500, "7E0", "7E8", buf, bufsz);
}

/* ── Periodic message slot ───────────────────────────────── */

int ds_periodic_msg(const char *slot, const char *canid,
                    const uint8_t data[8], char *buf, int bufsz)
{
	char cmd[256];
	snprintf(cmd, sizeof(cmd),
	         "*606%s_%s_%02X_%02X_%02X_%02X_%02X_%02X_%02X_%02X",
	         slot, canid,
	         data[0], data[1], data[2], data[3],
	         data[4], data[5], data[6], data[7]);
	return ds_cmd(cmd, buf, bufsz, 3000);
}

int ds_tester_present(char *buf, int bufsz)
{
	uint8_t tp[] = {0x02, 0x3E, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00};
	return ds_periodic_msg("B001", "7DF", tp, buf, bufsz);
}

/* ── ECU communication ───────────────────────────────────── */

int ds_ecu_init(char *buf, int bufsz)
{
	return ds_cmd("*608_18_00_FF_00", buf, bufsz, 5000);
}

int ds_session_physical(char *buf, int bufsz)
{
	/* *609_0_01C_01C_10_92 — DiagnosticSessionControl physical (01C = engine ECU) */
	return ds_cmd("*609_0_01C_01C_10_92", buf, bufsz, 3000);
}

int ds_session_broadcast(char *buf, int bufsz)
{
	/* *609_0_7DF_7DF_10_92 — DiagnosticSessionControl broadcast */
	return ds_cmd("*609_0_7DF_7DF_10_92", buf, bufsz, 3000);
}

int ds_ecu_read_param(uint8_t idx, uint8_t *data, int datasz)
{
	char cmd[64], buf[512];
	snprintf(cmd, sizeof(cmd), "*608_21_%02X", idx);
	int n = ds_cmd(cmd, buf, sizeof(buf), 3000);
	if (n <= 0) return -1;

	uint8_t parsed_idx;
	return ds_parse_param(buf, &parsed_idx, data, datasz);
}

int ds_ecu_write(uint8_t idx, char *buf, int bufsz)
{
	char cmd[64];
	snprintf(cmd, sizeof(cmd), "*608_10_%02X", idx);
	return ds_cmd(cmd, buf, bufsz, 3000);
}

/* ── Raw CAN frame ───────────────────────────────────────── */

int ds_can_send_raw(int bus, const char *txid, const char *rxid,
                    int dlc, const uint8_t *data,
                    char *buf, int bufsz)
{
	char cmd[256];
	int pos = snprintf(cmd, sizeof(cmd), "*609_%d_%s_%s_%d",
	                   bus, txid, rxid, dlc);
	for (int i = 0; i < dlc && pos < (int)sizeof(cmd) - 4; i++)
		pos += snprintf(cmd + pos, sizeof(cmd) - pos, "_%02X", data[i]);

	return ds_cmd(cmd, buf, bufsz, 3000);
}

/* ── Bluetooth ───────────────────────────────────────────── */

int ds_bt_enable(void)
{
	char buf[256];
	/* Command not confirmed yet — placeholder based on firmware strings */
	int n = ds_cmd("*F1F5", buf, sizeof(buf), 3000);
	if (n > 0 && strstr(buf, "enabled")) return 1;
	return 0;
}

int ds_bt_disable(void)
{
	char buf[256];
	int n = ds_cmd("*F0F5", buf, sizeof(buf), 3000);
	if (n > 0 && strstr(buf, "disabled")) return 1;
	return 0;
}

/* ── Utility ─────────────────────────────────────────────── */

int ds_raw(const char *cmd, char *buf, int bufsz)
{
	return ds_cmd(cmd, buf, bufsz, DEFAULT_TIMEOUT_MS);
}

int ds_parse_voltage(const char *resp)
{
	/* Response: "*0 1362" → 13620 mV, or "*0 NNNN" */
	if (!resp || resp[0] != '*') return -1;

	const char *space = strchr(resp, ' ');
	if (!space) return -1;

	int val = atoi(space + 1);
	if (val > 0)
		return val * 10; /* NNNN is voltage/10, so *10 → mV */
	return -1;
}

int ds_parse_param(const char *resp, uint8_t *idx, uint8_t *data, int datasz)
{
	/* Response: "*97 XX <hex bytes>" or similar */
	if (!resp || resp[0] != '*') return -1;

	/* Skip response code (e.g. "*97 ") */
	const char *p = resp + 1;
	while (*p && *p != ' ') p++;
	if (!*p) return -1;
	p++;

	/* Parse index */
	unsigned int i;
	if (sscanf(p, "%02X", &i) != 1) return -1;
	*idx = (uint8_t)i;
	p += 2;

	/* Parse data bytes */
	int count = 0;
	while (*p && count < datasz) {
		while (*p == ' ') p++;
		if (!*p) break;

		unsigned int b;
		if (sscanf(p, "%02X", &b) != 1) break;
		data[count++] = (uint8_t)b;
		p += 2;
	}

	return count;
}

void ds_hexdump(const uint8_t *data, int len)
{
	for (int i = 0; i < len; i++) {
		printf("%02X ", data[i]);
		if ((i + 1) % 16 == 0) printf("\n");
	}
	if (len % 16) printf("\n");
}

/* ── Logging / monitoring ────────────────────────────────── */

#include <signal.h>
#include <time.h>

static volatile int monitor_running = 1;

static void monitor_sigint(int sig)
{
	(void)sig;
	monitor_running = 0;
}

int ds_monitor(const uint8_t *params, int count,
               const char *csv_path, int interval_ms)
{
	FILE *out = stdout;
	if (csv_path) {
		out = fopen(csv_path, "w");
		if (!out) {
			fprintf(stderr, "Cannot open %s: %s\n", csv_path, strerror(errno));
			return -1;
		}
		/* unbuffered for crash safety */
		setvbuf(out, NULL, _IONBF, 0);
	}

	/* CSV header */
	fprintf(out, "timestamp");
	for (int i = 0; i < count; i++)
		fprintf(out, ",P%02X", params[i]);
	fprintf(out, "\n");

	signal(SIGINT, monitor_sigint);
	monitor_running = 1;

	int cycle = 0;
	while (monitor_running) {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		fprintf(out, "%ld.%03ld", (long)ts.tv_sec, ts.tv_nsec / 1000000);

		for (int i = 0; i < count && monitor_running; i++) {
			uint8_t data[64];
			int dlen = ds_ecu_read_param(params[i], data, sizeof(data));
			if (dlen > 0) {
				fprintf(out, ",");
				for (int j = 0; j < dlen; j++)
					fprintf(out, "%02X", data[j]);
			} else {
				fprintf(out, ",ERR");
			}
		}
		fprintf(out, "\n");
		cycle++;

		if (cycle % 10 == 0 && csv_path)
			fprintf(stderr, "\r%d cycles, logging to %s", cycle, csv_path);

		if (interval_ms > 0)
			usleep(interval_ms * 1000);
	}

	if (csv_path) {
		fprintf(stderr, "\n%d cycles saved to %s\n", cycle, csv_path);
		fclose(out);
	}
	return 0;
}
