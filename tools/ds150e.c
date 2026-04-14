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

	printf("=== CAN Setup ===\n");
	int n = ds_can_config_obd2(buf, sizeof(buf));
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
