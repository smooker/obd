/*
 * ds150e.c — DS150E (Autocom CDP+) diagnostic tool
 * Uses ds150e_lib for all communication.
 *
 * Usage: ds150e [device] <command> [args...]
 */

#include "ds150e_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define DEFAULT_DEV "/dev/ttyUSB0"
#define BUF_SZ 4096

static void usage(const char *prog)
{
	printf("Usage: %s [/dev/ttyUSBx] <command> [args]\n\n", prog);
	printf("Commands:\n");
	printf("  info          Device name, serial, firmware, voltage\n");
	printf("  voltage       Battery voltage\n");
	printf("  init          Init handshake (*60b, *60bc)\n");
	printf("  selftest      Run all self-tests (918A..F)\n");
	printf("  scan          Read all known ECU parameters\n");
	printf("  read <idx>    Read single ECU param (hex index)\n");
	printf("  cansetup      Configure CAN bus (OBD-II standard)\n");
	printf("  ecuinit       ECU init sequence\n");
	printf("  tester        Start Tester Present keepalive\n");
	printf("  bt on|off     Bluetooth enable/disable\n");
	printf("  raw <cmd>     Send raw command (e.g. '*203')\n");
	printf("  monitor [csv] Continuous param logging (Ctrl+C to stop)\n");
	printf("  live [csv]    Live decoded params: temps, RPM, MAF, DPF (Ctrl+C)\n");
	printf("  shell         Interactive mode\n");
}

static void cmd_info(void)
{
	char buf[256];

	printf("=== DS150E Device Info ===\n");

	if (ds_get_name(buf, sizeof(buf)) > 0)
		printf("  Device:   %s\n", buf);
	else
		printf("  Device:   (no response)\n");

	if (ds_get_serial(buf, sizeof(buf)) > 0)
		printf("  Serial:   %s\n", buf);
	else
		printf("  Serial:   (no response)\n");

	if (ds_get_firmware(buf, sizeof(buf)) > 0)
		printf("  Firmware: %s\n", buf);
	else
		printf("  Firmware: (no response)\n");

	int mv = ds_get_voltage_mv();
	if (mv > 0)
		printf("  Voltage:  %d.%02d V\n", mv / 1000, (mv % 1000) / 10);
	else
		printf("  Voltage:  (no response)\n");
}

static void cmd_init(void)
{
	char buf[256];

	printf("=== Init Handshake ===\n");

	int n = ds_init_60b(buf, sizeof(buf));
	printf("  *60b  -> %s\n", n > 0 ? buf : "(no response)");

	n = ds_init_60bc(buf, sizeof(buf));
	printf("  *60bc -> %s\n", n > 0 ? buf : "(no response)");
}

static void cmd_scan(void)
{
	char buf[256];

	printf("=== Init Handshake ===\n");
	int n = ds_init_60b(buf, sizeof(buf));
	printf("  *60b:  %s\n", n > 0 ? buf : "(no response)");
	n = ds_init_60bc(buf, sizeof(buf));
	printf("  *60bc: %s\n", n > 0 ? buf : "(no response)");

	printf("\n=== CAN Setup ===\n");
	n = ds_can_config_obd2(buf, sizeof(buf));
	printf("  CAN config: %s\n", n > 0 ? buf : "(no response)");

	printf("\n=== ECU Init ===\n");
	n = ds_ecu_init(buf, sizeof(buf));
	printf("  ECU init: %s\n", n > 0 ? buf : "(no response)");

	printf("\n=== Tester Present ===\n");
	n = ds_tester_present(buf, sizeof(buf));
	printf("  Tester Present: %s\n", n > 0 ? buf : "(no response)");

	printf("\n=== Parameter Scan (%d params) ===\n", DS_ALL_PARAMS_COUNT);
	for (int i = 0; i < (int)DS_ALL_PARAMS_COUNT; i++) {
		uint8_t idx = DS_ALL_PARAMS[i];
		uint8_t data[64];
		int dlen = ds_ecu_read_param(idx, data, sizeof(data));
		printf("  [%02X] ", idx);
		if (dlen > 0) {
			ds_hexdump(data, dlen);
		} else {
			printf("(no response)\n");
		}
		usleep(50000); /* 50ms between reads */
	}
}

