#!/usr/bin/env python3
"""
acz_decrypt.py — Autocom 2021.11 .acz/.sdf analysis tool

.acz files: AES encrypted + GZip compressed ZIP archives
.sdf files: SQL Server Compact Edition with password encryption

Key insights from Protocol.dll reverse engineering:
- SQL CE connection: "Data Source=<file>; Encrypt = TRUE; Password=<pwd>;
                      encryption mode=ppc2003 compatibility; Max Database Size=1024"
- .acz: CryptoObfuscator + .NET Reactor protected AES
- 5-char lookup table strings for obfuscation layer
"""

import sys
import os
import struct
import zipfile
import hashlib
import re
from pathlib import Path

BASE = Path(__file__).parent.parent / "files" / "Autocom 2021.11 Software"
DATA = BASE / "Data"


def scan_dll_strings(dll_path, min_len=4):
    """Extract all readable strings from a .NET DLL"""
    with open(dll_path, 'rb') as f:
        data = f.read()

    # Find #US heap (User Strings)
    pe_off = struct.unpack_from('<I', data, 0x3C)[0]
    opt_off = pe_off + 24
    dd_off = opt_off + 96
    cli_rva = struct.unpack_from('<I', data, dd_off + 14*8)[0]

    num_sections = struct.unpack_from('<H', data, pe_off + 6)[0]
    opt_size = struct.unpack_from('<H', data, pe_off + 20)[0]
    sec_off = pe_off + 24 + opt_size
    sections = []
    for i in range(num_sections):
        o = sec_off + i * 40
        rva = struct.unpack_from('<I', data, o+12)[0]
        vsz = struct.unpack_from('<I', data, o+8)[0]
        rawoff = struct.unpack_from('<I', data, o+20)[0]
        sections.append((rva, vsz, rawoff))

    def rva2off(rva):
        for srva, vsz, rawoff in sections:
            if srva <= rva < srva + vsz:
                return rva - srva + rawoff
        return None

    cli_off = rva2off(cli_rva)
    meta_rva = struct.unpack_from('<I', data, cli_off + 8)[0]
    meta_off = rva2off(meta_rva)

    ver_len = struct.unpack_from('<I', data, meta_off + 12)[0]
    ver_len = (ver_len + 3) & ~3
    streams_start = meta_off + 16 + ver_len
    num_streams = struct.unpack_from('<H', data, streams_start + 2)[0]

    pos = streams_start + 4
    streams = {}
    for i in range(num_streams):
        s_off = struct.unpack_from('<I', data, pos)[0]
        s_size = struct.unpack_from('<I', data, pos + 4)[0]
        name_start = pos + 8
        name_end = data.index(b'\x00', name_start)
        name = data[name_start:name_end].decode('ascii')
        name_end = (name_end + 4) & ~3
        pos = name_end
        streams[name] = (meta_off + s_off, s_size)

    results = []
    if '#US' in streams:
        us_off, us_size = streams['#US']
        pos = us_off + 1
        end = us_off + us_size
        while pos < end:
            b = data[pos]
            if b == 0:
                pos += 1
                continue
            if b < 0x80:
                length = b; pos += 1
            elif b < 0xC0:
                length = ((b & 0x3F) << 8) | data[pos+1]; pos += 2
            else:
                length = ((b & 0x1F) << 24) | (data[pos+1] << 16) | (data[pos+2] << 8) | data[pos+3]; pos += 4

            if length > 0 and pos + length <= end:
                raw = data[pos:pos+length]
                try:
                    s = raw[:-1].decode('utf-16-le') if length > 1 else ""
                    if len(s) >= min_len:
                        results.append(s)
                except:
                    pass
            pos += length

    return results


