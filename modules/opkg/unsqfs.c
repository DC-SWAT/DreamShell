/*
 * Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011
 * Phillip Lougher <phillip@lougher.demon.co.uk>
 *
 * Copyright (c) 2010 LG Electronics
 * Chan Jeong <chan.jeong@lge.com>
 *
 * Copyright (c) 2012 Reality Diluted, LLC
 * Steven J. Hill <sjhill@realitydiluted.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * unsqfs.c
 *
 * Unsquash a squashfs filesystem with minimal support. This code is
 * for little endian only, ignores uid/gid, ignores xattr, only works
 * for squashfs version >4.0, only supports zlib and lzo compression,
 * is only for Linux, is not multi-threaded and does not support any
 * regular expressions. You have been warned.
 *    -Steve
 *
 * To build as a part of a library or application compile this file
 * and link with the following CFLAGS and LDFLAGS:
 *
 *    CFLAGS += -O2 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
 *    LDFLAGS += -lz -llzo2
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <fnmatch.h>
#include <sys/types.h>
#include <sys/stat.h>
//#include <sys/mman.h>

#if USE_GZIP
#include <zlib/zlib.h>
#endif
#if USE_LZO
#include <minilzo/minilzo.h>
#endif

#include "unsqfs.h"


#if !defined(static_assert)
#if __STDC_VERSION__ >= 201112L
// glibc prior to 2.16 and uClibc lack this define
#define static_assert _Static_assert
#else
//#warning Compiling without static asserts
#define static_assert(e, m)
#endif
#endif

//#define IS_POWER_OF_TWO(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)


#define SQUASHFS_MAGIC			0x73717368
#define SQUASHFS_START			0

/* size of metadata (inode and directory) blocks */
#define SQUASHFS_METADATA_SIZE		8192

/* default size of data blocks */
#define SQUASHFS_FILE_SIZE		131072
#define SQUASHFS_FILE_MAX_SIZE		1048576

/* Max length of filename (not 255) */
#define SQUASHFS_NAME_LEN		256

#define SQUASHFS_INVALID_FRAG		((unsigned int) 0xffffffff)

/* Max number of types and file types */
#define SQUASHFS_DIR_TYPE		1
#define SQUASHFS_REG_TYPE		2
#define SQUASHFS_LDIR_TYPE		8
#define SQUASHFS_LREG_TYPE		9

/* Flag whether block is compressed or uncompressed, bit is set if block is
 * uncompressed */
#define SQUASHFS_COMPRESSED_BIT		(1 << 15)

#define SQUASHFS_COMPRESSED_SIZE(B)	(((B) & ~SQUASHFS_COMPRESSED_BIT) ? \
		(B) & ~SQUASHFS_COMPRESSED_BIT :  SQUASHFS_COMPRESSED_BIT)

#define SQUASHFS_COMPRESSED(B)		(!((B) & SQUASHFS_COMPRESSED_BIT))

#define SQUASHFS_COMPRESSED_BIT_BLOCK		(1 << 24)

#define SQUASHFS_COMPRESSED_SIZE_BLOCK(B)	((B) & \
	~SQUASHFS_COMPRESSED_BIT_BLOCK)

#define SQUASHFS_COMPRESSED_BLOCK(B)	(!((B) & SQUASHFS_COMPRESSED_BIT_BLOCK))

/*
 * Inode number ops.  Inodes consist of a compressed block number, and an
 * uncompressed offset within that block.
 */
typedef long long squashfs_inode;
static inline unsigned int inode_block(squashfs_inode inode_addr)
{
	return (unsigned int)(inode_addr >> 16);
}
static inline unsigned short inode_offset(squashfs_inode inode_addr)
{
	return (unsigned short)(inode_addr & 0xffff);
}
static inline squashfs_inode inode_address(
		unsigned int block, unsigned short offset)
{
	return ((squashfs_inode)block << 16) | offset;
}

/*
 * definitions for structures on disk
 */

typedef long long		squashfs_block;

#define ZLIB_COMPRESSION	1
#define LZO_COMPRESSION		3

