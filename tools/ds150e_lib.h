/*
 * ds150e_lib.h — DS150E (Autocom CDP+) command library
 *
 * Protocol: ASCII '*<cmd>[_<arg>...]\r' over FTDI serial (115200 8N1)
 * Source: usbmon capture analysis + VCI+ firmware string dump
 */

#ifndef DS150E_LIB_H
#define DS150E_LIB_H

#include <stdint.h>

/* ── Serial port ─────────────────────────────────────────── */

/* Open and configure serial port. Returns fd or -1 */
int ds_open(const char *dev);

/* Close serial port */
void ds_close(void);

/* Send command, receive response. Returns bytes read or -1.
 * cmd: without \r (added automatically)
 * buf: response buffer (stripped of \r)
 * timeout_ms: read timeout */
int ds_cmd(const char *cmd, char *buf, int bufsz, int timeout_ms);

/* ── Device info (no vehicle needed) ─────────────────────── */

/* *20A → device name (e.g. "*CDP+") */
int ds_get_name(char *buf, int bufsz);

/* *200 → serial number (e.g. "*100251") */
int ds_get_serial(char *buf, int bufsz);

/* *201 → firmware version (e.g. "*1622") */
int ds_get_firmware(char *buf, int bufsz);

/* *203 → battery/supply voltage (e.g. "*0 1362" = 13.62V)
 * Returns voltage in millivolts, or -1 on error */
int ds_get_voltage_mv(void);

/* ── Init / handshake ────────────────────────────────────── */

/* *60b → *121 (init handshake step 1) */
int ds_init_60b(char *buf, int bufsz);

/* *60bc → *121 (init handshake step 2) */
int ds_init_60bc(char *buf, int bufsz);

/* ── Self-test (no vehicle needed, needs 12V power) ──────── */

/* *918A..F → *918x_OK or *918x_FAIL
 * test_id: 'A'..'F'
 * Returns 1=OK, 0=FAIL, -1=no response */
int ds_selftest(char test_id);

/* Run all self-tests A..F, print results. Returns count of failures */
int ds_selftest_all(void);

/* ── CAN bus configuration ───────────────────────────────── */

/* *668_<bus>_<rate>_<txid>_<rxid>_<mode>_<timing>_<payload...>
 * Configure CAN bus for diagnostic session.
 * bus: 0
 * rate: 500 (kbps)
 * txid: e.g. "7E0" (engine ECU request)
 * rxid: e.g. "7E8" (engine ECU reply)
 * Returns response length or -1 */
int ds_can_config(int bus, int rate, const char *txid, const char *rxid,
                  char *buf, int bufsz);

/* Standard OBD-II CAN config (bus 0, 500kbps, 7E0/7E8) */
int ds_can_config_obd2(char *buf, int bufsz);

/* Full CAN config with checksum — exact replay from capture
 * *668_0_500_7E0_7E8_000_01C_02_3E_02_00_00_00_00_00_080_1A_87 */
int ds_can_config_full(char *buf, int bufsz);

/* ── Periodic message slot ───────────────────────────────── */

/* *606<slot>_<canid>_<b1>_<b2>_..._<b8>_<chk>
 * Setup periodic CAN message (e.g. Tester Present keepalive)
 * slot: e.g. "B001"
 * canid: e.g. "7DF"
 * data: 8 bytes CAN payload
 * Returns response length or -1 */
int ds_periodic_msg(const char *slot, const char *canid,
                    const uint8_t data[8], char *buf, int bufsz);

/* Standard Tester Present keepalive (02 3E 02 00 00 00 00 00 on 7DF) */
int ds_tester_present(char *buf, int bufsz);

/* ── ECU communication ───────────────────────────────────── */

/* *608_18_00_FF_00 → ReadDTCInformation */
int ds_ecu_init(char *buf, int bufsz);

/* *609_0_01C_01C_10_92 → DiagnosticSessionControl */
int ds_session_physical(char *buf, int bufsz);

/* *609_0_7DF_7DF_10_92 → DiagnosticSessionControl broadcast */
int ds_session_broadcast(char *buf, int bufsz);

/* *608_30_51_07_00 → flow control / next step */
int ds_flow_control(char *buf, int bufsz);

/* ── Full vehicle init — Mitsubishi ASX 4N13 ────────────── */

/* Complete init sequence from capture 2026-04-14:
 * 1. *668 full CAN config (500kbps, 7E0/7E8, checksum)
 * 2. *609 session physical (01C, $10 $92)
 * 3. *609 session broadcast (7DF, $10 $92)
 * 4. *606B001 TesterPresent keepalive (7DF)
 * 5. *608_18 ReadDTCInformation
 * 6. *608_30 flow control
 *
 * Prints each step + response. Returns 0=ok, -1=fail.
 * ECU responds with part number 1860C481 on success. */
int ds_asx_init(void);

