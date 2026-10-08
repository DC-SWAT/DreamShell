#!/usr/bin/env python3
# DreamShell MIE firmware
# Intel HEX to a raw image loaded at 0x8010
# (c)2026 SWAT <http://www.dc-swat.ru>

import sys

CODE_BASE = 0x8010
VECTORS = 0x9FF2
RAM_BASE = 0xA000


def load_ihx(path):
    mem = {}
    upper = 0
    for line in open(path):
        line = line.strip()
        if not line.startswith(":"):
            continue
        count = int(line[1:3], 16)
        addr = int(line[3:7], 16)
        kind = int(line[7:9], 16)
        if kind == 4:
            upper = int(line[9:13], 16) << 16
            continue
        if kind == 2:
            upper = int(line[9:13], 16) << 4
            continue
        if kind != 0:
            continue
        data = bytes.fromhex(line[9:9 + count * 2])
        for i, byte in enumerate(data):
            mem[upper + addr + i] = byte
    return mem


def area_span(map_path, name):
    base = None
    size = None
    for line in open(map_path):
        parts = line.split()
        if len(parts) < 2:
            continue
        if parts[1] == "s__" + name:
            base = int(parts[0], 16)
        elif parts[1] == "l__" + name:
            size = int(parts[0], 16)
    if base is None or size is None:
        return None
    return base, size


def main():
    ihx = sys.argv[1]
    mem = load_ihx(ihx)
    if CODE_BASE not in mem:
        sys.stderr.write("ihx has no code at 0x8010\n")
        return 1
    span = area_span(ihx[:-4] + ".map", "CODE")
    if span is None:
        sys.stderr.write("link map has no code area\n")
        return 1
    code_base, code_size = span
    if code_base + code_size > VECTORS:
        sys.stderr.write("code overlaps the interrupt table\n")
        return 1
    missing = [addr for addr in range(VECTORS, RAM_BASE) if addr not in mem]
    if missing:
        sys.stderr.write("interrupt table is missing at 0x9FF2\n")
        return 1
    size = RAM_BASE - CODE_BASE
    out = bytearray(size)
    for i in range(size):
        out[i] = mem.get(CODE_BASE + i, 0)
    with open(sys.argv[2], "wb") as fh:
        fh.write(out)
    sys.stdout.write("mie image %u bytes\n" % size)
    return 0


if __name__ == "__main__":
    sys.exit(main())
