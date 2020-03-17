/*
    This file is part of Ciso.

    Ciso is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Ciso is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA


    Copyright 2005 BOOSTER
	Copyright 2011-2013 SWAT
*/


/****************************************************************************
	Dependencies
****************************************************************************/
#include <stdio.h>
#include <stdlib.h>    // abort
#include <string.h>    // printf
#include <assert.h>
#include <memory.h>
#include <zlib.h>
#include <zconf.h>
#include <lzo/lzo1x.h>
#include "ciso.h"



/****************************************************************************
	Macros
****************************************************************************/
#define END(...) { \
	fprintf(stderr, "" __VA_ARGS__); \
	fprintf(stderr, " \n"); \
	abort(); \
}



/****************************************************************************
	Utilities
****************************************************************************/
/* This function is necessarily successful if it returns */
static FILE* fopen_orDie(const char* fname, const char* mode)
{
	FILE* const f = fopen(fname, mode);
	if (f==NULL) END("can't open '%s' with mode '%s'", fname, mode);
	return f;
}

/* This function is necessarily successful if it returns */
static size_t fread_orDie(void* buf, size_t eltSize, size_t nbElts, FILE* f)
{
	size_t const nbEltsRead = fread(buf, eltSize, nbElts, f);
	if (nbEltsRead != nbElts) END("error reading data");
	return nbEltsRead;
}

/* This function is necessarily successful if it returns */
static size_t fwrite_orDie(const void* buf, size_t eltSize, size_t nbElts, FILE* f)
{
	size_t const nbEltsWritten = fwrite(buf, eltSize, nbElts, f);
	if (nbEltsWritten != nbElts) END("error writing data");
	return nbEltsWritten;
}

/* This function is necessarily successful if it returns */
static void fseek_orDie(FILE* f, long int offset, int whence)
{
	int const res = fseek(f, offset, whence);
	if (res != 0) END("error seeking into file");
}



