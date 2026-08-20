#!/usr/bin/env python3
#
# Rearrange a file's FAT32 cluster chain on a DreamShell IDE image.
# Copyright (C) 2026 SWAT
# http://www.dc-swat.ru
#

import argparse
import os
import struct
import sys

SECTOR = 512
EOC_MIN = 0x0FFFFFF8
ATTR_DIR = 0x10
ATTR_VOL = 0x08
ATTR_LFN = 0x0F


def die(msg):
    sys.stderr.write('Error: %s\n' % msg)
    sys.exit(1)


def split_even(items, n):
    if n < 1:
        n = 1
    if n > len(items):
        n = len(items)
    if n < 1:
        return []
    base = len(items) // n
    extra = len(items) % n
    out = []
    i = 0
    for k in range(n):
        take = base + (1 if k < extra else 0)
        if take <= 0:
            continue
        out.append(items[i:i + take])
        i += take
    return out


def extents_of(chain):
    ext = []
    i = 0
    while i < len(chain):
        tcl = chain[i]
        ncl = 1
        while i + ncl < len(chain) and chain[i + ncl] == tcl + ncl:
            ncl += 1
        ext.append((ncl, tcl))
        i += ncl
    return ext


def clmt_ulen(ext):
    return 2 + 2 * len(ext)


