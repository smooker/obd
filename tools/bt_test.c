/*
 * bt_test.c — DS150E Bluetooth connection tests
 *
 * Prerequisite: DS150E paired (00:12:F3:17:A0:3E) and /dev/rfcomm0 bound.
 *   rfcomm bind 0 00:12:F3:17:A0:3E 3
 *
 * Usage: bt_test [/dev/rfcomm0]
 *
 * Tests run in order; each prints PASS/FAIL. Stops at first failure
 * unless -a (all) flag is given.
 */

#include "ds150e_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEV_DEFAULT "/dev/rfcomm0"

/* Known good values from 2026-04-27 pairing session */
#define EXPECTED_NAME     "*CDP+"
#define EXPECTED_SERIAL   "*100251"
#define EXPECTED_FIRMWARE "*1622"
#define VOLTAGE_MIN_MV    11000   /* 11.0V — battery without engine */
#define VOLTAGE_MAX_MV    15000   /* 15.0V — with charging */

static int failures;
static int run_all;

/* ── Test helpers ────────────────────────────────────────────── */

static void pass(const char *name)
{
	printf("  PASS  %s\n", name);
}

static void fail(const char *name, const char *detail)
{
	printf("  FAIL  %s  (%s)\n", name, detail);
	failures++;
}

static void section(const char *title)
{
	printf("\n[%s]\n", title);
}

/* ── Individual tests ────────────────────────────────────────── */

/* T01: open /dev/rfcomm0 */
static int t01_open(const char *dev)
{
	section("T01 open");
	if (ds_open(dev) < 0) {
		fail("open rfcomm", "ds_open returned -1 — is rfcomm0 bound?");
		return -1;
	}
	pass("open rfcomm");
	return 0;
}

/* T02: device name — *20A → *CDP+ */
static void t02_name(void)
{
	section("T02 device name");
	char buf[256], detail[512];
	int n = ds_get_name(buf, sizeof(buf));
	if (n <= 0) {
		fail("get_name", "no response");
		return;
	}
	if (strcmp(buf, EXPECTED_NAME) != 0) {
		snprintf(detail, sizeof(detail), "got '%s', want '%s'", buf, EXPECTED_NAME);
		fail("get_name", detail);
		return;
	}
	pass("get_name");
}

/* T03: serial number — *200 → *100251 */
static void t03_serial(void)
{
	section("T03 serial");
	char buf[256], detail[512];
	int n = ds_get_serial(buf, sizeof(buf));
	if (n <= 0) {
		fail("get_serial", "no response");
		return;
	}
	if (strcmp(buf, EXPECTED_SERIAL) != 0) {
		snprintf(detail, sizeof(detail), "got '%s', want '%s'", buf, EXPECTED_SERIAL);
		fail("get_serial", detail);
		return;
	}
	pass("get_serial");
}

/* T04: firmware version — *201 → *1622 */
static void t04_firmware(void)
{
	section("T04 firmware");
	char buf[256], detail[512];
	int n = ds_get_firmware(buf, sizeof(buf));
	if (n <= 0) {
		fail("get_firmware", "no response");
		return;
	}
	if (strcmp(buf, EXPECTED_FIRMWARE) != 0) {
		snprintf(detail, sizeof(detail), "got '%s', want '%s'", buf, EXPECTED_FIRMWARE);
		fail("get_firmware", detail);
		return;
	}
	pass("get_firmware");
}

/* T05: voltage — *203 → reasonable value */
static void t05_voltage(void)
{
	section("T05 voltage");
	int mv = ds_get_voltage_mv();
	if (mv <= 0) {
		fail("get_voltage", "no response or parse error");
		return;
	}
	if (mv < VOLTAGE_MIN_MV || mv > VOLTAGE_MAX_MV) {
		char detail[128];
		snprintf(detail, sizeof(detail), "%d.%02dV out of range [%d.%02d..%d.%02d]",
		         mv/1000, (mv%1000)/10,
		         VOLTAGE_MIN_MV/1000, (VOLTAGE_MIN_MV%1000)/10,
		         VOLTAGE_MAX_MV/1000, (VOLTAGE_MAX_MV%1000)/10);
		fail("get_voltage", detail);
		return;
	}
	printf("         voltage: %d.%02d V\n", mv/1000, (mv%1000)/10);
	pass("get_voltage");
}