static void cmd_read(const char *idx_str)
{
	unsigned int idx;
	if (sscanf(idx_str, "%x", &idx) != 1 || idx > 0xFF) {
		fprintf(stderr, "Invalid index: %s (use hex, e.g. 'B0')\n", idx_str);
		return;
	}

	uint8_t data[64];
	int dlen = ds_ecu_read_param((uint8_t)idx, data, sizeof(data));
	printf("[%02X] ", idx);
	if (dlen > 0)
		ds_hexdump(data, dlen);
	else
		printf("(no response)\n");
}

static void cmd_shell(void)
{
	char line[256], buf[BUF_SZ];

	printf("DS150E shell (type command without \\r, 'q' to quit)\n");
	printf("  Prefix * is added if missing\n\n");

	while (1) {
		printf("ds150e> ");
		fflush(stdout);

		if (!fgets(line, sizeof(line), stdin))
			break;

		line[strcspn(line, "\n")] = 0;
		if (line[0] == 'q' && line[1] == 0)
			break;
		if (line[0] == 0)
			continue;

		char cmd[256];
		if (line[0] == '*')
			snprintf(cmd, sizeof(cmd), "%s", line);
		else
			snprintf(cmd, sizeof(cmd), "*%s", line);

		int n = ds_raw(cmd, buf, sizeof(buf));
		if (n > 0) {
			printf("  -> %s\n  ", buf);
			ds_hexdump((uint8_t *)buf, n);
		} else {
			printf("  -> (no response)\n");
		}
	}
}

/* ── Live decoded parameters ─────────────────────────────────────────── */

/*
 * Decode rules from PIDS.md + ANALYSE.md + capture 2026-04-14/29.
 * DS150E *608_21_XX uses MUT-II/DENSO service $21 (not standard OBD2).
 * All raw values are decimal bytes from *97 <idx> <b0> <b1> ... response.
 *
 * Confirmed from capture + shell probing on ASX 4N13:
 *   0x02: b1=coolant_raw (b-40°C), b2=intake_raw (b-40°C)
 *   0x03: b0/b1=rpm_hi/lo  RPM=(b0*256+b1)  [needs scaling verify]
 *   0x04: b0=load_raw, b2=throttle_raw
 *   0x08: unknown, 2 bytes
 *   0x09: unknown
 *   0x10: unknown
 *   0x12: unknown
 *   0x16: unknown
 *   0x19: unknown
 *   0x24: unknown, possibly fuel rail pressure
 *   0x74: DPF block — b0=soot%, b1=soot2, b4/b5=diff_pressure_raw
 *   0x11: b2/b3=dist_since_regen (km, 256*b2+b3)
 *   0x51: b0=unknown flags
 *   0x4E: b0/b1=unknown temps?
 *   B0-B9: injector corrections (idle..full load)
 */

static volatile int live_running = 1;
static void live_sigint(int s) { (void)s; live_running = 0; }

/* Known params to read in live mode — ordered by priority */
static const uint8_t LIVE_PARAMS[] = {
	0x02, /* coolant + intake temps */
	0x03, /* RPM */
	0x04, /* load / throttle */
	0x74, /* DPF block */
	0x11, /* dist since regen */
	0x16, /* unknown — may be boost or rail pressure */
	0x19, /* unknown */
	0x24, /* unknown — rail pressure candidate */
	0x51, /* unknown flags */
};
#define LIVE_PARAMS_N (sizeof(LIVE_PARAMS)/sizeof(LIVE_PARAMS[0]))

