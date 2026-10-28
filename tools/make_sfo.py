#!/usr/bin/env python3
"""
Simple PS4 param.sfo generator for homebrew packages.
Generates compliant Sony SFO format without external dependencies.
"""
import struct
import sys
import os

def create_sfo(title_id="PICO00008", title="PICO-8 PS4", version="01.00"):
    # SFO Entry Formats:
    #   0x0404 = Integer (uint32, 4 bytes)
    #   0x0204 = Utf8 (null-terminated UTF-8 string; val_len = len(str) + 1, padded to max_len in data table)
    FMT_INT  = 0x0404
    FMT_UTF8 = 0x0204

    content_id = f"IV0000-{title_id}_00-PICO8PS4PLAYER00"
    entries = {
        "APP_TYPE":           (FMT_INT,  4,   1),
        "APP_VER":            (FMT_UTF8, 8,   version),
        "ATTRIBUTE":          (FMT_INT,  4,   0),
        "CATEGORY":           (FMT_UTF8, 4,   "gd"),
        "CONTENT_ID":         (FMT_UTF8, 48,  content_id),
        "DOWNLOAD_DATA_SIZE": (FMT_INT,  4,   0),
        "FORMAT":             (FMT_UTF8, 4,   "obs"),
        "PARENTAL_LEVEL":     (FMT_INT,  4,   1),
        "SYSTEM_VER":         (FMT_INT,  4,   0),
        "TITLE":              (FMT_UTF8, 128, title),
        "TITLE_ID":           (FMT_UTF8, 12,  title_id),
        "VERSION":            (FMT_UTF8, 8,   version),
    }

    # Sort keys alphabetically as required by SFO specification
    sorted_keys = sorted(entries.keys())
    count = len(sorted_keys)

    key_table = bytearray()
    key_offsets = {}
    for k in sorted_keys:
        key_offsets[k] = len(key_table)
        key_table.extend(k.encode('ascii') + b'\x00')

    val_table = bytearray()
    val_entries = []

    for k in sorted_keys:
        fmt, max_len, val = entries[k]
        val_offset = len(val_table)
        if fmt == FMT_INT:
            data = struct.pack('<I', val)
            val_len = 4
        else:
            raw = val.encode('utf-8')[:max_len - 1]
            val_len = len(raw) + 1
            data = raw + (b'\x00' * (max_len - len(raw)))
        val_table.extend(data)
        val_entries.append((key_offsets[k], fmt, val_len, max_len, val_offset))

    # Header: Magic (\x00PSF), Version (1.1), Key Table Offset, Data Table Offset, Entry Count
    header_size = 20
    entries_size = count * 16
    key_table_offset = header_size + entries_size
    data_table_offset = key_table_offset + len(key_table)

    # Pad data_table_offset to 4-byte boundary
    pad_len = (4 - (data_table_offset % 4)) % 4
    key_table.extend(b'\x00' * pad_len)
    data_table_offset += pad_len

    header = struct.pack(
        '<4sIIII',
        b'\x00PSF',
        0x00000101,
        key_table_offset,
        data_table_offset,
        count
    )

    entry_bytes = bytearray()
    for key_off, fmt, val_len, max_len, val_off in val_entries:
        entry_bytes.extend(struct.pack('<HHIII', key_off, fmt, val_len, max_len, val_off))

    return header + entry_bytes + key_table + val_table

if __name__ == "__main__":
    out_path = sys.argv[1] if len(sys.argv) > 1 else "param.sfo"
    sfo_data = create_sfo()
    with open(out_path, "wb") as f:
        f.write(sfo_data)
    print(f"Generated valid param.sfo at: {out_path} ({len(sfo_data)} bytes)")