def find_password_candidates():
    """Search all DLLs for potential SQL CE passwords"""
    print("=== Searching for SQL CE password candidates ===\n")

    dlls = [
        "Protocol.dll",
        "Diagnostic.dll",
        "Diagnostic.Protocols.dll",
        "Diagnostic.Common.dll",
        "Diagnostic.Core.dll",
        "Diagnostic.Core.Services.dll",
        "Core.Crypto.Lib.dll",
        "CommonSoftware.Core.dll",
        "diagnosis.dll",
        "VehicleSelection.dll",
        "SqlCEUtils.dll",
    ]

    for dll_name in dlls:
        dll_path = BASE / dll_name
        if not dll_path.exists():
            continue

        print(f"--- {dll_name} ---")

        # Search raw bytes for password-like patterns near SQL CE keywords
        with open(dll_path, 'rb') as f:
            raw = f.read()

        # Look for "Password" near connection strings
        for m in re.finditer(b'P\x00a\x00s\x00s\x00w\x00o\x00r\x00d\x00', raw):
            ctx = raw[m.start()-100:m.end()+200]
            # Try to decode UTF-16 context
            try:
                decoded = ctx.decode('utf-16-le', errors='replace')
                cleaned = ''.join(c if c.isprintable() else '|' for c in decoded)
                print(f"  Found 'Password' context: ...{cleaned[:150]}...")
            except:
                pass

        # Extract user strings
        try:
            strings = scan_dll_strings(dll_path, min_len=3)
            # Filter for potential passwords/connection strings
            for s in strings:
                if any(k in s.lower() for k in ['password', 'encrypt', 'data source', 'connection']):
                    print(f"  String: '{s}'")
        except Exception as e:
            print(f"  Error parsing: {e}")

        print()


def analyze_sdf(sdf_path):
    """Analyze SQL Server Compact .sdf file header"""
    with open(sdf_path, 'rb') as f:
        header = f.read(512)

    print(f"=== {os.path.basename(sdf_path)} ===")
    print(f"  Size: {os.path.getsize(sdf_path):,} bytes")
    print(f"  Magic: {header[:4].hex()}")

    # SQL CE 3.5 header signature
    if header[:4] == b'\x00\x01\x00\x00':
        print("  Type: SQL Server Compact Edition")
        # Version at offset 0x14
        ver = struct.unpack_from('<I', header, 0x14)[0]
        print(f"  Version: {ver}")
        # Database ID at offset 0x38
        dbid = header[0x38:0x48].hex()
        print(f"  DB ID: {dbid}")
        # Encrypted flag
        enc_flag = header[0x54] if len(header) > 0x54 else 0
        print(f"  Encrypted flag byte @0x54: {enc_flag:#x}")

    print(f"  First 64 bytes: {header[:64].hex()}")
    print()


def analyze_acz(acz_path):
    """Analyze .acz file structure"""
    print(f"=== {os.path.basename(acz_path)} ===")
    print(f"  Size: {os.path.getsize(acz_path):,} bytes")

    try:
        with zipfile.ZipFile(acz_path) as zf:
            names = zf.namelist()
            print(f"  Entries: {len(names)}")
            # Check first entry
            if names:
                info = zf.getinfo(names[0])
                data = zf.read(names[0])
                print(f"  First: {names[0]} ({len(data)} bytes)")
                print(f"  Data sample: {data[:32].hex()}")

                # Check if it's GZip after AES decrypt
                if data[:2] == b'\x1f\x8b':
                    print("  *** GZip compressed (not encrypted?)")
                else:
                    # High entropy = encrypted
                    unique = len(set(data[:256]))
                    print(f"  Unique bytes in first 256: {unique}/256 (encrypted={unique > 200})")
    except Exception as e:
        print(f"  Error: {e}")
    print()


def main():
    if len(sys.argv) > 1:
        cmd = sys.argv[1]
    else:
        cmd = "all"

    if cmd == "passwords" or cmd == "all":
        find_password_candidates()

    if cmd == "sdf" or cmd == "all":
        print("\n=== SDF File Analysis ===\n")
        for sdf in sorted(DATA.glob("*.sdf")):
            analyze_sdf(sdf)

    if cmd == "acz" or cmd == "all":
        print("\n=== ACZ File Analysis ===\n")
        for acz in sorted(DATA.glob("*.acz")):
            analyze_acz(acz)


if __name__ == "__main__":
    main()