struct squashfs_super_block {
	unsigned int		s_magic;
	unsigned int		inodes;
	unsigned int		mkfs_time /* time of filesystem creation */;
	unsigned int		block_size;
	unsigned int		fragments;
	unsigned short		compression;
	unsigned short		block_log;
	unsigned short		flags;
	unsigned short		res0;
	unsigned short		s_major;
	unsigned short		s_minor;
	squashfs_inode		root_inode;
	long long		bytes_used;
	long long		res1;
	long long		res2;
	long long		inode_table_start;
	long long		directory_table_start;
	long long		fragment_table_start;
	long long		res3;
};

struct squashfs_dir_index {
	unsigned int		index;
	unsigned int		start_block;
	unsigned int		size;
	unsigned char		name[0];
};

struct squashfs_base_inode_header {
	unsigned short		inode_type;
	unsigned short		mode;
	unsigned short		uid;
	unsigned short		guid;
	unsigned int		mtime;
	unsigned int 		inode_number;
};

struct squashfs_reg_inode_header {
	unsigned short		inode_type;
	unsigned short		mode;
	unsigned short		uid;
	unsigned short		guid;
	unsigned int		mtime;
	unsigned int 		inode_number;
	unsigned int		start_block;
	unsigned int		fragment;
	unsigned int		offset;
	unsigned int		file_size;
	unsigned int		block_list[0];
};

struct squashfs_lreg_inode_header {
	unsigned short		inode_type;
	unsigned short		mode;
	unsigned short		uid;
	unsigned short		guid;
	unsigned int		mtime;
	unsigned int 		inode_number;
	squashfs_block		start_block;
	long long			file_size;
	long long			sparse;
	unsigned int		nlink;
	unsigned int		fragment;
	unsigned int		offset;
	unsigned int		xattr;
	unsigned int		block_list[0];
};

struct squashfs_dir_inode_header {
	unsigned short		inode_type;
	unsigned short		mode;
	unsigned short		uid;
	unsigned short		guid;
	unsigned int		mtime;
	unsigned int 		inode_number;
	unsigned int		start_block;
	unsigned int		nlink;
	unsigned short		file_size;
	unsigned short		offset;
	unsigned int		parent_inode;
};

struct squashfs_ldir_inode_header {
	unsigned short		inode_type;
	unsigned short		mode;
	unsigned short		uid;
	unsigned short		guid;
	unsigned int		mtime;
	unsigned int 		inode_number;
	unsigned int		nlink;
	unsigned int		file_size;
	unsigned int		start_block;
	unsigned int		parent_inode;
	unsigned short		i_count;
	unsigned short		offset;
	unsigned int		res0;
	struct squashfs_dir_index	index[0];
};

union squashfs_inode_header {
	struct squashfs_base_inode_header	base;
	struct squashfs_reg_inode_header	reg;
	struct squashfs_lreg_inode_header	lreg;
	struct squashfs_dir_inode_header	dir;
	struct squashfs_ldir_inode_header	ldir;
};

struct squashfs_dir_entry {
	unsigned short		offset;
	short			inode_number;
	unsigned short		type;
	unsigned short		size;
	char			name[0];
};

struct squashfs_dir_header {
	unsigned int		count;
	unsigned int		start_block;
	unsigned int		inode_number;
};

struct squashfs_fragment_entry {
	long long		start_block;
	unsigned int		size;
	unsigned int		unused;
};

