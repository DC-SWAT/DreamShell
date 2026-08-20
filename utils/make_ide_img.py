#!/usr/bin/env python3
#
# DreamShell IDE/CF raw FAT32 image builder
# Copyright (C) 2026 SWAT
# http://www.dc-swat.ru
#

import argparse
import os
import struct
import sys
import time

SECTOR = 512
PART_START = 2048
MIN_FAT32 = 65526
EOC = 0x0FFFFFFF
ATTR_VOLUME = 0x08
ATTR_DIR = 0x10
ATTR_ARCH = 0x20
ATTR_LFN = 0x0F
SKIP_NAMES = {'.DS_Store', 'Thumbs.db', '.git', '.svn', '.gitignore'}


def die(msg):
    sys.stderr.write('Error: %s\n' % msg)
    sys.exit(1)


def skip_name(name):
    if name in SKIP_NAMES:
        return True
    if name.startswith('.'):
        return True
    return False


def fat_time(ts):
    tm = time.localtime(ts)
    year = tm.tm_year
    if year < 1980:
        year = 1980
    if year > 2107:
        year = 2107
    date = ((year - 1980) << 9) | (tm.tm_mon << 5) | tm.tm_mday
    tval = (tm.tm_hour << 11) | (tm.tm_min << 5) | (tm.tm_sec // 2)
    tenths = (tm.tm_sec % 2) * 100
    return date, tval, tenths


def lfn_checksum(name11):
    s = 0
    for b in name11:
        s = ((s & 1) << 7) + (s >> 1) + b
        s &= 0xFF
    return s


def sanitize_83_char(c):
    c = c.upper()
    if c in '+,;=[]*?<>|"\\/: ' or c == '.' or ord(c) < 0x20 or ord(c) > 0x7E:
        return '_'
    return c


def split_name(name):
    i = name.rfind('.')
    if i <= 0:
        return name, ''
    return name[:i], name[i + 1:]


def make_short_name(name, used):
    stem, ext = split_name(name)
    stem_s = ''.join(sanitize_83_char(c) for c in stem)
    ext_s = ''.join(sanitize_83_char(c) for c in ext)[:3]
    if not stem_s:
        stem_s = 'FILE'
    if len(stem_s) <= 8:
        short = '%-8s%-3s' % (stem_s[:8], ext_s)
        if short not in used:
            used.add(short)
            shown = stem_s[:8] + (('.' + ext_s) if ext_s else '')
            return short.encode('ascii'), (name != shown)
    n = 1
    while True:
        tail = '~%d' % n
        head = stem_s[:max(1, 8 - len(tail))]
        short = '%-8s%-3s' % ((head + tail)[:8], ext_s)
        if short not in used:
            used.add(short)
            return short.encode('ascii'), True
        n += 1
        if n > 999999:
            die('too many 8.3 name collisions for %s' % name)


def lfn_count(name):
    return (len(name) + 1 + 12) // 13


def lfn_entries(name, name11):
    chk = lfn_checksum(name11)
    u = name.encode('utf-16le')
    chars = [u[i] | (u[i + 1] << 8) for i in range(0, len(u), 2)]
    chars.append(0)
    nent = (len(chars) + 12) // 13
    out = []
    for seq in range(nent, 0, -1):
        chunk = chars[(seq - 1) * 13:seq * 13]
        while len(chunk) < 13:
            chunk.append(0xFFFF)
        rec = bytearray(32)
        rec[0] = seq | (0x40 if seq == nent else 0)
        rec[11] = ATTR_LFN
        rec[13] = chk
        def put(off, vals):
            for i, v in enumerate(vals):
                rec[off + i * 2] = v & 0xFF
                rec[off + i * 2 + 1] = (v >> 8) & 0xFF
        put(1, chunk[0:5])
        put(14, chunk[5:11])
        put(28, chunk[11:13])
        out.append(bytes(rec))
    return out


def dirent(name11, attr, cluster, size, ts):
    date, tval, tenths = fat_time(ts)
    rec = bytearray(32)
    rec[0:11] = name11
    rec[11] = attr
    rec[13] = tenths
    rec[14:16] = struct.pack('<H', tval)
    rec[16:18] = struct.pack('<H', date)
    rec[18:20] = struct.pack('<H', date)
    rec[20:22] = struct.pack('<H', (cluster >> 16) & 0xFFFF)
    rec[22:24] = struct.pack('<H', tval)
    rec[24:26] = struct.pack('<H', date)
    rec[26:28] = struct.pack('<H', cluster & 0xFFFF)
    rec[28:32] = struct.pack('<I', size & 0xFFFFFFFF)
    return bytes(rec)


def volume_entry(label, ts):
    lab = (label.upper() + '           ')[:11]
    name11 = ''.join(c if 0x20 <= ord(c) <= 0x7E else '_' for c in lab).encode('ascii')
    return dirent(name11, ATTR_VOLUME, 0, 0, ts)


def layout_fat32(part_sectors):
    reserved = 32
    num_fats = 2
    for spc in (64, 32, 16, 8, 4, 2, 1):
        fat_sectors = 1
        nclst = 0
        for _ in range(16):
            data_sectors = part_sectors - reserved - num_fats * fat_sectors
            if data_sectors <= 0:
                nclst = 0
                break
            nclst = data_sectors // spc
            need = ((nclst + 2) * 4 + SECTOR - 1) // SECTOR
            if need == fat_sectors:
                break
            fat_sectors = need
        if nclst >= MIN_FAT32:
            data_sectors = part_sectors - reserved - num_fats * fat_sectors
            nclst = data_sectors // spc
            return spc, reserved, fat_sectors, nclst
    die('image is too small for FAT32 (increase --size)')


def chs_from_lba(lba, heads=255, spt=63):
    cyl = lba // (heads * spt)
    tmp = lba % (heads * spt)
    head = tmp // spt
    sec = (tmp % spt) + 1
    if cyl > 1023:
        cyl = 1023
        head = heads - 1
        sec = spt
    return head & 0xFF, ((cyl >> 2) & 0xC0) | (sec & 0x3F), cyl & 0xFF


def auto_size_bytes(content_bytes):
    need = content_bytes + content_bytes // 4 + 32 * 1024 * 1024
    mb = (need + 1024 * 1024 - 1) // (1024 * 1024)
    if mb < 64:
        mb = 64
    mb = (mb + 15) // 16 * 16
    return mb * 1024 * 1024


class Fat32Image(object):
    def __init__(self, path, size, label, ts):
        if size % SECTOR:
            die('image size must be a multiple of 512')
        self.path = path
        self.size = size
        self.total_sectors = size // SECTOR
        if self.total_sectors <= PART_START + 2048:
            die('image is too small')
        self.part_sectors = self.total_sectors - PART_START
        self.spc, self.reserved, self.fat_sectors, self.nclst = layout_fat32(self.part_sectors)
        self.cluster_bytes = self.spc * SECTOR
        self.label = label
        self.ts = ts
        self.fat_start = PART_START + self.reserved
        self.data_start = self.fat_start + self.fat_sectors * 2
        self.max_cluster = self.nclst + 1
        self.next_cluster = 3
        self.fat = [0] * (self.nclst + 2)
        self.fat[0] = 0xFFFFFFF8
        self.fat[1] = 0xFFFFFFFF
        self.fat[2] = EOC
        self.fp = open(path, 'wb')
        self.fp.seek(size - 1)
        self.fp.write(b'\x00')

    def close(self):
        if self.fp:
            self.fp.close()
            self.fp = None

    def write_lba(self, lba, data):
        self.fp.seek(lba * SECTOR)
        self.fp.write(data)

    def alloc_chain(self, nbytes):
        if nbytes <= 0:
            return 0, 0
        n = (nbytes + self.cluster_bytes - 1) // self.cluster_bytes
        if n < 1:
            n = 1
        if self.next_cluster + n - 1 > self.max_cluster:
            die('image is full, increase --size (need more than %d MB)' % (self.size // (1024 * 1024)))
        first = self.next_cluster
        for i in range(n):
            cl = first + i
            if i + 1 < n:
                self.fat[cl] = cl + 1
            else:
                self.fat[cl] = EOC
        self.next_cluster = first + n
        return first, n

    def alloc_root(self, nbytes):
        n = (nbytes + self.cluster_bytes - 1) // self.cluster_bytes
        if n < 1:
            n = 1
        if n > 1:
            extra, _ = self.alloc_chain((n - 1) * self.cluster_bytes)
            self.fat[2] = extra
        else:
            self.fat[2] = EOC
        return 2, n

    def cluster_lba(self, cl):
        return self.data_start + (cl - 2) * self.spc

    def write_chain(self, first, data):
        cl = first
        off = 0
        while cl >= 2:
            chunk = data[off:off + self.cluster_bytes]
            if len(chunk) < self.cluster_bytes:
                chunk = chunk + b'\x00' * (self.cluster_bytes - len(chunk))
            self.write_lba(self.cluster_lba(cl), chunk)
            off += self.cluster_bytes
            nxt = self.fat[cl] & 0x0FFFFFFF
            if nxt >= 0x0FFFFFF8:
                break
            cl = nxt

    def write_file_chain(self, src, first, size):
        cl = first
        left = size
        with open(src, 'rb') as inf:
            while cl >= 2 and left > 0:
                n = min(self.cluster_bytes, left)
                chunk = inf.read(n)
                if len(chunk) < self.cluster_bytes:
                    chunk = chunk + b'\x00' * (self.cluster_bytes - len(chunk))
                self.write_lba(self.cluster_lba(cl), chunk)
                left -= n
                nxt = self.fat[cl] & 0x0FFFFFFF
                if nxt >= 0x0FFFFFF8:
                    break
                cl = nxt

    def write_mbr(self):
        mbr = bytearray(SECTOR)
        mbr[0:3] = b'\xEB\xFE\x90'
        ent = bytearray(16)
        sh, ss, sc = chs_from_lba(PART_START)
        eh, es, ec = chs_from_lba(PART_START + self.part_sectors - 1)
        ent[0] = 0x80
        ent[1] = sh
        ent[2] = ss
        ent[3] = sc
        ent[4] = 0x0C
        ent[5] = eh
        ent[6] = es
        ent[7] = ec
        ent[8:12] = struct.pack('<I', PART_START)
        ent[12:16] = struct.pack('<I', self.part_sectors)
        mbr[0x1BE:0x1CE] = ent
        mbr[510] = 0x55
        mbr[511] = 0xAA
        self.write_lba(0, bytes(mbr))

    def write_vbr(self):
        vbr = bytearray(SECTOR)
        vbr[0:11] = b'\xEB\xFE\x90MSDOS5.0'
        vbr[11:13] = struct.pack('<H', SECTOR)
        vbr[13] = self.spc
        vbr[14:16] = struct.pack('<H', self.reserved)
        vbr[16] = 2
        vbr[21] = 0xF8
        vbr[24:26] = struct.pack('<H', 63)
        vbr[26:28] = struct.pack('<H', 255)
        vbr[28:32] = struct.pack('<I', PART_START)
        vbr[32:36] = struct.pack('<I', self.part_sectors)
        vbr[36:40] = struct.pack('<I', self.fat_sectors)
        vbr[44:48] = struct.pack('<I', 2)
        vbr[48:50] = struct.pack('<H', 1)
        vbr[50:52] = struct.pack('<H', 6)
        vbr[64] = 0x80
        vbr[66] = 0x29
        vbr[67:71] = struct.pack('<I', int(self.ts) & 0xFFFFFFFF)
        lab = (self.label.upper() + '           ')[:11]
        vbr[71:82] = lab.encode('ascii', 'replace')
        vbr[82:90] = b'FAT32   '
        vbr[510] = 0x55
        vbr[511] = 0xAA
        self.write_lba(PART_START, bytes(vbr))
        self.write_lba(PART_START + 6, bytes(vbr))

    def write_fsinfo(self):
        used = 0
        for cl in range(2, self.nclst + 2):
            if self.fat[cl]:
                used += 1
        free = self.nclst - used
        nxt = self.next_cluster if self.next_cluster <= self.max_cluster else 0xFFFFFFFF
        info = bytearray(SECTOR)
        info[0:4] = struct.pack('<I', 0x41615252)
        info[484:488] = struct.pack('<I', 0x61417272)
        info[488:492] = struct.pack('<I', free & 0xFFFFFFFF)
        info[492:496] = struct.pack('<I', nxt & 0xFFFFFFFF)
        info[510] = 0x55
        info[511] = 0xAA
        self.write_lba(PART_START + 1, bytes(info))
        self.write_lba(PART_START + 7, bytes(info))

    def write_fats(self):
        blob = bytearray(self.fat_sectors * SECTOR)
        for i, v in enumerate(self.fat):
            blob[i * 4:i * 4 + 4] = struct.pack('<I', v & 0xFFFFFFFF)
        self.write_lba(self.fat_start, bytes(blob))
        self.write_lba(self.fat_start + self.fat_sectors, bytes(blob))

    def finalize(self):
        self.write_mbr()
        self.write_vbr()
        self.write_fsinfo()
        self.write_fats()
        self.fp.flush()
        self.close()


class DirObj(object):
    def __init__(self, name, parent, ts):
        self.name = name
        self.parent = parent
        self.ts = ts
        self.dirs = []
        self.files = []
        self.first = 0

    def add_dir(self, name, ts):
        d = DirObj(name, self, ts)
        self.dirs.append(d)
        return d

    def add_file(self, name, src, size, ts):
        self.files.append([name, src, size, ts, 0])


def dir_entry_bytes(d, label, is_root):
    n = 0
    if not is_root:
        n += 2
    elif label:
        n += 1
    used = set()
    for sub in d.dirs:
        name11, need = make_short_name(sub.name, used)
        if need:
            n += lfn_count(sub.name)
        n += 1
    for item in d.files:
        name11, need = make_short_name(item[0], used)
        if need:
            n += lfn_count(item[0])
        n += 1
    return n * 32


def build_entries(d, label, is_root):
    entries = []
    used = set()
    if not is_root:
        entries.append(dirent(b'.          ', ATTR_DIR, d.first, 0, d.ts))
        pcl = d.parent.first if d.parent.parent is not None else 0
        entries.append(dirent(b'..         ', ATTR_DIR, pcl, 0, d.ts))
    elif label:
        entries.append(volume_entry(label, d.ts))
    for sub in d.dirs:
        name11, need = make_short_name(sub.name, used)
        if need:
            entries.extend(lfn_entries(sub.name, name11))
        entries.append(dirent(name11, ATTR_DIR, sub.first, 0, sub.ts))
    for item in d.files:
        name, src, size, ts, first = item
        name11, need = make_short_name(name, used)
        if need:
            entries.extend(lfn_entries(name, name11))
        entries.append(dirent(name11, ATTR_ARCH, first, size, ts))
    return b''.join(entries)


def collect_tree(src_dir, stats, prefix):
    src_dir = os.path.abspath(src_dir)
    if not os.path.isdir(src_dir):
        die('source directory not found: %s' % src_dir)
    now = time.time()
    root = DirObj('', None, now)
    dest_root = root
    if prefix:
        parts = [p for p in prefix.replace('\\', '/').split('/') if p and p != '.']
        cur = root
        for p in parts:
            cur = cur.add_dir(p, now)
        dest_root = cur
    mapping = {src_dir: dest_root}
    nfiles = 0
    nbytes = 0
    ndirs = 0
    for dirpath, dirnames, filenames in os.walk(src_dir):
        dirnames[:] = sorted(n for n in dirnames if not skip_name(n))
        parent = mapping[dirpath]
        parent.ts = os.stat(dirpath).st_mtime
        for name in dirnames:
            p = os.path.join(dirpath, name)
            mapping[p] = parent.add_dir(name, os.stat(p).st_mtime)
            ndirs += 1
        for name in sorted(filenames):
            if skip_name(name):
                continue
            p = os.path.join(dirpath, name)
            if not os.path.isfile(p):
                continue
            st = os.stat(p)
            parent.add_file(name, p, st.st_size, st.st_mtime)
            nfiles += 1
            nbytes += st.st_size
    stats['nfiles'] = nfiles
    stats['nbytes'] = nbytes
    stats['ndirs'] = ndirs
    return root


def write_files(d, img):
    for item in d.files:
        size = item[2]
        if size > 0:
            first, _ = img.alloc_chain(size)
            img.write_file_chain(item[1], first, size)
            item[4] = first
    for sub in d.dirs:
        write_files(sub, img)


def alloc_dirs(d, img, label, is_root):
    nbytes = dir_entry_bytes(d, label, is_root)
    if is_root:
        d.first, _ = img.alloc_root(max(nbytes, 1))
    else:
        d.first, _ = img.alloc_chain(max(nbytes, 1))
    for sub in d.dirs:
        alloc_dirs(sub, img, label, False)


def write_dirs(d, img, label, is_root):
    blob = build_entries(d, label, is_root)
    img.write_chain(d.first, blob)
    for sub in d.dirs:
        write_dirs(sub, img, label, False)


def content_bytes(src_dir):
    total = 0
    for dirpath, dirnames, filenames in os.walk(src_dir):
        dirnames[:] = [n for n in dirnames if not skip_name(n)]
        for name in filenames:
            if skip_name(name):
                continue
            p = os.path.join(dirpath, name)
            if os.path.isfile(p):
                total += os.path.getsize(p)
    return total


def main():
    ap = argparse.ArgumentParser(description='Create a raw MBR+FAT32 disk image for DreamShell IDE/CF.')
    ap.add_argument('src', help='source directory (DS build folder)')
    ap.add_argument('-o', '--output', default='DS.img', help='output raw image path')
    ap.add_argument('-s', '--size', default='0', help='image size in MB (0 = auto)')
    ap.add_argument('-l', '--label', default='DREAMSHELL', help='FAT volume label')
    ap.add_argument('-p', '--prefix', default='DS', help='directory on the image (default: DS)')
    args = ap.parse_args()

    src = args.src
    if not os.path.isdir(src):
        die('source directory not found: %s' % src)

    cbytes = content_bytes(src)
    try:
        size_mb = int(str(args.size).lower().replace('mb', '').strip())
    except ValueError:
        die('invalid --size: %s' % args.size)

    if size_mb <= 0:
        size = auto_size_bytes(cbytes)
    else:
        size = size_mb * 1024 * 1024
        if size < cbytes + 8 * 1024 * 1024:
            die('size %d MB is too small for %d MB of files' % (
                size_mb, (cbytes + 1024 * 1024 - 1) // (1024 * 1024)))
        if size < 34 * 1024 * 1024:
            die('FAT32 needs at least 34 MB')

    label = (args.label.strip() or 'DREAMSHELL')[:11]
    ts = time.time()
    stats = {}
    root = collect_tree(src, stats, args.prefix)

    print('Creating %d MB FAT32 image: %s' % (size // (1024 * 1024), args.output))
    img = Fat32Image(args.output, size, label, ts)
    if img.cluster_bytes >= 1024:
        clus = '%d KB' % (img.cluster_bytes // 1024)
    else:
        clus = '%d bytes' % img.cluster_bytes
    print('Partition: LBA %d, %s clusters, %d clusters' % (
        PART_START, clus, img.nclst))
    print('Copying %d files, %d directories (%d MB)...' % (
        stats['nfiles'], stats['ndirs'],
        (stats['nbytes'] + 1024 * 1024 - 1) // (1024 * 1024)))
    try:
        write_files(root, img)
        alloc_dirs(root, img, label, True)
        write_dirs(root, img, label, True)
        img.finalize()
    except Exception:
        img.close()
        raise
    print('IDE image ready: %s' % args.output)


if __name__ == '__main__':
    main()