def contiguous_sect_old(clust, fptr, ss, csize, ext):
    csect = int(fptr // ss) & (csize - 1)
    if clust == 0:
        ncl, tcl = ext[0]
        return (ncl * csize) - csect
    for ncl, tcl in ext:
        if clust < tcl + ncl:
            ncl = ncl - (clust - tcl)
            if ncl == 0:
                return 0
            return (ncl * csize) - csect
    return 0


def contiguous_sect_new(clust, fptr, ss, csize, ext):
    csect = int(fptr // ss) & (csize - 1)
    cl = int(fptr // ss) // csize
    for ncl, tcl in ext:
        if cl < ncl:
            ncl = ncl - cl
            if ncl == 0:
                return 0
            return (ncl * csize) - csect
        cl -= ncl
    return 0


def expected_contig(chain, clust, fptr, ss, csize):
    csect = int(fptr // ss) & (csize - 1)
    idx = int(fptr // ss) // csize
    if idx >= len(chain) or chain[idx] != clust:
        for i, c in enumerate(chain):
            if c == clust:
                idx = i
                break
        else:
            return 0
    n = 1
    while idx + n < len(chain) and chain[idx + n] == clust + n:
        n += 1
    return n * csize - csect


def simulate_read(chain, fsize, ss, csize, fn):
    ext = extents_of(chain)
    if not ext:
        return 0, 0
    remain = fsize
    fptr = 0
    clust = chain[0]
    mismatches = 0
    steps = 0
    while remain >= ss:
        csect = int(fptr // ss) & (csize - 1)
        cc = remain // ss
        if csect + cc > csize:
            cs = fn(clust, fptr, ss, csize, ext)
            if cc > cs:
                cc = cs
            if csect + cc > csize:
                next_off = fptr + ss * cc
                if next_off < fsize:
                    nidx = int(next_off // ss) // csize
                    if 0 <= nidx < len(chain):
                        clust = chain[nidx]
        want = expected_contig(chain, chain[int(fptr // ss) // csize], fptr, ss, csize)
        got = fn(chain[int(fptr // ss) // csize], fptr, ss, csize, ext)
        if csect + (remain // ss) > csize and got != want:
            mismatches += 1
        if cc < 1:
            cc = 1
        steps += 1
        fptr += cc * ss
        remain -= cc * ss
        sidx = int(fptr // ss) // csize if fptr < fsize else len(chain) - 1
        if 0 <= sidx < len(chain):
            clust = chain[sidx]
    return mismatches, steps


def make_pattern(chain, pattern, pieces):
    if not chain:
        return chain
    if pattern == 'restore':
        slots = sorted(chain)
        return slots
    if pattern == 'scatter':
        return list(reversed(sorted(chain)))
    chunks = split_even(sorted(chain), pieces)
    if not chunks:
        return list(chain)
    if pattern == 'reverse':
        return [c for ch in reversed(chunks) for c in ch]
    if pattern == 'forward':
        if len(chunks) < 3:
            return [c for ch in chunks for c in ch]
        mid = chunks[1:-1]
        return [c for ch in ([chunks[0]] + mid[1::2] + chunks[-1:] + mid[0::2]) for c in ch]
    if pattern == 'zebra':
        a = [c for ch in chunks[0::2] for c in ch]
        b = [c for ch in chunks[1::2] for c in ch]
        return a + b
    die('unknown pattern: %s' % pattern)


class Fat32(object):
    def __init__(self, path, writable):
        self.path = path
        self.fp = open(path, 'r+b' if writable else 'rb')
        mbr = self.read_lba(0)
        if mbr[510:512] != b'\x55\xaa':
            die('no MBR signature')
        self.part_lba = struct.unpack_from('<I', mbr, 0x1BE + 8)[0]
        vbr = self.read_lba(self.part_lba)
        self.bps = struct.unpack_from('<H', vbr, 11)[0]
        if self.bps != SECTOR:
            die('unsupported sector size %d' % self.bps)
        self.spc = vbr[13]
        self.reserved = struct.unpack_from('<H', vbr, 14)[0]
        self.nfats = vbr[16]
        self.fat_sectors = struct.unpack_from('<I', vbr, 36)[0]
        self.root = struct.unpack_from('<I', vbr, 44)[0]
        self.cluster_bytes = self.spc * SECTOR
        self.fat_lba = self.part_lba + self.reserved
        self.data_lba = self.fat_lba + self.nfats * self.fat_sectors
        nent = (self.fat_sectors * SECTOR) // 4
        self.fp.seek(self.fat_lba * SECTOR)
        raw = self.fp.read(nent * 4)
        self.fat = [struct.unpack_from('<I', raw, i * 4)[0] for i in range(nent)]

    def close(self):
        self.fp.close()

    def read_lba(self, lba, count=1):
        self.fp.seek(lba * SECTOR)
        return self.fp.read(count * SECTOR)

    def write_lba(self, lba, data):
        self.fp.seek(lba * SECTOR)
        self.fp.write(data)

    def cluster_lba(self, cl):
        return self.data_lba + (cl - 2) * self.spc

    def fat_next(self, cl):
        return self.fat[cl] & 0x0FFFFFFF

    def set_fat(self, cl, val):
        self.fat[cl] = (self.fat[cl] & 0xF0000000) | (val & 0x0FFFFFFF)

    def flush_fat(self):
        blob = b''.join(struct.pack('<I', v) for v in self.fat)
        for i in range(self.nfats):
            self.fp.seek((self.fat_lba + i * self.fat_sectors) * SECTOR)
            self.fp.write(blob)

    def read_cluster(self, cl):
        return self.read_lba(self.cluster_lba(cl), self.spc)

    def write_cluster(self, cl, data):
        if len(data) < self.cluster_bytes:
            data = data + b'\x00' * (self.cluster_bytes - len(data))
        self.write_lba(self.cluster_lba(cl), data[:self.cluster_bytes])

    def walk_chain(self, first):
        out = []
        cl = first
        seen = set()
        while cl >= 2:
            if cl in seen:
                die('FAT loop at cluster %d' % cl)
            seen.add(cl)
            out.append(cl)
            nxt = self.fat_next(cl)
            if nxt >= EOC_MIN:
                break
            if nxt == 0 or nxt == 1 or nxt == 0x0FFFFFF7:
                die('bad FAT link %d -> %d' % (cl, nxt))
            cl = nxt
        return out

    def dir_records(self, first):
        recs = []
        for cl in self.walk_chain(first):
            raw = self.read_cluster(cl)
            for i in range(0, len(raw), 32):
                recs.append((cl, i, raw[i:i + 32]))
        return recs

    def parse_dir(self, first):
        entries = []
        lfn = []
        for cl, off, rec in self.dir_records(first):
            if rec[0] == 0x00:
                break
            if rec[0] == 0xE5:
                lfn = []
                continue
            attr = rec[11]
            if attr == ATTR_LFN:
                seq = rec[0]
                chunk = rec[1:11] + rec[14:26] + rec[28:32]
                chars = []
                for i in range(0, len(chunk), 2):
                    u = chunk[i] | (chunk[i + 1] << 8)
                    if u == 0 or u == 0xFFFF:
                        break
                    chars.append(chr(u))
                if seq & 0x40:
                    lfn = [''.join(chars)]
                else:
                    lfn.append(''.join(chars))
                continue
            if attr & ATTR_VOL:
                lfn = []
                continue
            name11 = rec[0:11]
            short = name11[0:8].decode('ascii', 'replace').rstrip()
            ext = name11[8:11].decode('ascii', 'replace').rstrip()
            shortn = short + (('.' + ext) if ext else '')
            if lfn:
                name = ''.join(reversed(lfn))
            else:
                name = shortn
            lfn = []
            clst = struct.unpack_from('<H', rec, 26)[0] | (struct.unpack_from('<H', rec, 20)[0] << 16)
            size = struct.unpack_from('<I', rec, 28)[0]
            entries.append({
                'name': name,
                'short': shortn,
                'dir': bool(attr & ATTR_DIR),
                'cluster': clst,
                'size': size,
                'dir_cl': cl,
                'dir_off': off,
            })
        return entries

    def find_path(self, path):
        parts = [p for p in path.replace('\\', '/').split('/') if p and p != '.']
        cl = self.root
        ent = None
        if not parts:
            return {'name': '/', 'dir': True, 'cluster': self.root, 'size': 0,
                    'dir_cl': 0, 'dir_off': 0}
        for i, part in enumerate(parts):
            found = None
            for e in self.parse_dir(cl):
                if e['name'].lower() == part.lower() or e['short'].lower() == part.lower():
                    found = e
                    break
            if found is None:
                return None
            if i + 1 < len(parts):
                if not found['dir']:
                    return None
                cl = found['cluster']
            else:
                ent = found
        return ent

    def list_files(self, cl, prefix):
        out = []
        for e in self.parse_dir(cl):
            if e['name'] in ('.', '..'):
                continue
            p = prefix + '/' + e['name'] if prefix else e['name']
            if e['dir']:
                out.extend(self.list_files(e['cluster'], p))
            else:
                out.append((p, e))
        return out

    def permute_file(self, ent, new_chain):
        old = self.walk_chain(ent['cluster'])
        if len(old) != len(new_chain):
            die('chain length mismatch %d vs %d' % (len(old), len(new_chain)))
        if sorted(old) != sorted(new_chain):
            die('pattern must reuse the same clusters')
        src_of = {new_chain[i]: old[i] for i in range(len(old))}
        visited = set()
        for dest in new_chain:
            if dest in visited:
                continue
            cycle = []
            cur = dest
            while cur not in visited:
                visited.add(cur)
                cycle.append(cur)
                cur = src_of[cur]
                if cur == dest:
                    break
            if len(cycle) == 1 and src_of[cycle[0]] == cycle[0]:
                continue
            tmp = [self.read_cluster(c) for c in cycle]
            src_data = {cycle[i]: tmp[i] for i in range(len(cycle))}
            for d in cycle:
                self.write_cluster(d, src_data[src_of[d]])
            del src_data, tmp
        for i, cl in enumerate(new_chain):
            if i + 1 < len(new_chain):
                self.set_fat(cl, new_chain[i + 1])
            else:
                self.set_fat(cl, 0x0FFFFFFF)
        self.flush_fat()
        if new_chain[0] != ent['cluster']:
            rec = bytearray(self.read_cluster(ent['dir_cl']))
            rec[ent['dir_off'] + 26:ent['dir_off'] + 28] = struct.pack('<H', new_chain[0] & 0xFFFF)
            rec[ent['dir_off'] + 20:ent['dir_off'] + 22] = struct.pack('<H', (new_chain[0] >> 16) & 0xFFFF)
            self.write_cluster(ent['dir_cl'], bytes(rec))
            ent['cluster'] = new_chain[0]
        self.fp.flush()
        os.fsync(self.fp.fileno())


def print_info(path, chain, size, spc, ss):
    ext = extents_of(chain)
    ulen = clmt_ulen(ext)
    print('%s' % path)
    print('  size %d  clusters %d  cluster %d KB  extents %d  cltbl[0]=%d' % (
        size, len(chain), (spc * ss) // 1024, len(ext), ulen))
    show = ext if len(ext) <= 24 else ext[:12] + ext[-8:]
    for i, (ncl, tcl) in enumerate(show):
        print('  [%d] ncl=%d tcl=%d' % (i, ncl, tcl))
    if len(ext) > 24:
        print('  ... %d more' % (len(ext) - len(show)))
    return ext


def run_sim(chain, size, ss, spc, path):
    ext = print_info(path, chain, size, spc, ss)
    if not ext:
        return
    old_m, old_s = simulate_read(chain, size, ss, spc, contiguous_sect_old)
    new_m, new_s = simulate_read(chain, size, ss, spc, contiguous_sect_new)
    print('  sim old contiguous_sect: %d mismatches / %d steps' % (old_m, old_s))
    print('  sim new contiguous_sect: %d mismatches / %d steps' % (new_m, new_s))


def selftest():
    ss = 512
    csize = 64
    n = 140
    base = 1000
    contig = list(range(base, base + n))
    reverse = make_pattern(contig, 'reverse', 14)
    scatter = make_pattern(contig, 'scatter', 14)
    zebra = make_pattern(contig, 'zebra', 14)
    forward = make_pattern(contig, 'forward', 8)
    fsize = n * csize * ss
    cases = [
        ('contig', contig),
        ('reverse-14', reverse),
        ('zebra-14', zebra),
        ('forward-8', forward),
        ('scatter', scatter),
    ]
    failed = 0
    for name, chain in cases:
        old_m, _ = simulate_read(chain, fsize, ss, csize, contiguous_sect_old)
        new_m, _ = simulate_read(chain, fsize, ss, csize, contiguous_sect_new)
        ext = extents_of(chain)
        print('%s: extents=%d cltbl[0]=%d old_mismatch=%d new_mismatch=%d' % (
            name, len(ext), clmt_ulen(ext), old_m, new_m))
        if new_m != 0:
            failed += 1
            print('  FAIL new algorithm')
        if name == 'contig' and old_m != 0:
            failed += 1
            print('  FAIL old should be fine on contig')
        if name != 'contig' and len(ext) > 1 and old_m == 0:
            print('  note: old algorithm happened to match')
    if failed:
        die('selftest failed')
    print('selftest ok')


def main():
    ap = argparse.ArgumentParser(description='Fragment or inspect a file on a FAT32 IDE image.')
    ap.add_argument('cmd', choices=('info', 'sim', 'fragment', 'selftest'))
    ap.add_argument('image', nargs='?', help='raw MBR+FAT32 image (DS.img)')
    ap.add_argument('path', nargs='?', help='path inside the image, e.g. DS/games/foo/foo.dni')
    ap.add_argument('-p', '--pattern', default='reverse',
                    choices=('reverse', 'forward', 'zebra', 'scatter', 'restore'))
    ap.add_argument('-n', '--pieces', type=int, default=14)
    ap.add_argument('-a', '--all-dni', action='store_true')
    args = ap.parse_args()

    if args.cmd == 'selftest':
        selftest()
        return

    if not args.image:
        die('image path required')
    writable = args.cmd == 'fragment'
    fs = Fat32(args.image, writable)
    try:
        if args.all_dni or not args.path:
            files = fs.list_files(fs.root, '')
            files = [x for x in files if x[0].lower().endswith('.dni')]
            if not files:
                die('no .dni files found')
            for p, e in files:
                chain = fs.walk_chain(e['cluster']) if e['cluster'] else []
                if args.cmd == 'sim':
                    run_sim(chain, e['size'], SECTOR, fs.spc, p)
                else:
                    print_info(p, chain, e['size'], fs.spc, SECTOR)
            return
        ent = fs.find_path(args.path)
        if ent is None:
            die('not found: %s' % args.path)
        if ent['dir']:
            die('%s is a directory' % args.path)
        chain = fs.walk_chain(ent['cluster']) if ent['cluster'] else []
        if args.cmd == 'info':
            print_info(args.path, chain, ent['size'], fs.spc, SECTOR)
            return
        if args.cmd == 'sim':
            run_sim(chain, ent['size'], SECTOR, fs.spc, args.path)
            return
        new_chain = make_pattern(chain, args.pattern, args.pieces)
        if new_chain == chain:
            print('already matches pattern, nothing to do')
            return
        print('before:')
        print_info(args.path, chain, ent['size'], fs.spc, SECTOR)
        fs.permute_file(ent, new_chain)
        chain = fs.walk_chain(ent['cluster'])
        print('after %s n=%d:' % (args.pattern, args.pieces))
        run_sim(chain, ent['size'], SECTOR, fs.spc, args.path)
    finally:
        fs.close()


if __name__ == '__main__':
    main()