static void print_row(FILE *out, long ms,
                      int coolant, int intake, int rpm,
                      int load_raw, int throttle_raw,
                      int dpf_soot, int dpf_dp,
                      int dist_regen,
                      int p16_0, int p16_1,
                      int p19_0, int p24_0, int p24_1,
                      int p51_0)
{
	if (out != stdout) {
		fprintf(out, "%ld,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		        ms, coolant, intake, rpm, load_raw, throttle_raw,
		        dpf_soot, dpf_dp, dist_regen,
		        p16_0, p19_0, p24_0, p24_1, p51_0, p16_1);
		return;
	}

	/* Human-readable — two lines, overwrite with \033[2A */
	printf("\033[2K  time   : %ld\n", ms / 1000);
	printf("\033[2K  coolant: %3d°C   intake: %3d°C   RPM: %4d   load: %3d   throttle: %3d\n", coolant, intake, rpm, load_raw, throttle_raw);
	printf("\033[2K  DPF soot: %2d%%   DPF dp_raw: %5d   dist_regen: %4d km\n", dpf_soot, dpf_dp, dist_regen);
	printf("\033[2K  p16: %3d/%3d   p19: %3d   p24: %3d/%3d   p51: %02X\n", p16_0, p16_1, p19_0, p24_0, p24_1, (unsigned)p51_0);
	printf("\033[5A");
	fflush(stdout);
}

static void cmd_live(const char *csv_path)
{
	/* CAN session init — *607 first to close any stale session */
	char buf[256];
	ds_cmd("*607", buf, sizeof(buf), 2000);
	int n = ds_can_config_obd2(buf, sizeof(buf));
	if (n <= 0) {
		fprintf(stderr, "CAN init failed — is car running and BT connected?\n");
		return;
	}
	fprintf(stderr, "CAN session open. ECU: %.67s\n", buf);

	FILE *out = stdout;
	if (csv_path) {
		out = fopen(csv_path, "w");
		if (!out) { perror(csv_path); return; }
		setvbuf(out, NULL, _IONBF, 0);
		fprintf(out, "time_s,coolant_C,intake_C,rpm,load_raw,throttle_raw,"
		             "dpf_soot_pct,dpf_dp_raw,dist_regen_km,"
		             "p16_b0,p16_b1,p19_b0,p24_b0,p24_b1,p51_b0\n");
		fprintf(stderr, "Logging to %s (Ctrl+C to stop)\n", csv_path);
	} else {
		printf("\n\n\n\n\n"); /* reserve lines for overwrite */
	}

	signal(SIGINT, live_sigint);
	live_running = 1;
	int cycle = 0;

	while (live_running) {
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);
		long ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

		uint8_t d[64];
		int coolant = -999, intake = -999, rpm = -1;
		int load_raw = -1, throttle_raw = -1;
		int dpf_soot = -1, dpf_dp = -1, dist_regen = -1;
		int p16_0 = -1, p16_1 = -1, p19_0 = -1;
		int p24_0 = -1, p24_1 = -1, p51_0 = -1;

		if (ds_ecu_read_param(0x02, d, sizeof(d)) >= 3) {
			coolant = (int)d[1] - 40;
			intake  = (int)d[2] - 40;
		}
		if (ds_ecu_read_param(0x03, d, sizeof(d)) >= 2)
			rpm = (int)d[0] * 256 + (int)d[1];
		if (ds_ecu_read_param(0x04, d, sizeof(d)) >= 3) {
			load_raw     = d[0];
			throttle_raw = d[2];
		}
		if (ds_ecu_read_param(0x74, d, sizeof(d)) >= 6) {
			dpf_soot = (int)d[0];
			dpf_dp   = (int)d[4] * 256 + (int)d[5];
		}
		if (ds_ecu_read_param(0x11, d, sizeof(d)) >= 4)
			dist_regen = (int)d[2] * 256 + (int)d[3];
		if (ds_ecu_read_param(0x16, d, sizeof(d)) >= 2) {
			p16_0 = d[0]; p16_1 = d[1];
		}
		if (ds_ecu_read_param(0x19, d, sizeof(d)) >= 1)
			p19_0 = d[0];
		if (ds_ecu_read_param(0x24, d, sizeof(d)) >= 2) {
			p24_0 = d[0]; p24_1 = d[1];
		}
		if (ds_ecu_read_param(0x51, d, sizeof(d)) >= 1)
			p51_0 = d[0];

		print_row(out, ms, coolant, intake, rpm, load_raw, throttle_raw,
		          dpf_soot, dpf_dp, dist_regen,
		          p16_0, p16_1, p19_0, p24_0, p24_1, p51_0);
		cycle++;

		if (csv_path && cycle % 10 == 0)
			fprintf(stderr, "\r%d cycles", cycle);
	}

	if (out != stdout) {
		fclose(out);
		fprintf(stderr, "\n%d cycles saved to %s\n", cycle, csv_path);
	} else {
		printf("\n\n\n\n\n");
	}
}