/****************************************************************************
	decompress CSO to ISO
****************************************************************************/
static void decomp_ciso(const char* fname_out, const char* fname_in)
{
	FILE* const fin = fopen_orDie(fname_in, "rb");
	FILE* const fout = fopen_orDie(fname_out, "wb");

	CISO_H ciso;
	z_stream z;
	int ciso_total_block;

	uint32_t *index_buf;
	unsigned char *block_buf1;
	unsigned char *block_buf2;

	/* read header */
	fread_orDie(&ciso, 1, sizeof(ciso), fin);

	/* check header */
	if (
		(ciso.magic[0] != 'Z' && ciso.magic[0] != 'C') ||
		ciso.magic[1] != 'I' ||
		ciso.magic[2] != 'S' ||
		ciso.magic[3] != 'O' ||
		ciso.block_size == 0 ||
		ciso.total_bytes == 0
	)
	{
		END("ciso file format error");
	}

	{	uint64_t const ctb64 = ciso.total_bytes / ciso.block_size;
		assert(ctb64 < INT_MAX);
		ciso_total_block = (int)ctb64;
	}

	/* allocate index block */
	{
		size_t const index_size = (size_t)(ciso_total_block + 1 ) * sizeof(uint32_t);
		index_buf  = malloc(index_size);
		block_buf1 = malloc(ciso.block_size*2);
		block_buf2 = malloc(ciso.block_size*2);

		if( !index_buf || !block_buf1 || !block_buf2 )
			END("Can't allocate memory %d and %d x2",
				(int)index_size, (int)ciso.block_size*2);

		/* read index block */
		fread_orDie(index_buf, 1, index_size, fin);
	}

	/* show info */
	printf("Decompress '%s' to '%s' \n", fname_in, fname_out);
	printf("Total File Size %ld bytes\n", (long)ciso.total_bytes);
	printf("block size      %d  bytes\n", (int)ciso.block_size);
	printf("total blocks    %d  blocks\n", ciso_total_block);
	printf("index align     %d\n", 1<<ciso.align);
	printf("type  %s\n", ciso.magic[0] == 'Z' ? "ZISO" : "CISO");

	if(ciso.magic[0] == 'Z') {
		if(lzo_init() != LZO_E_OK) END("lzo_init() failed");
	} else {
		/* init zlib */
		z.zalloc = Z_NULL;
		z.zfree  = Z_NULL;
		z.opaque = Z_NULL;
	}

	/* decompress data */
	{
		int percent_period = ciso_total_block / 100;
		int percent_cnt = 0;
		unsigned index, plain;
		long int read_pos;

		int block;
		for(block = 0;block < ciso_total_block ; block++)
		{
			size_t read_size;
			size_t cmp_size;

			if(--percent_cnt<=0)
			{
				percent_cnt = percent_period;
				printf("decompress %d%%\r",block / percent_period);
			}

			if(ciso.magic[0] == 'C')  {
				if (inflateInit2(&z,-15) != Z_OK)
					END("deflateInit : %s\n", (z.msg) ? z.msg : "???");
			}

			/* check index */
			index  = index_buf[block];
			plain  = index & 0x80000000;
			index  &= 0x7fffffff;
			assert((LONG_MAX >> ciso.align) > index);  // check overflow
			read_pos = index << (ciso.align);

			if(plain)
			{
				read_size = ciso.block_size;
			}
			else
			{
				unsigned int const index2 = index_buf[block+1] & 0x7fffffff;
				read_size = (index2-index) << (ciso.align);
			}

			fseek_orDie(fin, read_pos, SEEK_SET);

			z.avail_in  = (uInt)fread_orDie(block_buf2, 1, read_size , fin);

			if(z.avail_in != read_size)
				END("block=%d : read error", block);

			if(plain)
			{
				memcpy(block_buf1,block_buf2,read_size);
				cmp_size = read_size;
			}
			else
			{
				if(ciso.magic[0] == 'Z') {
					unsigned long lzolen;
					int const status = lzo1x_decompress(block_buf2, z.avail_in, block_buf1, &lzolen, NULL);
					cmp_size = lzolen;
					z.avail_out = ciso.block_size;

					if(status != LZO_E_OK) END("block %d:lzo1x_decompress: %d", block, status);

				} else {
					int status;

					z.next_out  = block_buf1;
					z.avail_out = ciso.block_size;
					z.next_in   = block_buf2;

					status = inflate(&z, Z_FULL_FLUSH);

					if (status != Z_STREAM_END)
						END("block %d:inflate : %s[%d]", block,(z.msg) ? z.msg : "error", status);

					cmp_size = ciso.block_size - z.avail_out;
				}

				if (cmp_size != ciso.block_size)
					END("block %d : block size error %d != %d",
						block, (int)cmp_size, (int)ciso.block_size);
			}
			/* write decompressed block */
			fwrite_orDie(block_buf1, 1,cmp_size , fout);

			if(ciso.magic[0] == 'C')  {
				/* term zlib */
				if (inflateEnd(&z) != Z_OK)
					END("inflateEnd : %s\n", (z.msg) ? z.msg : "error");
			}
		}
	}

	printf("ciso decompress completed\n");

	/* clean out */
	free(index_buf);
	free(block_buf1);
	free(block_buf2);
	fclose(fin);
	fclose(fout);
}