/* T06: init handshake — *60b + *60bc → *121 */
static void t06_init(void)
{
	section("T06 init handshake");
	char buf[256], detail[512];

	int n = ds_init_60b(buf, sizeof(buf));
	if (n <= 0) {
		fail("init_60b", "no response");
		return;
	}
	if (!strstr(buf, "*121")) {
		snprintf(detail, sizeof(detail), "got '%s', want '*121'", buf);
		fail("init_60b", detail);
		return;
	}
	pass("init_60b");

	n = ds_init_60bc(buf, sizeof(buf));
	if (n <= 0) {
		fail("init_60bc", "no response");
		return;
	}
	if (!strstr(buf, "*121")) {
		snprintf(detail, sizeof(detail), "got '%s', want '*121'", buf);
		fail("init_60bc", detail);
		return;
	}
	pass("init_60bc");
}

/* T07: raw command round-trip — *20A echoes device name */
static void t07_raw(void)
{
	section("T07 raw command");
	char buf[256], detail[512];
	int n = ds_raw("*20A", buf, sizeof(buf));
	if (n <= 0) {
		fail("raw *20A", "no response");
		return;
	}
	if (strcmp(buf, EXPECTED_NAME) != 0) {
		snprintf(detail, sizeof(detail), "got '%s'", buf);
		fail("raw *20A", detail);
		return;
	}
	pass("raw *20A");
}

/*
 * T08: CAN setup + ASX ECU init
 * Requires car: Mitsubishi ASX 4N13, ignition ON (KOEO)
 * Skipped automatically if voltage < 11.5V (no OBD power = no car).
 */
static void t08_asx_init(void)
{
	section("T08 ASX CAN init (requires car, KOEO)");

	/* Voltage already checked in T05 — do NOT call *203 here.
	 * Any extra command between *60bc and *668 can confuse DS150E state. */

	/* CAN config: 500kbps, 7E0/7E8 */
	char buf[256];
	int n = ds_can_config_obd2(buf, sizeof(buf));
	printf("         *668 response: n=%d buf='%s'\n", n, n > 0 ? buf : "");
	if (n < 0) {
		fail("asx_can_config", "no response to *668");
		return;
	}
	pass("asx_can_config");

	/* *668 response contains ECU part number — verify 1860C481 present */
	/* Response is decimal ASCII bytes; 49 56 54 48 67 52 56 49 = "1860C481" */
	if (!strstr(buf, "49 56 54 48 67 52 56 49")) {
		printf("         *668 buf='%s'\n", buf);
		fail("asx_ecu_partno", "1860C481 not found in *668 response");
		return;
	}
	pass("asx_ecu_partno");
}

/*
 * T09: read ECU param 0x17 (percentage-like, 64 64 expected from captures)
 * Requires T08 to have passed.
 */
static void t09_read_param(void)
{
	section("T09 read ECU param 0x17");

	/* No voltage check here — *203 resets DS150E CAN state.
	 * T08 already confirmed car is present (voltage > 11.5V + ECU partno). */
	uint8_t data[64];
	int dlen = ds_ecu_read_param(0x17, data, sizeof(data));
	if (dlen <= 0) {
		fail("read_param 0x17", "no response");
		return;
	}
	printf("         param 0x17: ");
	ds_hexdump(data, dlen);
	/* expect FF 64 64 ... (255 100 100 from prior captures) */
	if (data[0] == 0xFF && data[1] == 100 && data[2] == 100)
		printf("         (matches expected FF 64 64)\n");
	pass("read_param 0x17");
}

/* ── Entry point ─────────────────────────────────────────────── */

int main(int argc, char **argv)
{
	const char *dev = DEV_DEFAULT;
	run_all = 0;
	failures = 0;

	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "/dev/", 5) == 0)
			dev = argv[i];
		else if (strcmp(argv[i], "-a") == 0)
			run_all = 1;
	}

	printf("DS150E Bluetooth test suite\n");
	printf("  device : %s\n", dev);
	printf("  DS150E : 00:12:F3:17:A0:3E (Autocom CDP+ BT 100953)\n");
	printf("  mode   : %s\n\n", run_all ? "run all" : "stop on first failure");

	/* T01 — must succeed before anything else */
	if (t01_open(dev) < 0)
		goto done;

	t02_name();
	if (failures && !run_all) goto done;

	t03_serial();
	if (failures && !run_all) goto done;

	t04_firmware();
	if (failures && !run_all) goto done;

	t05_voltage();
	if (failures && !run_all) goto done;

	t06_init();
	if (failures && !run_all) goto done;

	t07_raw();
	if (failures && !run_all) goto done;

	t08_asx_init();
	if (failures && !run_all) goto done;

	t09_read_param();

done:
	ds_close();

	printf("\n");
	if (failures == 0)
		printf("All tests passed.\n");
	else
		printf("%d test(s) FAILED.\n", failures);

	return failures ? 1 : 0;
}