int main(int argc, char **argv)
{
	const char *dev = DEFAULT_DEV;
	const char *cmd = NULL;
	int arg_start = 1;

	/* Find device and command */
	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "/dev/", 5) == 0) {
			dev = argv[i];
		} else if (!cmd) {
			cmd = argv[i];
			arg_start = i + 1;
		}
	}

	if (!cmd || strcmp(cmd, "help") == 0 || strcmp(cmd, "-h") == 0) {
		usage(argv[0]);
		return cmd ? 0 : 1;
	}

	printf("Opening %s ...\n", dev);
	if (ds_open(dev) < 0)
		return 1;
	printf("Connected (115200 8N1)\n\n");

	if (strcmp(cmd, "info") == 0) {
		cmd_info();
	} else if (strcmp(cmd, "voltage") == 0) {
		int mv = ds_get_voltage_mv();
		if (mv > 0)
			printf("Voltage: %d.%02d V\n", mv / 1000, (mv % 1000) / 10);
		else
			printf("No response\n");
	} else if (strcmp(cmd, "init") == 0) {
		cmd_init();
	} else if (strcmp(cmd, "selftest") == 0) {
		int f = ds_selftest_all();
		printf("\n%d failure(s)\n", f);
	} else if (strcmp(cmd, "scan") == 0) {
		cmd_scan();
	} else if (strcmp(cmd, "read") == 0) {
		if (arg_start < argc)
			cmd_read(argv[arg_start]);
		else
			fprintf(stderr, "Usage: ds150e read <hex_index>\n");
	} else if (strcmp(cmd, "cansetup") == 0) {
		char buf[256];
		int n = ds_can_config_obd2(buf, sizeof(buf));
		printf("CAN config: %s\n", n > 0 ? buf : "(no response)");
	} else if (strcmp(cmd, "ecuinit") == 0) {
		char buf[256];
		int n = ds_ecu_init(buf, sizeof(buf));
		printf("ECU init: %s\n", n > 0 ? buf : "(no response)");
	} else if (strcmp(cmd, "tester") == 0) {
		char buf[256];
		int n = ds_tester_present(buf, sizeof(buf));
		printf("Tester Present: %s\n", n > 0 ? buf : "(no response)");
	} else if (strcmp(cmd, "bt") == 0) {
		if (arg_start < argc && strcmp(argv[arg_start], "on") == 0)
			printf("BT: %s\n", ds_bt_enable() ? "enabled" : "failed");
		else if (arg_start < argc && strcmp(argv[arg_start], "off") == 0)
			printf("BT: %s\n", ds_bt_disable() ? "disabled" : "failed");
		else
			fprintf(stderr, "Usage: ds150e bt on|off\n");
	} else if (strcmp(cmd, "raw") == 0) {
		if (arg_start < argc) {
			char buf[BUF_SZ];
			int n = ds_raw(argv[arg_start], buf, sizeof(buf));
			if (n > 0) {
				printf("-> %s\n", buf);
				ds_hexdump((uint8_t *)buf, n);
			} else {
				printf("(no response)\n");
			}
		} else {
			fprintf(stderr, "Usage: ds150e raw '*cmd'\n");
		}
	} else if (strcmp(cmd, "live") == 0) {
		const char *csv = (arg_start < argc) ? argv[arg_start] : NULL;
		cmd_live(csv);
	} else if (strcmp(cmd, "monitor") == 0) {
		const char *csv = (arg_start < argc) ? argv[arg_start] : NULL;
		printf("=== Monitor mode (Ctrl+C to stop) ===\n");
		printf("Logging %d params%s\n\n", DS_ALL_PARAMS_COUNT,
		       csv ? csv : " to stdout");
		ds_monitor(DS_ALL_PARAMS, DS_ALL_PARAMS_COUNT, csv, 200);
	} else if (strcmp(cmd, "shell") == 0) {
		cmd_shell();
	} else {
		fprintf(stderr, "Unknown command: %s\n", cmd);
		usage(argv[0]);
	}

	ds_close();
	return 0;
}