/****************************************************************************
	compress ISO
****************************************************************************/
/* if this function returns, it's necessary successful */
static CISO_H init_ciso_header(FILE* fp, int is_ziso)
{
	CISO_H ciso;
	long int pos;

	fseek_orDie(fp, 0, SEEK_END);
	pos = ftell(fp);
	if (pos==-1L) END("Cannot determine file size; terminating ... \n");

	/* init ciso header */
	memset(&ciso, 0, sizeof(ciso));

	if(is_ziso) {
		ciso.magic[0] = 'Z';
	} else {
		ciso.magic[0] = 'C';
	}
	ciso.magic[1] = 'I';
	ciso.magic[2] = 'S';
	ciso.magic[3] = 'O';
	ciso.ver      = 0x01;

	ciso.block_size  = 0x800; /* ISO9660 one of sector */
	assert(pos > 0);
	ciso.total_bytes = (uint64_t)pos;
#if 0
	/* align >0 has bug */
	for(ciso.align = 0 ; (ciso.total_bytes >> ciso.align) >0x80000000LL ; ciso.align++);
#endif

	fseek_orDie(fp,0,SEEK_SET);

	return ciso;
}

static void comp_ciso(int level, int is_ziso, int no_comp_diff,
                     const char* fname_out, const char* fname_in)
{
	FILE* const fin = fopen_orDie(fname_in, "rb");
	FILE* const fout = fopen_orDie(fname_out, "wb");

	unsigned long long write_pos;
	int block;
	unsigned char buf4[64] = {0};
	int cmp_size;
	int percent_period;
	int percent_cnt;
	int align_b, align_m;

	z_stream z;
	lzo_voidp wrkmem = NULL;

	CISO_H ciso = init_ciso_header(fin, is_ziso);
	int const ciso_total_block = (int)(ciso.total_bytes / ciso.block_size);

	/* allocate index block */
	size_t const index_size = (size_t)(ciso_total_block + 1 ) * sizeof(uint32_t);

	uint32_t* const index_buf  = calloc(1, index_size);
	unsigned char* const block_buf1 = malloc(ciso.block_size*2);
	unsigned char* const block_buf2 = malloc(ciso.block_size*2);

	if( !index_buf || !block_buf1 || !block_buf2 )
		END("Can't allocate memory\n");

	if(is_ziso)  {
		if(lzo_init() != LZO_E_OK)
			END("lzo_init() failed\n");

		//wrkmem = (lzo_voidp) malloc(LZO1X_1_MEM_COMPRESS);
		wrkmem = (lzo_voidp) malloc(LZO1X_999_MEM_COMPRESS);
		if (wrkmem==NULL) END("memory allocation error");

	} else {
		/* init zlib */
		z.zalloc = Z_NULL;
		z.zfree  = Z_NULL;
		z.opaque = Z_NULL;
	}

	/* show info */
	printf("Compress '%s' to '%s' \n", fname_in, fname_out);
	printf("Total File Size %ld bytes\n", (long)ciso.total_bytes);
	printf("block size      %d  bytes\n", (int)ciso.block_size);
	printf("index align     %d\n", 1<<ciso.align);
	printf("compress level  %d\n", level);
	printf("type  %s\n", is_ziso ? "ZISO" : "CISO");

	/* write header block */
	fwrite_orDie(&ciso, 1, sizeof(ciso), fout);

	/* skip index block */
	assert(index_size < LONG_MAX);
	fseek_orDie(fout, (long int)index_size, SEEK_CUR);

	write_pos = sizeof(ciso) + index_size;

	/* compress data */
	percent_period = ciso_total_block/100;
	percent_cnt    = ciso_total_block/100;

	align_b = 1 << ciso.align;
	align_m = align_b - 1;

	for(block = 0;block < ciso_total_block ; block++)
	{
		if(--percent_cnt<=0)
		{
			percent_cnt = percent_period;
			printf("compress %3d%% avarage rate %3d%%\r",
				block / percent_period,
				(block==0) ? 0 : (int)(100*write_pos/((unsigned long long)block*0x800)) );
		}

		if(!is_ziso)  {
			if (deflateInit2(&z, level , Z_DEFLATED, -15,8,Z_DEFAULT_STRATEGY) != Z_OK)
				END("deflateInit : %s\n", (z.msg) ? z.msg : "???");
		}

		/* write align */
		{
			int align = (int)write_pos & align_m;
			if (align)
			{
				align = align_b - align;
				assert(align >= 0);
				fwrite_orDie(buf4, 1, (size_t)align, fout);
				write_pos = write_pos + (unsigned long long)align;
			}
		}

		/* mark offset index */
		index_buf[block] = (uint32_t)(write_pos >> ciso.align);

		/* read buffer */
		z.next_out  = block_buf2;
		z.avail_out = ciso.block_size*2;
		z.next_in   = block_buf1;
		z.avail_in  = (uInt)fread_orDie(block_buf1, 1, ciso.block_size , fin);

		if(is_ziso)  {
			unsigned long lzolen;

			//status = lzo1x_1_compress(block_buf1, z.avail_in, block_buf2, &lzolen, wrkmem);
			int const status =
				lzo1x_999_compress(block_buf1, z.avail_in, block_buf2, &lzolen, wrkmem);

			//printf("in: %d out: %d\n", z.avail_in, lzolen);

			if (status != LZO_E_OK)
				END("compression failed: lzo1x_1_compress: %d\n", status);

			assert(lzolen < INT_MAX);
			cmp_size = (int)lzolen;

		} else {

			/* compress block */
			int const status = deflate(&z, Z_FINISH);
			if (status != Z_STREAM_END)
				END("block %d:deflate : %s[%d]\n", block,(z.msg) ? z.msg : "error", status);
			cmp_size = (int)(ciso.block_size*2 - z.avail_out);
		}

		/* choise plain / compress */
		if (cmp_size >= ((int)ciso.block_size - no_comp_diff))
		{
			cmp_size = (int)ciso.block_size;
			memcpy(block_buf2, block_buf1, (size_t)cmp_size);
			/* plain block mark */
			index_buf[block] |= 0x80000000;
		}

		/* write compressed block */
		fwrite_orDie(block_buf2, 1, (size_t)cmp_size, fout);

		/* mark next index */
		write_pos = write_pos + (unsigned long long)cmp_size;

		if(!is_ziso)  {
			/* term zlib */
			if (deflateEnd(&z) != Z_OK)
				END("deflateEnd : %s\n", (z.msg) ? z.msg : "error");
		}
	}

	/* last position (total size)*/
	index_buf[block] = (uint32_t)(write_pos >> ciso.align);

	/* write index block */
	fseek_orDie(fout, sizeof(ciso), SEEK_SET);
	fwrite_orDie(index_buf, 1, index_size, fout);

	printf("ciso compression completed ; total size = %8d bytes , rate %d%% \n",
			(int)write_pos, (int)(write_pos*100/ciso.total_bytes));

	/* clean out */
	free(index_buf);
	free(block_buf1);
	free(block_buf2);
	fclose(fin);
	fclose(fout);
}


/****************************************************************************
	command line
****************************************************************************/
int main(int argc, char *argv[])
{
	int level;
	int no_comp_diff = 48;  /* minimym gain required per block */

	printf("ISO9660 comp/dec to/from CISO/ZISO v1.1 by SWAT \n");

	if (argc < 5)
	{
		printf("Usage: %s library level infile outfile [no_comp_diff_bytes]\n", argv[0]);
		printf(" level:   1-9 for compress (1=fast/large - 9=small/slow), 0 for decompress\n");
		printf(" library: zlib or lzo\n\n");
		return 0;
	}
	level = argv[2][0] - '0';
	if(level < 0 || level > 9)
	{
        printf("Unknown mode: %c\n", argv[2][0]);
		return 1;
	}

	if(argc > 5) {
		no_comp_diff = atoi(argv[5]);
	}

	{
		const char* const fname_in = argv[3];
		const char* const fname_out = argv[4];
		int const is_ziso = !strcmp("lzo", argv[1]);

		if(level==0)
			decomp_ciso(fname_out, fname_in);
		else
			comp_ciso(level, is_ziso, no_comp_diff, fname_out, fname_in);
	}

	return 0;
}