#ifdef SQUASHFS_TRACE
#define TRACE(s, args...) \
		do { \
			printf("unsquashfs: "s, ## args); \
		} while(0)
#else
#define TRACE(s, args...)
#endif

#define ERROR(s, args...) \
		do { \
			printf("unsquashfs: "s, ## args); \
		} while(0)

#define DIR_ENT_SIZE	16

struct dir_ent {
	char name[SQUASHFS_NAME_LEN + 1];
	squashfs_inode inode_addr;
	unsigned int type;
};

struct dir {
	int dir_count;
	int cur_entry;
	struct dir_ent *dirs;
	bool is_open;
};

struct path_entry {
	char *name;
	struct pathname *paths;
};

struct pathname {
	int names;
	struct path_entry *name;
};

struct pathnames {
	int count;
	struct pathname *path[0];
};
#define PATHS_ALLOC_SIZE 10

#define HASH_TABLE_SIZE (1 << 16)
/*
static_assert(IS_POWER_OF_TWO(HASH_TABLE_SIZE),
		"size must be a power of two so we can do efficient modulo");
*/
struct metadata_table {
	struct hash_table_entry *hash_table[HASH_TABLE_SIZE];
	struct PkgData *pdata;
};

struct PkgData {
	struct squashfs_super_block sBlk;
	struct metadata_table inode_table;
	struct metadata_table directory_table;
	long long *fragment_table_index;
	struct squashfs_fragment_entry **fragment_table_blocks;

	int fd;

	struct dir dir;
};


// === Low-level I/O ===

static int read_fs_bytes(const int fd, const long long offset,
		void *buf, const size_t bytes)
{
	TRACE("read_bytes: reading from position 0x%llx, bytes %lu\n",
			offset, bytes);

	if (lseek(fd, (off_t)offset, SEEK_SET) == -1) {
		int err = -errno;
		ERROR("Error seeking in input: %s\n", strerror(errno));
		return err;
	}

	size_t count = 0;
	while (count < bytes) {
		const int res = read(fd, buf + count, bytes - count);
		if (res < 1) {
			if (res == 0) {
				ERROR("Error reading input: unexpected EOF\n");
				return -EIO;
			} else if (errno != EINTR) {
				int err = -errno;
				ERROR("Error reading input: %s\n", strerror(errno));
				return err;
			}
		} else {
			count += res;
		}
	}

	return 0;
}

static int read_compressed(const struct PkgData *pdata,
		const long long offset, const size_t csize,
		void *buf, const size_t buf_size)
{
	if (csize >= buf_size) {
		// In the case compression doesn't make a block smaller,
		// mksquashfs will store the block uncompressed.
		ERROR("Refusing to load too-large compressed block\n");
		return -EIO;
	}

	// Load compressed data into temporary buffer.
	char tmp[csize];
	int err = read_fs_bytes(pdata->fd, offset, tmp, csize);
	if (err < 0) {
		return err;
	}

#if USE_GZIP
	if (pdata->sBlk.compression == ZLIB_COMPRESSION) {
		unsigned long bytes_zlib = buf_size;
		int error = uncompress(buf, &bytes_zlib, (const Bytef *) tmp, csize);
		if (error == Z_OK) {
			return (int) bytes_zlib;
		}

		ERROR("GZIP uncompress failed with error code %d\n", error);
		return -EIO;
	}
#endif
#if USE_LZO
	if (pdata->sBlk.compression == LZO_COMPRESSION) {
		lzo_uint bytes_lzo = buf_size;
		int error = lzo1x_decompress_safe((const lzo_bytep) tmp, csize, buf, &bytes_lzo, NULL);
		if (error == LZO_E_OK) {
			return (int) bytes_lzo;
		}

		ERROR("LZO uncompress failed with error code %d\n", error);
		return -EIO;
	}
#endif

	ERROR("Unsupported compression algorithm (id: %hu)\n",
				pdata->sBlk.compression);
	return -EINVAL;
}

static int read_uncompressed(const struct PkgData *pdata,
		const long long offset, const size_t csize,
		void *buf, const size_t buf_size)
{
	if (csize > buf_size) {
		ERROR("Refusing to load oversized uncompressed block\n");
		return -EIO;
	}

	int err = read_fs_bytes(pdata->fd, offset, buf, csize);
	if (err < 0) {
		return err;
	}

	return (int) csize;
}


// === High level I/O ===

static int read_metadata_block(const struct PkgData *pdata,
		const long long start, long long *next,
		void *buf, const size_t buf_size)
{
	long long offset = start;

	unsigned short c_byte;
	int ret = read_fs_bytes(pdata->fd, offset, &c_byte, 2);
	if (ret) {
		goto failed;
	}

	offset += 2;
	int csize = SQUASHFS_COMPRESSED_SIZE(c_byte);

	TRACE("read_metadata_block: block @0x%llx, %d %s bytes\n", start, csize,
			SQUASHFS_COMPRESSED(c_byte) ? "compressed" : "uncompressed");

	ret = SQUASHFS_COMPRESSED(c_byte)
		? read_compressed(pdata, offset, csize, buf, buf_size)
		: read_uncompressed(pdata, offset, csize, buf, buf_size);
	if (ret < 0) {
		goto failed;
	}

	offset += csize;
	if (next) *next = offset;
	return ret;

failed:
	ERROR("Failed to read metadata block @0x%llx\n", start);
	return ret;
}

static int read_data_block(const struct PkgData *pdata,
		void *buf, const size_t buf_size,
		const long long offset, const unsigned int c_byte)
{
	const size_t csize = SQUASHFS_COMPRESSED_SIZE_BLOCK(c_byte);
	return SQUASHFS_COMPRESSED_BLOCK(c_byte)
		? read_compressed(pdata, offset, csize, buf, buf_size)
		: read_uncompressed(pdata, offset, csize, buf, buf_size);
}


// === Metadata table ===

struct hash_table_entry {
	long long cstart, cnext;
	void *udata;
	size_t usize;
	struct hash_table_entry *next;
};

static int calculate_hash(long long cstart)
{
	return cstart & (HASH_TABLE_SIZE - 1);
}

static struct hash_table_entry *load_entry(struct PkgData *pdata,
		long long cstart)
{
	// Allocate space for decompressed metadata block.
	void *udata = malloc(SQUASHFS_METADATA_SIZE);
	if (!udata) {
		ERROR("Failed to allocate metadata block\n");
		return NULL;
	}

	long long cnext;
	int usize = read_metadata_block(pdata, cstart, &cnext,
			udata, SQUASHFS_METADATA_SIZE);
	if (usize < 0) {
		ERROR("Failed to read metadata block\n");
		free(udata);
		return NULL;
	}

	struct hash_table_entry *entry = malloc(sizeof(struct hash_table_entry));
	if (!entry) {
		ERROR("Failed to allocate hash table entry\n");
		free(udata);
		return NULL;
	}
	entry->cstart = cstart;
	entry->cnext = cnext;
	entry->udata = udata;
	entry->usize = usize;
	entry->next = NULL;

	return entry;
}

static const struct hash_table_entry *fetch_entry(struct metadata_table *table,
		long long cstart)
{
	struct hash_table_entry **hash_table = table->hash_table;
	const int hash = calculate_hash(cstart);
	struct hash_table_entry *entry = hash_table[hash];
	while (entry && entry->cstart != cstart) {
		entry = entry->next;
	}
	if (!entry) {
		entry = load_entry(table->pdata, cstart);
		hash_table[hash] = entry;
	}
	return entry;
}

static void free_metadata_table(struct metadata_table *table)
{
	struct hash_table_entry **hash_table = table->hash_table;
	for (unsigned int i = 0; i < HASH_TABLE_SIZE; i++) {
		struct hash_table_entry *entry = hash_table[i];
		while (entry) {
			struct hash_table_entry *next = entry->next;
			free(entry->udata);
			free(entry);
			entry = next;
		}
	}
}

struct metadata_accessor {
	struct metadata_table *table;
	const struct hash_table_entry *entry;
	unsigned short offset;
};

static bool init_metadata_accessor(
		struct metadata_accessor *accessor,
		struct metadata_table *table,
		squashfs_block start_block, unsigned short offset)
{
	const struct hash_table_entry *entry = fetch_entry(table, start_block);
	if (!entry) {
		ERROR("Table block %lld not found\n", start_block);
		return false;
	}
	accessor->table = table;
	accessor->entry = entry;
	accessor->offset = offset;
	return true;
}

static bool read_metadata(
		struct metadata_accessor *accessor,
		void *dest, size_t num_bytes)
{
	const struct hash_table_entry *entry = accessor->entry;
	unsigned int offset = accessor->offset;

	while (num_bytes != 0) {
		// Copy bytes from current block.
		size_t step_bytes = offset + num_bytes <= entry->usize
				? num_bytes : entry->usize - offset;
		memcpy(dest, entry->udata + offset, step_bytes);
		dest += step_bytes;
		offset += step_bytes;
		num_bytes -= step_bytes;

		if (num_bytes != 0) {
			// Next block.
			long long start_block = entry->cnext;
			entry = fetch_entry(accessor->table, start_block);
			accessor->entry = entry;
			if (!entry) {
				ERROR("Table block %lld not found\n", start_block);
				return false;
			}
			offset = 0;
		}
	}

	accessor->offset = offset;
	return true;
}


// === Inodes ===

struct inode {
	int num_blocks;
	struct metadata_accessor accessor;
	long long file_size;
	int fragment;
	int frag_bytes;
	int offset; // file: offset in fragment block
	            // dir:  offset in directory block
	long long start; // file: compressed block start address
	                 // dir:  offset of directory block in directory table
};

static bool read_inode(struct PkgData *pdata,
		squashfs_inode inode_addr, struct inode *i)
{
	TRACE("read_inode: reading inode %012llX\n", inode_addr);

	if (!init_metadata_accessor(&i->accessor, &pdata->inode_table,
			pdata->sBlk.inode_table_start + inode_block(inode_addr),
			inode_offset(inode_addr))) {
		return false;
	}

	union squashfs_inode_header header;
	void *header_ptr = &header.base;
	if (!read_metadata(&i->accessor, header_ptr, sizeof(header.base))) {
		return false;
	}
	header_ptr += sizeof(header.base);

	switch(header.base.inode_type) {
		case SQUASHFS_DIR_TYPE: {
			struct squashfs_dir_inode_header *inode = &header.dir;

			if (!read_metadata(&i->accessor, header_ptr,
					sizeof(*inode) - sizeof(header.base))) {
				return false;
			}

			i->file_size = inode->file_size;
			i->offset = inode->offset;
			i->start = inode->start_block;
			break;
		}
		case SQUASHFS_LDIR_TYPE: {
			struct squashfs_ldir_inode_header *inode = &header.ldir;

			if (!read_metadata(&i->accessor, header_ptr,
					sizeof(*inode) - sizeof(header.base))) {
				return false;
			}

			i->file_size = inode->file_size;
			i->offset = inode->offset;
			i->start = inode->start_block;
			break;
		}
		case SQUASHFS_REG_TYPE: {
			struct squashfs_reg_inode_header *inode = &header.reg;

			if (!read_metadata(&i->accessor, header_ptr,
					sizeof(*inode) - sizeof(header.base))) {
				return false;
			}

			const bool has_fragment = inode->fragment != SQUASHFS_INVALID_FRAG;
			i->file_size = inode->file_size;
			i->frag_bytes = has_fragment
				? inode->file_size % pdata->sBlk.block_size
				: 0;
			i->fragment = inode->fragment;
			i->offset = inode->offset;
			i->num_blocks = (inode->file_size
				+ (has_fragment ? 0 : pdata->sBlk.block_size - 1)
				) >> pdata->sBlk.block_log;
			i->start = inode->start_block;
			break;
		}
		case SQUASHFS_LREG_TYPE: {
			struct squashfs_lreg_inode_header *inode = &header.lreg;

			if (!read_metadata(&i->accessor, header_ptr,
					sizeof(*inode) - sizeof(header.base))) {
				return false;
			}

			const bool has_fragment = inode->fragment != SQUASHFS_INVALID_FRAG;
			i->file_size = inode->file_size;
			i->frag_bytes = has_fragment
				? inode->file_size % pdata->sBlk.block_size
				: 0;
			i->fragment = inode->fragment;
			i->offset = inode->offset;
			i->num_blocks = (inode->file_size
				+ (has_fragment ? 0 : pdata->sBlk.block_size - 1)
				) >> pdata->sBlk.block_log;
			i->start = inode->start_block;
			break;
		}
		default:
			TRACE("read_inode: skipping inode type %d\n",
					header.base.inode_type);
			return false;
	}
	return true;
}


// === Directories ===

static bool squashfs_opendir(struct PkgData *pdata,
		squashfs_inode inode_addr, struct dir *dir)
{
	TRACE("squashfs_opendir: inode %012llX\n", inode_addr);

	struct inode i;
	if (!read_inode(pdata, inode_addr, &i)) {
		ERROR("Failed to read directory inode %012llX\n", inode_addr);
		return false;
	}

	struct metadata_accessor accessor;
	if (!init_metadata_accessor(&accessor, &pdata->directory_table,
			pdata->sBlk.directory_table_start + i.start, i.offset)) {
		return false;
	}

	dir->dir_count = 0;
	dir->cur_entry = 0;
	dir->dirs = NULL;

	char buffer[sizeof(struct squashfs_dir_entry) + SQUASHFS_NAME_LEN + 1]
		__attribute__((aligned));
	struct squashfs_dir_entry *dire = (struct squashfs_dir_entry *) buffer;

	int remaining = i.file_size - 3;
	while (remaining > 0) {
		struct squashfs_dir_header dirh;
		if (!read_metadata(&accessor, &dirh, sizeof(dirh))) {
			return false;
		}
		remaining -= sizeof(dirh);

		int dir_count = dirh.count + 1;
		TRACE("squashfs_opendir: Read directory header, %d directory entries\n",
				dir_count);

		while(dir_count--) {
			if (!read_metadata(&accessor, dire, sizeof(*dire))) {
				return false;
			}
			if (!read_metadata(&accessor, dire->name, dire->size + 1)) {
				return false;
			}
			dire->name[dire->size + 1] = '\0';
			remaining -= sizeof(*dire) + dire->size + 1;
			TRACE("squashfs_opendir: directory entry %s, inode "
				"%d:%d, type %d\n", dire->name,
				dirh.start_block, dire->offset, dire->type);
			if((dir->dir_count % DIR_ENT_SIZE) == 0) {
				struct dir_ent *new_dir = realloc(dir->dirs,
						(dir->dir_count + DIR_ENT_SIZE)
						* sizeof(struct dir_ent));
				if (!new_dir) {
					ERROR("Failed to (re)allocate directory contents\n");
					return false;
				}
				dir->dirs = new_dir;
			}
			strcpy(dir->dirs[dir->dir_count].name, dire->name);
			dir->dirs[dir->dir_count].inode_addr =
					inode_address(dirh.start_block, dire->offset);
			dir->dirs[dir->dir_count].type = dire->type;
			dir->dir_count ++;
		}
	}

	dir->is_open = true;
	return true;
}

static struct dir_ent *squashfs_dir_next(struct dir *dir)
{
	if (dir->cur_entry == dir->dir_count) {
		return NULL;
	} else {
		return &dir->dirs[dir->cur_entry++];
	}
}

static void squashfs_closedir(struct dir *dir)
{
	free(dir->dirs);
	dir->dirs = NULL;
	dir->is_open = false;
}


// === File contents ===

// Every fragment entry is located in exactly one metadata block.
/*
static_assert(IS_POWER_OF_TWO(sizeof(struct squashfs_fragment_entry)),
		"unexpected fragment entry size");
static_assert(IS_POWER_OF_TWO(SQUASHFS_METADATA_SIZE),
		"unexpected metadata size");
static_assert(
		sizeof(struct squashfs_fragment_entry) <= SQUASHFS_METADATA_SIZE,
		"fragment entry larger than metadata block");*/
static const unsigned int entries_per_block =
		SQUASHFS_METADATA_SIZE / sizeof(struct squashfs_fragment_entry);

static unsigned int get_num_fragment_table_blocks(struct PkgData *pdata) {
	return (pdata->sBlk.fragments + entries_per_block - 1) / entries_per_block;
}

static long long *read_fragment_table_index(struct PkgData *pdata)
{
	const unsigned int num_table_blocks = get_num_fragment_table_blocks(pdata);

	TRACE("read_fragment_index: %d fragments, table spans %u metadata blocks, "
			"index at 0x%llx\n", pdata->sBlk.fragments, num_table_blocks,
			pdata->sBlk.fragment_table_start);

	const size_t index_size = num_table_blocks * sizeof(long long);
	long long *index = malloc(index_size);
	if (read_fs_bytes(pdata->fd, pdata->sBlk.fragment_table_start,
			index, index_size) < 0) {
		ERROR("Failed to read fragment table index\n");
		free(index);
		return NULL;
	}

	return index;
}

static const struct squashfs_fragment_entry *fetch_fragment_entry(
		struct PkgData *pdata, int fragment)
{
	// Sanity check on fragment number.
	if ((unsigned int)fragment >= pdata->sBlk.fragments) {
		ERROR("Fragment out of range: %d of %u\n",
				fragment, pdata->sBlk.fragments);
		return NULL;
	}

	// Compute location of fragment info in fragment table.
	const unsigned int block_nr  = fragment / entries_per_block;
	const unsigned int block_idx = fragment % entries_per_block;

	// Check if relevant fragment table block is cached.
	struct squashfs_fragment_entry **blocks = pdata->fragment_table_blocks;
	if (blocks) {
		// Cache exists.
		if (blocks[block_nr]) {
			return &blocks[block_nr][block_idx];
		}
	} else {
		// Create empty cache.
		const unsigned int num_table_blocks =
				get_num_fragment_table_blocks(pdata);
		if (!(blocks = calloc(num_table_blocks, sizeof(*blocks)))) {
			ERROR("Failed to allocate fragment table block cache "
					"of %u blocks\n", num_table_blocks);
			return NULL;
		}
		pdata->fragment_table_blocks = blocks;
	}

	// Read fragment table index.
	long long *index = pdata->fragment_table_index;
	if (!index) {
		if (!(index = read_fragment_table_index(pdata))) {
			return NULL;
		}
		pdata->fragment_table_index = index;
	}

	// Allocate one fragment table block.
	const size_t block_size =
			  block_nr < pdata->sBlk.fragments / entries_per_block
			? SQUASHFS_METADATA_SIZE
			: (pdata->sBlk.fragments % entries_per_block)
				* sizeof(struct squashfs_fragment_entry);
	struct squashfs_fragment_entry *table_block;
	if (!(table_block = malloc(block_size))) {
		ERROR("Failed to allocate fragment table\n");
		return NULL;
	}

	// Load fragment table block.
	int length = read_metadata_block(pdata, index[block_nr], NULL,
			table_block, block_size);
	TRACE("Read fragment table block %u, from 0x%llx, length %d\n",
			block_nr, index[block_nr], length);
	if (length < 0) {
		ERROR("Failed to read fragment table block %u\n", block_nr);
		free(table_block);
		return NULL;
	} else if ((size_t)length != block_size) {
		ERROR("Bad length reading fragment table block %u: "
				"expected %zu, got %d\n", block_nr, block_size, length);
		free(table_block);
		return NULL;
	}

	// Insert block in cache.
	blocks[block_nr] = table_block;

	return &blocks[block_nr][block_idx];
}

static int write_buf(struct PkgData *pdata, struct inode *inode, void *buf)
{
	TRACE("write_buf: regular file, %d blocks\n", inode->num_blocks);

	const int file_end = inode->file_size / pdata->sBlk.block_size;
	long long start = inode->start;
	for (int i = 0; i < inode->num_blocks; i++) {
		int size =
			  i == file_end
			? inode->file_size & (pdata->sBlk.block_size - 1)
			: pdata->sBlk.block_size;

		unsigned int c_byte;
		if (!read_metadata(&inode->accessor, &c_byte, sizeof(c_byte))) {
			return -EIO;
		}
		if (c_byte == 0) { // sparse file
			memset(buf, 0, size);
		} else {
			const int usize = read_data_block(pdata, buf, size, start, c_byte);
			if (usize < 0) {
				return usize;
			} else if (usize != size) {
				ERROR("Error: data block contains %d bytes, expected %d\n",
						usize, size);
				return -EIO;
			}
			start += SQUASHFS_COMPRESSED_SIZE_BLOCK(c_byte);
		}
		buf += size;
	}

	if (inode->frag_bytes) {
		TRACE("read_fragment: reading fragment %d\n", inode->fragment);

		const struct squashfs_fragment_entry *fragment_entry =
				fetch_fragment_entry(pdata, inode->fragment);
		if (!fragment_entry) {
			ERROR("Failed to get info about fragment %d\n", inode->fragment);
			return -EIO;
		}

		void *data = malloc(pdata->sBlk.block_size);
		if (!data) {
			ERROR("Failed to allocate block data buffer\n");
			return -ENOMEM;
		}

		const int usize = read_data_block(pdata, data, pdata->sBlk.block_size,
				fragment_entry->start_block, fragment_entry->size);
		if (usize < 0) {
			free(data);
			return usize;
		}

		memcpy(buf, data + inode->offset, inode->frag_bytes);
		free(data);
	}

	return 0;
}


// === Public functions ===

struct PkgData *opk_sqfs_open(const char *image_name)
{
	struct PkgData *pdata = calloc(1, sizeof(*pdata));
	if (!pdata) {
		ERROR("Unable to create data structure: %s\n", strerror(errno));
		goto fail_exit;
	}
	pdata->inode_table.pdata = pdata;
	pdata->directory_table.pdata = pdata;

	if ((pdata->fd = open(image_name, O_RDONLY)) == -1) {
		ERROR("Could not open \"%s\": %s\n", image_name, strerror(errno));
		goto fail_free;
	}

	TRACE("Loading superblock...\n");
	if (read_fs_bytes(pdata->fd, SQUASHFS_START,
			&pdata->sBlk, sizeof(pdata->sBlk)) < 0) {
		ERROR("Failed to read SQUASHFS superblock on \"%s\"\n", image_name);
		goto fail_close;
	}
	if (pdata->sBlk.s_magic != SQUASHFS_MAGIC) {
		ERROR("Invalid SQUASHFS superblock on \"%s\"\n", image_name);
		goto fail_close;
	}
	if (pdata->sBlk.s_major != 4 || pdata->sBlk.s_minor != 0) {
		ERROR("Unsupported SQUASHFS version on \"%s\"\n", image_name);
		goto fail_close;
	}

	TRACE("Done opening image.\n");
	return pdata;

fail_close:
	close(pdata->fd);
fail_free:
	free(pdata);
fail_exit:
	return NULL;
}

void opk_sqfs_close(struct PkgData *pdata)
{
	squashfs_closedir(&pdata->dir);

	close(pdata->fd);

	free_metadata_table(&pdata->inode_table);
	free_metadata_table(&pdata->directory_table);
	free(pdata->fragment_table_index);
	if (pdata->fragment_table_blocks) {
		const unsigned int num_table_blocks =
				get_num_fragment_table_blocks(pdata);
		for (unsigned int i = 0; i < num_table_blocks; i++) {
			free(pdata->fragment_table_blocks[i]);
		}
		free(pdata->fragment_table_blocks);
	}
	free(pdata);
}

static int get_inode_from_dir(struct PkgData *pdata,
		const char *name, squashfs_inode inode_addr, struct inode *i)
{
	struct dir dir;
	if (!squashfs_opendir(pdata, inode_addr, &dir)) {
		return -EIO;
	}

	int found = 0;
	const char *dirsep = strchr(name, '/');
	if (dirsep) {
		// Look for subdir.
		struct dir_ent *ent;
		while ((ent = squashfs_dir_next(&dir))) {
			if (ent->type == SQUASHFS_DIR_TYPE
					|| ent->type == SQUASHFS_LDIR_TYPE) {
				if (!strncmp(ent->name, name, dirsep - name)) {
					if (!read_inode(pdata, ent->inode_addr, i)) {
						goto exit_close;
					}
					found = get_inode_from_dir(
							pdata, dirsep + 1, ent->inode_addr, i);
					goto exit_close;
				}
			}
		}
	} else {
		// Look for regular file.
		struct dir_ent *ent;
		while ((ent = squashfs_dir_next(&dir))) {
			if (ent->type == SQUASHFS_REG_TYPE
					|| ent->type == SQUASHFS_LREG_TYPE) {
				if (!strcmp(ent->name, name)) {
					found = read_inode(pdata, ent->inode_addr, i);
					goto exit_close;
				}
			}
		}
	}

exit_close:
	squashfs_closedir(&dir);
	return found;
}

int opk_sqfs_extract_file(struct PkgData *pdata, const char *name,
		void **out_data, size_t *out_size)
{
	struct inode i;
	int ret = get_inode_from_dir(pdata, name, pdata->sBlk.root_inode, &i);
	if (ret < 0) {
		return ret;
	} else if (!ret) {
		ERROR("Unable to find inode for path \"%s\"\n", name);
		return -ENOENT;
	}

	void *buf = malloc(i.file_size);
	if (!buf) {
		ERROR("Unable to allocate file extraction buffer\n");
		return -ENOMEM;
	}

	ret = write_buf(pdata, &i, buf);
	if (ret < 0) {
		free(buf);
		return ret;
	}

	*out_data = buf;
	*out_size = i.file_size;

	return 0;
}

int opk_sqfs_get_metadata(struct PkgData *pdata, const char **filename)
{
	if (!pdata->dir.is_open) {
		if (!squashfs_opendir(pdata, pdata->sBlk.root_inode, &pdata->dir))
			return -EIO;
	}

	struct dir_ent *ent;
	while ((ent = squashfs_dir_next(&pdata->dir))) {
		if (ent->type == SQUASHFS_REG_TYPE || ent->type == SQUASHFS_LREG_TYPE) {
			char *ptr = strrchr(ent->name, '.');
			if (ptr && !strcmp(ptr + 1, "desktop")) {
				*filename = ent->name;
				return 1;
			}
		}
	}

	squashfs_closedir(&pdata->dir);
	return 0;
}