/* *608_21_<idx> → *97 <idx> <bytes>
 * Read ECU parameter by index.
 * idx: parameter index (hex, e.g. 0x02, 0x46, 0xB0)
 * data: output buffer for parameter bytes
 * Returns number of data bytes, or -1 */
int ds_ecu_read_param(uint8_t idx, uint8_t *data, int datasz);

/* *608_10_<idx> → start session / write
 * idx: parameter index */
int ds_ecu_write(uint8_t idx, char *buf, int bufsz);

/* ── Raw CAN frame ───────────────────────────────────────── */

/* *609_<bus>_<txid>_<rxid>_<dlc>_<data...>
 * Send raw CAN frame */
int ds_can_send_raw(int bus, const char *txid, const char *rxid,
                    int dlc, const uint8_t *data,
                    char *buf, int bufsz);

/* ── Bluetooth ───────────────────────────────────────────── */

/* Enable/disable wireless (Bluetooth)
 * Returns 1 if confirmed, 0 if failed */
int ds_bt_enable(void);
int ds_bt_disable(void);

/* ── Utility ─────────────────────────────────────────────── */

/* Send raw command string (with * prefix, without \r) */
int ds_raw(const char *cmd, char *buf, int bufsz);

/* Parse voltage response "*0 NNNN" → millivolts */
int ds_parse_voltage(const char *resp);

/* Parse param response "*97 XX <bytes>" → data bytes
 * Returns number of bytes parsed */
int ds_parse_param(const char *resp, uint8_t *idx, uint8_t *data, int datasz);

/* Hex dump to stdout */
void ds_hexdump(const uint8_t *data, int len);

/* ── Logging / monitoring ────────────────────────────────── */

/* Continuous parameter monitor — reads params in loop, writes CSV.
 * params: array of param indices to monitor
 * count: number of params
 * csv_path: output CSV file (NULL = stdout)
 * interval_ms: delay between full scan cycles
 * Returns 0 on clean exit (Ctrl+C), -1 on error */
int ds_monitor(const uint8_t *params, int count,
               const char *csv_path, int interval_ms);

/* ── Known ECU parameter indices (Mitsubishi ASX 4N13) ───── */

/* Core sensors */
#define DS_PARAM_02    0x02  /* ? */
#define DS_PARAM_03    0x03  /* ? (pattern shared with 13, 5D) */
#define DS_PARAM_04    0x04  /* ? (variable last byte) */
#define DS_PARAM_08    0x08
#define DS_PARAM_09    0x09
#define DS_PARAM_10    0x10
#define DS_PARAM_12    0x12
#define DS_PARAM_14    0x14
#define DS_PARAM_15    0x15
#define DS_PARAM_16    0x16
#define DS_PARAM_17    0x17  /* 64 64 = 100,100 (percentage?) */
#define DS_PARAM_18    0x18
#define DS_PARAM_19    0x19

/* Injector codes (ASCII fragments) */
#define DS_PARAM_46    0x46  /* "/6Ruk" */
#define DS_PARAM_4F    0x4F  /* "77/H" */
#define DS_PARAM_5B    0x5B  /* "AAA6" */

/* Calibration lookup table (X→121 pairs) */
#define DS_PARAM_A0    0xA0
#define DS_PARAM_A1    0xA1
#define DS_PARAM_A3    0xA3
#define DS_PARAM_A4    0xA4
#define DS_PARAM_A7    0xA7
#define DS_PARAM_A8    0xA8

/* Injector quantity correction (10 work points × 2 values) */
#define DS_PARAM_B0    0xB0  /* idle */
#define DS_PARAM_B1    0xB1
#define DS_PARAM_B2    0xB2
#define DS_PARAM_B3    0xB3
#define DS_PARAM_B4    0xB4
#define DS_PARAM_B5    0xB5
#define DS_PARAM_B6    0xB6
#define DS_PARAM_B7    0xB7
#define DS_PARAM_B8    0xB8
#define DS_PARAM_B9    0xB9  /* full load */

/* All known param indices for full scan */
static const uint8_t DS_ALL_PARAMS[] = {
    0x02, 0x03, 0x04, 0x08, 0x09, 0x10, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0x24, 0x26, 0x27, 0x28,
    0x46, 0x47, 0x49, 0x4A, 0x4B, 0x4C, 0x4F,
    0x58, 0x59, 0x5B, 0x5D, 0x5F,
    0x74,
    0xA0, 0xA1, 0xA3, 0xA4, 0xA7, 0xA8,
    0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5,
    0xB6, 0xB7, 0xB8, 0xB9,
    0xBE, 0xBF,
    /* truncated in prev capture: */
    0x01, 0x11, 0x4E, 0x51, 0xA2, 0xA5, 0xA6,
};
#define DS_ALL_PARAMS_COUNT (sizeof(DS_ALL_PARAMS)/sizeof(DS_ALL_PARAMS[0]))

#endif /* DS150E_LIB_H */
