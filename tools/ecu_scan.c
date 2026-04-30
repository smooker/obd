/*
 * ecu_scan.c — Fast CAN address scanner for DS150E
 *
 * Scans 11-bit CAN request IDs in the diagnostic range (700..7EF).
 * For each address X, tries *668 with txid=X, rxid=X+8 (standard offset).
 * Prints any ECU that responds (not *255 timeout).
 *
 * Usage: ecu_scan [/dev/rfcomm0]
 */

#include "ds150e_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_DEV "/dev/rfcomm0"

/* Standard diagnostic CAN ID pairs to try — known Mitsubishi/common */
static const struct { int tx; int rx; const char *name; } KNOWN[] = {
	{ 0x7E0, 0x7E8, "Engine ECU" },
	{ 0x7E1, 0x7E9, "ECU2" },
	{ 0x7E2, 0x7EA, "ECU3" },
	{ 0x7E3, 0x7EB, "ECU4" },
	{ 0x7E4, 0x7EC, "ECU5" },
	{ 0x7E5, 0x7ED, "ECU6" },
	{ 0x7E6, 0x7EE, "ECU7" },
	{ 0x7E7, 0x7EF, "ECU8" },
	{ 0x700, 0x708, "0x700" },
	{ 0x710, 0x718, "0x710" },
	{ 0x720, 0x728, "0x720 (ABS/ESP Mitsubishi?)" },
	{ 0x730, 0x738, "0x730" },
	{ 0x740, 0x748, "0x740" },
	{ 0x750, 0x758, "0x750" },
	{ 0x760, 0x768, "0x760 (BCM?)" },
	{ 0x770, 0x778, "0x770" },
	{ 0x780, 0x788, "0x780 (ETACS/Body?)" },
	{ 0x790, 0x798, "0x790" },
	{ 0x7A0, 0x7A8, "0x7A0 (HVAC?)" },
	{ 0x7B0, 0x7B8, "0x7B0 (ABS?)" },
	{ 0x7C0, 0x7C8, "0x7C0 (Meter?)" },
	{ 0x7D0, 0x7D8, "0x7D0 (SRS/Airbag?)" },
	{ 0x7DF, 0x7E8, "0x7DF broadcast" },
};
#define KNOWN_N (int)(sizeof(KNOWN)/sizeof(KNOWN[0]))

static int try_ecu(int tx, int rx)
{
	char cmd[256], buf[512];

	/* close any active session */
	ds_cmd("*607", buf, sizeof(buf), 1000);

	snprintf(cmd, sizeof(cmd),
	         "*668_0_500_%03X_%03X_000_01C_02_3E_02_00_00_00_00_00_080_1A_87",
	         tx, rx);

	int n = ds_cmd(cmd, buf, sizeof(buf), 3000);

	/* *255 = timeout, *1 = no ECU, short response = no data */
	if (n <= 0) return 0;
	if (strcmp(buf, "*255") == 0) return 0;
	if (strcmp(buf, "*1") == 0) return 0;

	/* print raw response for analysis */
	printf("  FOUND  tx=%03X rx=%03X  n=%d  %s\n", tx, rx, n, buf);
	return 1;
}

int main(int argc, char **argv)
{
	const char *dev = DEFAULT_DEV;
	if (argc > 1 && strncmp(argv[1], "/dev/", 5) == 0)
		dev = argv[1];

	printf("ECU address scan on %s\n\n", dev);

	if (ds_open(dev) < 0)
		return 1;

	int found = 0;
	for (int i = 0; i < KNOWN_N; i++) {
		printf("  scan   tx=%03X rx=%03X  %-30s",
		       KNOWN[i].tx, KNOWN[i].rx, KNOWN[i].name);
		fflush(stdout);
		if (try_ecu(KNOWN[i].tx, KNOWN[i].rx)) {
			found++;
		} else {
			printf("  (no response)\n");
		}
	}

	printf("\n%d ECU(s) found.\n", found);
	ds_close();
	return 0;
}
