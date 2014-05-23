/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Unit tests and benches -
 *
 *  Copyright(C) 2002 Pascal Massimino <skal@planet-d.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * $Id: xvid_bench.c 1985 2011-05-18 09:02:35Z Isibaar $
 *
 ****************************************************************************/

/*****************************************************************************
 *                            
 *  'Reference' output is at the end of file.
 *
 *   compiles with something like:
 *   gcc -o xvid_bench xvid_bench.c  -I../src/ -lxvidcore -lm
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>    /* for memset */
#include <assert.h>

#ifndef WIN32
#include <sys/time.h>	/* for gettimeofday */
#else
#include <time.h>
#endif


#include "xvid.h"

// inner guts
#include "portab.h"
#include "dct/idct.h"
#include "dct/fdct.h"
#include "image/colorspace.h"
#include "image/interpolate8x8.h"
#include "utils/mem_transfer.h"
#include "quant/quant.h"
#include "motion/sad.h"
#include "utils/emms.h"
#include "utils/timer.h"
#include "quant/quant_matrix.c"
#include "bitstream/cbp.h"
#include "bitstream/bitstream.h"

#include <math.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

int speed_ref = 100;  /* on slow machines, decrease this value */
int verbose = 0;
unsigned int cpu_mask;

/*********************************************************************
 * misc
 *********************************************************************/

/* returns time in micro-s*/
double gettime_usec()
{    
#ifndef WIN32
	struct timeval  tv;
	gettimeofday(&tv, 0);
	return tv.tv_sec*1.0e6 + tv.tv_usec;
#else
	clock_t clk;
	clk = clock();
	return clk * 1000. / CLOCKS_PER_SEC;  /* clock() returns time in Milliseconds */
#endif
}

/* returns squared deviates (mean(v*v)-mean(v)^2) of a 8x8 block */
double sqr_dev(uint8_t v[8*8])
{
	double sum=0.;
	double sum2=0.;
	int n;
	for (n=0;n<8*8;n++)
	{
		sum  += v[n];
		sum2 += v[n]*v[n];
	}
	sum2 /= n;
	sum /= n;
	return sum2-sum*sum;
}

/*********************************************************************
 * cpu init
 *********************************************************************/

typedef struct {
	const char *name;
	unsigned int cpu;
} CPU;

CPU cpu_list[] = {
	{ "PLAINC ", 0 },
#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
	{ "MMX    ", XVID_CPU_MMX },
	{ "MMXEXT ", XVID_CPU_MMXEXT | XVID_CPU_MMX },
	{ "SSE2   ", XVID_CPU_SSE2 | XVID_CPU_MMX },
	{ "SSE3   ", XVID_CPU_SSE3 | XVID_CPU_SSE2 | XVID_CPU_MMX },
	{ "SSE41  ", XVID_CPU_SSE41| XVID_CPU_SSE3 | XVID_CPU_SSE2 | XVID_CPU_MMX },
	{ "3DNOW  ", XVID_CPU_3DNOW },
	{ "3DNOWE ", XVID_CPU_3DNOW | XVID_CPU_3DNOWEXT },
#endif
#ifdef ARCH_IS_PPC
	{ "ALTIVEC", XVID_CPU_ALTIVEC },
#endif
#ifdef ARCH_IS_IA64
//	{ "IA64   ", XVID_CPU_IA64 },
#endif
//	{ "TSC    ", XVID_CPU_TSC },
	{ 0, 0 }
};


int init_cpu(CPU *cpu)
{
	xvid_gbl_info_t xinfo;

	/* Get the available CPU flags */
	memset(&xinfo, 0, sizeof(xinfo));
	xinfo.version = XVID_VERSION;
	xvid_global(NULL, XVID_GBL_INFO, &xinfo, NULL);

	/* Are we trying to test a subset of the host CPU features */
	if ((xinfo.cpu_flags & cpu->cpu) == cpu->cpu) {
		int xerr;
		xvid_gbl_init_t xinit;
		memset(&xinit, 0, sizeof(xinit));
		xinit.cpu_flags = cpu->cpu | XVID_CPU_FORCE;
		xinit.version = XVID_VERSION;
		xerr = xvid_global(NULL, XVID_GBL_INIT, &xinit, NULL);
		if (xerr==XVID_ERR_FAIL) {
			/* libxvidcore failed to init */
			return 0;
		}
	} else {
		/* The host CPU doesn't support some required feature for this test */
		return(0);
	}
	return 1;
}

#define CRC32_REMAINDER 0xCBF43926
#define CRC32_INITIAL 0xffffffff

#define DO1(c, crc) ((crc) = crc32tab[((unsigned int)((crc)>>24) ^ (*c++)) & 0xff] ^ ((crc) << 8))
#define DO2(c, crc)  DO1(c, crc); DO1(c, crc);
#define DO4(c, crc)  DO2(c, crc); DO2(c, crc);
#define DO8(c, crc)  DO4(c, crc); DO4(c, crc);

/******************************************************************************
* Precomputed AAL5 CRC32 lookup table
******************************************************************************/

static unsigned long crc32tab[256] = {

	0x00000000L, 0x04C11DB7L, 0x09823B6EL, 0x0D4326D9L,
	0x130476DCL, 0x17C56B6BL, 0x1A864DB2L, 0x1E475005L,
	0x2608EDB8L, 0x22C9F00FL, 0x2F8AD6D6L, 0x2B4BCB61L,
	0x350C9B64L, 0x31CD86D3L, 0x3C8EA00AL, 0x384FBDBDL,
	0x4C11DB70L, 0x48D0C6C7L, 0x4593E01EL, 0x4152FDA9L,
	0x5F15ADACL, 0x5BD4B01BL, 0x569796C2L, 0x52568B75L,
	0x6A1936C8L, 0x6ED82B7FL, 0x639B0DA6L, 0x675A1011L,
	0x791D4014L, 0x7DDC5DA3L, 0x709F7B7AL, 0x745E66CDL,
	0x9823B6E0L, 0x9CE2AB57L, 0x91A18D8EL, 0x95609039L,
	0x8B27C03CL, 0x8FE6DD8BL, 0x82A5FB52L, 0x8664E6E5L,
	0xBE2B5B58L, 0xBAEA46EFL, 0xB7A96036L, 0xB3687D81L,
	0xAD2F2D84L, 0xA9EE3033L, 0xA4AD16EAL, 0xA06C0B5DL,
	0xD4326D90L, 0xD0F37027L, 0xDDB056FEL, 0xD9714B49L,
	0xC7361B4CL, 0xC3F706FBL, 0xCEB42022L, 0xCA753D95L,
	0xF23A8028L, 0xF6FB9D9FL, 0xFBB8BB46L, 0xFF79A6F1L,
	0xE13EF6F4L, 0xE5FFEB43L, 0xE8BCCD9AL, 0xEC7DD02DL,
	0x34867077L, 0x30476DC0L, 0x3D044B19L, 0x39C556AEL,
	0x278206ABL, 0x23431B1CL, 0x2E003DC5L, 0x2AC12072L,
	0x128E9DCFL, 0x164F8078L, 0x1B0CA6A1L, 0x1FCDBB16L,
	0x018AEB13L, 0x054BF6A4L, 0x0808D07DL, 0x0CC9CDCAL,
	0x7897AB07L, 0x7C56B6B0L, 0x71159069L, 0x75D48DDEL,
	0x6B93DDDBL, 0x6F52C06CL, 0x6211E6B5L, 0x66D0FB02L,
	0x5E9F46BFL, 0x5A5E5B08L, 0x571D7DD1L, 0x53DC6066L,
	0x4D9B3063L, 0x495A2DD4L, 0x44190B0DL, 0x40D816BAL,
	0xACA5C697L, 0xA864DB20L, 0xA527FDF9L, 0xA1E6E04EL,
	0xBFA1B04BL, 0xBB60ADFCL, 0xB6238B25L, 0xB2E29692L,
	0x8AAD2B2FL, 0x8E6C3698L, 0x832F1041L, 0x87EE0DF6L,
	0x99A95DF3L, 0x9D684044L, 0x902B669DL, 0x94EA7B2AL,
	0xE0B41DE7L, 0xE4750050L, 0xE9362689L, 0xEDF73B3EL,
	0xF3B06B3BL, 0xF771768CL, 0xFA325055L, 0xFEF34DE2L,
	0xC6BCF05FL, 0xC27DEDE8L, 0xCF3ECB31L, 0xCBFFD686L,
	0xD5B88683L, 0xD1799B34L, 0xDC3ABDEDL, 0xD8FBA05AL,
	0x690CE0EEL, 0x6DCDFD59L, 0x608EDB80L, 0x644FC637L,
	0x7A089632L, 0x7EC98B85L, 0x738AAD5CL, 0x774BB0EBL,
	0x4F040D56L, 0x4BC510E1L, 0x46863638L, 0x42472B8FL,
	0x5C007B8AL, 0x58C1663DL, 0x558240E4L, 0x51435D53L,
	0x251D3B9EL, 0x21DC2629L, 0x2C9F00F0L, 0x285E1D47L,
	0x36194D42L, 0x32D850F5L, 0x3F9B762CL, 0x3B5A6B9BL,
	0x0315D626L, 0x07D4CB91L, 0x0A97ED48L, 0x0E56F0FFL,
	0x1011A0FAL, 0x14D0BD4DL, 0x19939B94L, 0x1D528623L,
	0xF12F560EL, 0xF5EE4BB9L, 0xF8AD6D60L, 0xFC6C70D7L,
	0xE22B20D2L, 0xE6EA3D65L, 0xEBA91BBCL, 0xEF68060BL,
	0xD727BBB6L, 0xD3E6A601L, 0xDEA580D8L, 0xDA649D6FL,
	0xC423CD6AL, 0xC0E2D0DDL, 0xCDA1F604L, 0xC960EBB3L,
	0xBD3E8D7EL, 0xB9FF90C9L, 0xB4BCB610L, 0xB07DABA7L,
	0xAE3AFBA2L, 0xAAFBE615L, 0xA7B8C0CCL, 0xA379DD7BL,
	0x9B3660C6L, 0x9FF77D71L, 0x92B45BA8L, 0x9675461FL,
	0x8832161AL, 0x8CF30BADL, 0x81B02D74L, 0x857130C3L,
	0x5D8A9099L, 0x594B8D2EL, 0x5408ABF7L, 0x50C9B640L,
	0x4E8EE645L, 0x4A4FFBF2L, 0x470CDD2BL, 0x43CDC09CL,
	0x7B827D21L, 0x7F436096L, 0x7200464FL, 0x76C15BF8L,
	0x68860BFDL, 0x6C47164AL, 0x61043093L, 0x65C52D24L,
	0x119B4BE9L, 0x155A565EL, 0x18197087L, 0x1CD86D30L,
	0x029F3D35L, 0x065E2082L, 0x0B1D065BL, 0x0FDC1BECL,
	0x3793A651L, 0x3352BBE6L, 0x3E119D3FL, 0x3AD08088L,
	0x2497D08DL, 0x2056CD3AL, 0x2D15EBE3L, 0x29D4F654L,
	0xC5A92679L, 0xC1683BCEL, 0xCC2B1D17L, 0xC8EA00A0L,
	0xD6AD50A5L, 0xD26C4D12L, 0xDF2F6BCBL, 0xDBEE767CL,
	0xE3A1CBC1L, 0xE760D676L, 0xEA23F0AFL, 0xEEE2ED18L,
	0xF0A5BD1DL, 0xF464A0AAL, 0xF9278673L, 0xFDE69BC4L,
	0x89B8FD09L, 0x8D79E0BEL, 0x803AC667L, 0x84FBDBD0L,
	0x9ABC8BD5L, 0x9E7D9662L, 0x933EB0BBL, 0x97FFAD0CL,
	0xAFB010B1L, 0xAB710D06L, 0xA6322BDFL, 0xA2F33668L,
	0xBCB4666DL, 0xB8757BDAL, 0xB5365D03L, 0xB1F740B4L

};

uint32_t
calc_crc(uint8_t *mem, int len, uint32_t crc)
{
	while( len >= 8) {
		DO8(mem, crc);
		len -= 8;
	}

	while( len ) {
		DO1(mem, crc);
		len--;
	}

	return crc;
}

void byte_swap(uint8_t *mem, int len, int element_size) {
#ifdef ARCH_IS_BIG_ENDIAN
	int i;

	if(element_size == 1) {
		/* No need to swap */
	} else if(element_size == 2) {
		uint8_t temp[2];
		
		for(i=0; i < (len/2); i++ ) {
			temp[0] = mem[0];
			temp[1] = mem[1];
			mem[0] = temp[1];
			mem[1] = temp[0];

			mem += 2;
		}
	} else if(element_size == 4) {
		uint8_t temp[4];
		
		for(i=0; i < (len/4); i++ ) {
			temp[0] = mem[0];
			temp[1] = mem[1];
			temp[2] = mem[2];
			temp[3] = mem[3];
			mem[0] = temp[3];
			mem[1] = temp[2];
			mem[2] = temp[1];
			mem[3] = temp[0];

			mem += 4;
		}
	} else {
		printf("ERROR: byte_swap unsupported element_size(%u)\n", element_size);
	}
#endif
}

/*********************************************************************
 * test DCT
 *********************************************************************/

#define ABS(X)  ((X)<0 ? -(X) : (X))

void test_dct()
{
	const int nb_tests = 300*speed_ref;
	int tst;
	CPU *cpu;
	int i;
	DECLARE_ALIGNED_MATRIX(iDst0, 8, 8, short, 16);
	DECLARE_ALIGNED_MATRIX(iDst,  8, 8, short, 16);
	DECLARE_ALIGNED_MATRIX(fDst,  8, 8, short, 16);
	double overhead;

	printf( "\n ===== test fdct/idct =====\n" );

	for(i=0; i<8*8; ++i) iDst0[i] = (i*7-i*i) & 0x7f;
	overhead = gettime_usec();
	for(tst=0; tst<nb_tests; ++tst)
	{
		for(i=0; i<8*8; ++i) fDst[i] = iDst0[i];
		for(i=0; i<8*8; ++i) iDst[i] = fDst[i];
	}
	overhead = gettime_usec() - overhead;

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		double t, PSNR, MSE;

		if (!init_cpu(cpu))
			continue;

		t = gettime_usec();
		emms();
		for(tst=0; tst<nb_tests; ++tst)
		{
			for(i=0; i<8*8; ++i) fDst[i] = iDst0[i];
			fdct(fDst);
			for(i=0; i<8*8; ++i) iDst[i] = fDst[i];
			idct(iDst);
		}
		emms();
		t = (gettime_usec() - t - overhead) / nb_tests;
		MSE = 0.;
		for(i=0; i<8*8; ++i) {
			double delta = 1.0*(iDst[i] - iDst0[i]);
			MSE += delta*delta;
		}
		PSNR = (MSE==0.) ? 1.e6 : -4.3429448*log( MSE/64. );
		printf( "%s -  %.3f usec       PSNR=%.3f  MSE=%.3f %s\n",
				cpu->name, t, PSNR, MSE,
				(ABS(MSE)>=64)? "| ERROR" :"");
	}
}

/*********************************************************************
 * test SAD
 *********************************************************************/

void test_sad()
{
	const int nb_tests = 2000*speed_ref;
	int tst;
	CPU *cpu;
	int i;
	DECLARE_ALIGNED_MATRIX(Cur,  16, 16, uint8_t, 16);
	DECLARE_ALIGNED_MATRIX(Ref1, 16, 16, uint8_t, 16);
	DECLARE_ALIGNED_MATRIX(Ref2, 16, 16, uint8_t, 16);

	printf( "\n ======  test SAD ======\n" );
	for(i=0; i<16*16;++i) {
		Cur[i] = (i/5) ^ 0x05;
		Ref1[i] = (i + 0x0b) & 0xff;
		Ref2[i] = i ^ 0x76;
	}

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		double t;
		uint32_t s;
		if (!init_cpu(cpu))
			continue;

		t = gettime_usec();
		emms();
		for(tst=0; tst<nb_tests; ++tst) s = sad8(Cur, Ref1, 16);
		emms();
		t = (gettime_usec() - t) / nb_tests;
		printf("%s - sad8    %.3f usec       sad=%d %s\n",
			   cpu->name, t, s,
			   (s!=3776)?"| ERROR": "" );

		t = gettime_usec();
		emms();
		for(tst=0; tst<nb_tests; ++tst) s = sad16(Cur, Ref1, 16, -1);
		emms();
		t = (gettime_usec() - t) / nb_tests;
		printf("%s - sad16   %.3f usec       sad=%d %s\n",
			   cpu->name, t, s,
			   (s!=27214)?"| ERROR": "" );

		t = gettime_usec();
		emms();
		for(tst=0; tst<nb_tests; ++tst) s = sad16bi(Cur, Ref1, Ref2, 16);
		emms();
		t = (gettime_usec() - t) / nb_tests;
		printf( "%s - sad16bi %.3f usec       sad=%d %s\n",
				cpu->name, t, s,
				(s!=26274)?"| ERROR": "" );

                t = gettime_usec();
                emms();
                for(tst=0; tst<nb_tests; ++tst) s = sad8bi(Cur, Ref1, Ref2, 8);
                emms();
                t = (gettime_usec() - t) / nb_tests;
                printf( "%s - sad8bi %.3f usec       sad=%d %s\n",
                                cpu->name, t, s,
                                (s!=4002)?"| ERROR": "" );

		t = gettime_usec();
		emms();
		for(tst=0; tst<nb_tests; ++tst) s = dev16(Cur, 16);
		emms();
		t = (gettime_usec() - t) / nb_tests;
		printf( "%s - dev16   %.3f usec       sad=%d %s\n",
				cpu->name, t, s,
				(s!=3344)?"| ERROR": "" );

		printf( " --- \n" );
	}
}

/*********************************************************************
 * test interpolation
 *********************************************************************/

#define ENTER \
for(i=0; i<16*8; ++i) Dst[i] = 0;   \
t = gettime_usec();                   \
emms();

#define LEAVE \
emms();                             \
t = (gettime_usec() - t) / nb_tests;  \
	iCrc = calc_crc((uint8_t*)Dst, sizeof(Dst), CRC32_INITIAL)

#define TEST_MB(FUNC, R)                \
ENTER                               \
for(tst=0; tst<nb_tests; ++tst) (FUNC)(Dst, Src0, 16, (R)); \
LEAVE

#define TEST_MB2(FUNC)                  \
ENTER                               \
for(tst=0; tst<nb_tests; ++tst) (FUNC)(Dst, Src0, 16); \
LEAVE


void test_mb()
{
	const int nb_tests = 2000*speed_ref;
	CPU *cpu;
	const uint8_t Src0[16*9] = {
		/* try to have every possible combinaison of rounding... */
		0, 0, 1, 0, 2, 0, 3, 0, 4             ,0,0,0, 0,0,0,0,
		0, 1, 1, 1, 2, 1, 3, 1, 3             ,0,0,0, 0,0,0,0,
		0, 2, 1, 2, 2, 2, 3, 2, 2             ,0,0,0, 0,0,0,0,
		0, 3, 1, 3, 2, 3, 3, 3, 1             ,0,0,0, 0,0,0,0,
		1, 3, 0, 2, 1, 0, 2, 3, 4             ,0,0,0, 0,0,0,0,
		2, 2, 1, 2, 0, 1, 3, 5, 3             ,0,0,0, 0,0,0,0,
		3, 1, 2, 3, 1, 2, 2, 6, 2             ,0,0,0, 0,0,0,0,
		1, 0, 1, 3, 0, 3, 1, 6, 1             ,0,0,0, 0,0,0,0,
		4, 3, 2, 1, 2, 3, 4, 0, 3             ,0,0,0, 0,0,0,0
	};
	uint8_t Dst[16*8] = {0};

	printf( "\n ===  test block motion ===\n" );

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		double t;
		int tst, i, iCrc;

		if (!init_cpu(cpu))
			continue;

		TEST_MB(interpolate8x8_halfpel_h, 0);
		printf("%s - interp- h-round0 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0x115381ba)?"| ERROR": "" );

		TEST_MB(interpolate8x8_halfpel_h, 1);
		printf("%s -           round1 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0x2b1f528f)?"| ERROR": "" );


		TEST_MB(interpolate8x8_halfpel_v, 0);
		printf("%s - interp- v-round0 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0x423cdcc7)?"| ERROR": "" );

		TEST_MB(interpolate8x8_halfpel_v, 1);
		printf("%s -           round1 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0x42202efe)?"| ERROR": "" );


		TEST_MB(interpolate8x8_halfpel_hv, 0);
		printf("%s - interp-hv-round0 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0xd198d387)?"| ERROR": "" );

		TEST_MB(interpolate8x8_halfpel_hv, 1);
		printf("%s -           round1 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0x9ecfd921)?"| ERROR": "" );


		/* this is a new function, as of 06.06.2002 */
#if 0
		TEST_MB2(interpolate8x8_avrg);
		printf("%s - interpolate8x8_c %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=8107)?"| ERROR": "" );
#endif

    /* New functions for field prediction by CK 1.10.2005 */
#pragma NEW8X4
		TEST_MB(interpolate8x4_halfpel_h, 0);
		printf("%s - interpfield-h -round0 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0x9538d6df)?"| ERROR": "" );

		TEST_MB(interpolate8x4_halfpel_h, 1);
		printf("%s -                round1 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0xde5f1db4)?"| ERROR": "" );


		TEST_MB(interpolate8x4_halfpel_v, 0);
		printf("%s - interpfield- v-round0 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0xea5a69ef)?"| ERROR": "" );

		TEST_MB(interpolate8x4_halfpel_v, 1);
		printf("%s -                round1 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0x4f10ec0f)?"| ERROR": "" );


		TEST_MB(interpolate8x4_halfpel_hv, 0);
		printf("%s - interpfield-hv-round0 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0xf97ee367)?"| ERROR": "" );

		TEST_MB(interpolate8x4_halfpel_hv, 1);
		printf("%s -                round1 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, iCrc,
			   (iCrc!=0xb6a9f581)?"| ERROR": "" );
/* End of 8x4 functions */

		printf( " --- \n" );
	}
}

#undef ENTER
#undef LEAVE
#undef TEST_MB
#undef TEST_MB2

/*********************************************************************
 * test transfer
 *********************************************************************/

#define INIT_TRANSFER \
for(i=0; i<8*32; ++i) {             \
Src8[i] = i; Src16[i] = i;        \
Dst8[i] = 0; Dst16[i] = 0;        \
Ref1[i] = i^0x27;                 \
Ref2[i] = i^0x51;                 \
}

#define TEST_TRANSFER_BEGIN(DST)              \
INIT_TRANSFER                             \
overhead = -gettime_usec();               \
for(tst=0; tst<nb_tests; ++tst) {         \
  for(i=0; i<8*32; ++i) (DST)[i] = i^0x6a;\
}                                         \
overhead += gettime_usec();               \
t = gettime_usec();                       \
emms();                                   \
for(tst=0; tst<nb_tests; ++tst) {         \
  for(i=0; i<8*32; ++i) (DST)[i] = i^0x6a;


#define TEST_TRANSFER_END(DST)                \
}                                         \
emms();                                   \
t = (gettime_usec()-t -overhead) / nb_tests;\
byte_swap((uint8_t*)(DST), 8*32*sizeof((DST)[0]), sizeof((DST)[0]));  \
s = calc_crc((uint8_t*)(DST), 8*32*sizeof((DST)[0]), CRC32_INITIAL)

#define TEST_TRANSFER(FUNC, DST, SRC)         \
TEST_TRANSFER_BEGIN(DST);                 \
  (FUNC)((DST), (SRC), 32);               \
TEST_TRANSFER_END(DST)


#define TEST_TRANSFER2_BEGIN(DST, SRC)        \
INIT_TRANSFER                             \
overhead = -gettime_usec();               \
for(tst=0; tst<nb_tests; ++tst) {         \
  for(i=0; i<8*32; ++i) (DST)[i] = i^0x6a;\
  for(i=0; i<8*32; ++i) (SRC)[i] = i^0x3e;\
}                                         \
overhead += gettime_usec();               \
t = gettime_usec();                       \
emms();                                   \
for(tst=0; tst<nb_tests; ++tst) {         \
  for(i=0; i<8*32; ++i) (DST)[i] = i^0x6a;\
  for(i=0; i<8*32; ++i) (SRC)[i] = i^0x3e;

#define TEST_TRANSFER2_END(DST)               \
}                                         \
emms();                                   \
t = (gettime_usec()-t -overhead) / nb_tests;\
byte_swap((uint8_t*)(DST), 8*32*sizeof((DST)[0]), sizeof((DST)[0]));  \
s = calc_crc((uint8_t*)(DST), 8*32*sizeof((DST)[0]), CRC32_INITIAL)

#define TEST_TRANSFER2(FUNC, DST, SRC, R1)    \
TEST_TRANSFER2_BEGIN(DST,SRC);            \
  (FUNC)((DST), (SRC), (R1), 32);         \
TEST_TRANSFER2_END(DST)

#define TEST_TRANSFER3(FUNC, DST, SRC, R1, R2)\
TEST_TRANSFER_BEGIN(DST);                 \
  (FUNC)((DST), (SRC), (R1), (R2), 32);   \
TEST_TRANSFER_END(DST)

void test_transfer()
{
	const int nb_tests = 4000*speed_ref;
	int i;
	CPU *cpu;
//	uint8_t  Src8[8*32], Dst8[8*32], Ref1[8*32], Ref2[8*32];
//	int16_t Src16[8*32], Dst16[8*32];
  DECLARE_ALIGNED_MATRIX(Src8, 8, 32, uint8_t, CACHE_LINE);
  DECLARE_ALIGNED_MATRIX(Dst8, 8, 32, uint8_t, CACHE_LINE);
  DECLARE_ALIGNED_MATRIX(Ref1, 8, 32, uint8_t, CACHE_LINE);
  DECLARE_ALIGNED_MATRIX(Ref2, 8, 32, uint8_t, CACHE_LINE);
  DECLARE_ALIGNED_MATRIX(Src16, 8, 32, uint16_t, CACHE_LINE);
  DECLARE_ALIGNED_MATRIX(Dst16, 8, 32, uint16_t, CACHE_LINE);

	printf( "\n ===  test transfer ===\n" );

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		double t, overhead;
		int tst, s;

		if (!init_cpu(cpu))
			continue;

		TEST_TRANSFER(transfer_8to16copy, Dst16, Src8);
		printf("%s - 8to16     %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0x115814bb)?"| ERROR": "");

		TEST_TRANSFER(transfer_16to8copy, Dst8, Src16);
		printf( "%s - 16to8     %.3f usec       crc32=0x%08x %s\n",
				cpu->name, t, s,
				(s!=0xee7ccbb4)?"| ERROR": "");

    /* New functions for field prediction by CK 1.10.2005 */
#pragma NEW8X4
		TEST_TRANSFER(transfer8x4_copy, Dst8, Src8);
		printf("%s - 8to4      %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0xbb9c3db5)?"| ERROR": "");
/* End of new functions */

		TEST_TRANSFER(transfer8x8_copy, Dst8, Src8);
		printf("%s - 8to8      %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0xd37b3295)?"| ERROR": "");

		TEST_TRANSFER(transfer_16to8add, Dst8, Src16);
		printf("%s - 16to8add  %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0xdd817bf4)?"| ERROR": "" );

		TEST_TRANSFER2(transfer_8to16sub, Dst16, Src8, Ref1);
		{
			int s1, s2;
			s1 = calc_crc((uint8_t*)Dst16, 8*32*sizeof(Dst16[0]), CRC32_INITIAL);
			s2 = calc_crc((uint8_t*)Src8, 8*32*sizeof(Src8[0]), CRC32_INITIAL);
			printf("%s - 8to16sub  %.3f usec       crc32(1)=0x%08x crc32(2)=0x%08x %s %s\n",
				   cpu->name, t, s1, s2,
				   (s1!=0xa1e07163)?"| ERROR1": "",
				   (s2!=0xd86c5d23)?"| ERROR2": "" );
		}

		TEST_TRANSFER3(transfer_8to16sub2, Dst16, Src8, Ref1, Ref2);
		printf("%s - 8to16sub2 %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0x99b6c4c7)?"| ERROR": "" );

		printf( " --- \n" );
	}
}

/*********************************************************************
 * test quantization
 *********************************************************************/

#define TEST_QUANT(FUNC, DST, SRC)              \
t = gettime_usec();                         \
for(s=CRC32_INITIAL,qm=1; qm<=255; ++qm) {              \
  for(i=0; i<8*8; ++i) Quant[i] = qm;       \
  set_inter_matrix( mpeg_quant_matrices, Quant );                \
  emms();                                   \
  for(q=1; q<=max_Q; ++q) {                 \
	for(tst=0; tst<nb_tests; ++tst)         \
	  (FUNC)((DST), (SRC), q, mpeg_quant_matrices);              \
	byte_swap((uint8_t*)(DST), 64*sizeof((DST)[0]), sizeof((DST)[0]));  \
	s = calc_crc((uint8_t*)(DST), 64*sizeof((DST)[0]), s); \
  }                                         \
  emms();                                   \
}                                           \
t = (gettime_usec()-t-overhead)/nb_tests/qm

#define TEST_QUANT2(FUNC, DST, SRC)             \
t = gettime_usec();                         \
for(s=CRC32_INITIAL,qm=1; qm<=255; ++qm) {              \
  for(i=0; i<8*8; ++i) Quant[i] = qm;       \
  set_intra_matrix( mpeg_quant_matrices, Quant );                \
  emms();                                   \
  for(q=1; q<=max_Q; ++q) {                 \
	init_intra_matrix( mpeg_quant_matrices, q ); \
	for(tst=0; tst<nb_tests; ++tst)         \
	  (FUNC)((DST), (SRC), q, q, mpeg_quant_matrices);           \
	byte_swap((uint8_t*)(DST), 64*sizeof((DST)[0]), sizeof((DST)[0]));  \
	s = calc_crc((uint8_t*)(DST), 64*sizeof((DST)[0]), s); \
  }                                         \
  emms();                                   \
}                                           \
t = (gettime_usec()-t-overhead)/nb_tests/qm

#define TEST_INTRA(REFFUNC, NEWFUNC, RANGE)              \
{ int32_t i,q,s;\
	DECLARE_ALIGNED_MATRIX(Src, 8, 8, int16_t, 16); \
  DECLARE_ALIGNED_MATRIX(Dst, 8, 8, int16_t, 16); \
  DECLARE_ALIGNED_MATRIX(Dst2,8, 8, int16_t, 16); \
  for(q=1;q<=max_Q;q++)          \
    for(s=-RANGE;s<RANGE;s++) { \
      for(i=0;i<64;i++) Src[i]=s; \
      (REFFUNC)((Dst),(Src),q,q,mpeg_quant_matrices);   \
      (NEWFUNC)((Dst2),(Src),q,q,mpeg_quant_matrices);  \
      for(i=0;i<64;i++)     \
        if(Dst[i]!=Dst2[i]) printf("ERROR : " #NEWFUNC " i%d quant:%d input:%d C_result:%d ASM_result:%d\n",i,q,s,Dst[i],Dst2[i]);  \
    }      \
}

#define TEST_INTER(REFFUNC, NEWFUNC, RANGE)              \
{ int i,q,s;  \
	DECLARE_ALIGNED_MATRIX(Src, 8, 8, int16_t, 16); \
  DECLARE_ALIGNED_MATRIX(Dst, 8, 8, int16_t, 16); \
  DECLARE_ALIGNED_MATRIX(Dst2,8, 8, int16_t, 16); \
  for(q=1;q<=max_Q;q++)  \
    for(s=-RANGE;s<RANGE;s++) {   \
      for(i=0;i<64;i++) Src[i]=s; \
      (REFFUNC)((Dst),(Src),q,mpeg_quant_matrices);  \
      (NEWFUNC)((Dst2),(Src),q,mpeg_quant_matrices); \
      emms();           \
      for(i=0;i<64;i++) \
        if(Dst[i]!=Dst2[i]) printf("ERROR : " #NEWFUNC " i%d quant:%d input:%d C_result:%d ASM_result:%d\n",i,q,s,Dst[i],Dst2[i]); \
    } \
}

void test_quant()
{
	const int32_t nb_tests = 1*speed_ref;
	const int32_t max_Q = 31;
	DECLARE_ALIGNED_MATRIX(mpeg_quant_matrices, 8, 64, uint16_t, 16);

	int32_t i, qm;
	CPU *cpu;
	DECLARE_ALIGNED_MATRIX(Src, 8, 8, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Dst, 8, 8, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Dst2,8, 8, int16_t, 16);
	uint8_t Quant[8*8];

	printf( "\n =====  test quant =====\n" );

/* we deliberately enfringe the norm's specified range [-127,127], */
/* to test the robustness of the iquant module */
	for(i=0; i<64; ++i) {
		Src[i] = 1 + (i-32) * (i&6);
		Dst[i] = 0;
	}

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		double t, overhead;
		int32_t tst, q;
		uint32_t s;

		if (!init_cpu(cpu))
			continue;

		// exhaustive tests to compare against the (ref) C-version
		TEST_INTRA(quant_h263_intra_c,   quant_h263_intra,    2048);
		TEST_INTRA(dequant_h263_intra_c, dequant_h263_intra , 512 );
		TEST_INTER(quant_h263_inter_c,   quant_h263_inter ,   2048);
		TEST_INTER(dequant_h263_inter_c, dequant_h263_inter , 512 );

		overhead = -gettime_usec();
		for(s=0,qm=1; qm<=255; ++qm) {
			for(i=0; i<8*8; ++i) Quant[i] = qm;
			set_inter_matrix(mpeg_quant_matrices, Quant );
			for(q=1; q<=max_Q; ++q)
				for(i=0; i<64; ++i) s+=Dst[i]^i^qm;
		}
		overhead += gettime_usec();

		TEST_QUANT2(quant_mpeg_intra, Dst, Src);
		printf("%s -   quant_mpeg_intra %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0x3b999af6)? "| ERROR": "");

		TEST_QUANT(quant_mpeg_inter, Dst, Src);
		printf("%s -   quant_mpeg_inter %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0xf6de7757)?"| ERROR": "");

		TEST_QUANT2(dequant_mpeg_intra, Dst, Src);
		printf("%s - dequant_mpeg_intra %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0x2def7bc7)?"| ERROR": "");

		TEST_QUANT(dequant_mpeg_inter, Dst, Src);
		printf("%s - dequant_mpeg_inter %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0xd878c722)?"| ERROR": "");

		TEST_QUANT2(quant_h263_intra, Dst, Src);
		printf("%s -   quant_h263_intra %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0x2eba9d43)?"| ERROR": "");

		TEST_QUANT(quant_h263_inter, Dst, Src);
		printf("%s -   quant_h263_inter %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0xbd315a7e)?"| ERROR": "");

		TEST_QUANT2(dequant_h263_intra, Dst, Src);
		printf("%s - dequant_h263_intra %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0x9841212a)?"| ERROR": "");

		TEST_QUANT(dequant_h263_inter, Dst, Src);
		printf("%s - dequant_h263_inter %.3f usec       crc32=0x%08x %s\n",
			   cpu->name, t, s,
			   (s!=0xe7df8fba)?"| ERROR": "");

		printf( " --- \n" );
	}
}

/*********************************************************************
 * test distortion operators
 *********************************************************************/

static void ieee_reseed(long s);
static long ieee_rand(int Min, int Max);

#define TEST_SSE(FUNCTION, SRC1, SRC2, STRIDE) \
  do { \
    t = gettime_usec(); \
    tst = nb_tests; \
    while((tst--)>0) sse = (FUNCTION)((SRC1), (SRC2), (STRIDE)); \
    emms(); \
    t = (gettime_usec() - t)/(double)nb_tests;	\
  } while(0)


void test_sse()
{
	const int nb_tests = 100000*speed_ref;
	int i;
	CPU *cpu;
	DECLARE_ALIGNED_MATRIX(Src1, 8, 8, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Src2, 8, 8, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Src3, 8, 8, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Src4, 8, 8, int16_t, 16);

	printf( "\n =====  test sse =====\n" );

	ieee_reseed(1);
	for(i=0; i<64; ++i) {
		Src1[i] = ieee_rand(-2048, 2047);
		Src2[i] = ieee_rand(-2048, 2047);
		Src3[i] = ieee_rand(-2048, 2047);
		Src4[i] = ieee_rand(-2048, 2047);
	}

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		double t;
		int tst, sse;

		if (!init_cpu(cpu))
			continue;

		/* 16 bit element blocks */
		TEST_SSE(sse8_16bit, Src1, Src2, 16);
		printf("%s -   sse8_16bit#1 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=182013834)?"| ERROR": "");
		TEST_SSE(sse8_16bit, Src1, Src3, 16);
		printf("%s -   sse8_16bit#2 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=142545203)?"| ERROR": "");
		TEST_SSE(sse8_16bit, Src1, Src4, 16);
		printf("%s -   sse8_16bit#3 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=146340935)?"| ERROR": "");
		TEST_SSE(sse8_16bit, Src2, Src3, 16);
		printf("%s -   sse8_16bit#4 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=130136661)?"| ERROR": "");
		TEST_SSE(sse8_16bit, Src2, Src4, 16);
		printf("%s -   sse8_16bit#5 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=136870353)?"| ERROR": "");
		TEST_SSE(sse8_16bit, Src3, Src4, 16);
		printf("%s -   sse8_16bit#6 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=164107772)?"| ERROR": "");

		/* 8 bit element blocks */
		TEST_SSE(sse8_8bit, (int8_t*)Src1, (int8_t*)Src2, 8);
		printf("%s -    sse8_8bit#1 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=1356423)?"| ERROR": "");
		TEST_SSE(sse8_8bit, (int8_t*)Src1, (int8_t*)Src3, 8);
		printf("%s -    sse8_8bit#2 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=1173074)?"| ERROR": "");
		TEST_SSE(sse8_8bit, (int8_t*)Src1, (int8_t*)Src4, 8);
		printf("%s -    sse8_8bit#3 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=1092357)?"| ERROR": "");
		TEST_SSE(sse8_8bit, (int8_t*)Src2, (int8_t*)Src3, 8);
		printf("%s -    sse8_8bit#4 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=1360239)?"| ERROR": "");
		TEST_SSE(sse8_8bit, (int8_t*)Src2, (int8_t*)Src4, 8);
		printf("%s -    sse8_8bit#5 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=1208414)?"| ERROR": "");
		TEST_SSE(sse8_8bit, (int8_t*)Src3, (int8_t*)Src4, 8);
		printf("%s -    sse8_8bit#6 %.3f usec       sse=%d %s\n",
			   cpu->name, t, sse, (sse!=1099285)?"| ERROR": "");

		printf(" ---\n");
	}
}

/*********************************************************************
 * test non-zero AC counting
 *********************************************************************/

#define TEST_CBP(FUNC, SRC, NB)           \
t = gettime_usec();                       \
emms();                                   \
for(tst=0; tst<NB; ++tst) {         \
  cbp = (FUNC)((SRC));                    \
}                                         \
emms();                                   \
t = (gettime_usec()-t ) / nb_tests;

void test_cbp()
{
	const int nb_tests = 10000*speed_ref;
	int i, n, m;
	CPU *cpu;
	DECLARE_ALIGNED_MATRIX(Src1, 6, 64, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Src2, 6, 64, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Src3, 6, 64, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Src4, 6, 64, int16_t, 16);
  DECLARE_ALIGNED_MATRIX(Src5, 6, 64, int16_t, 16);

	printf( "\n =====  test cbp =====\n" );

	for(i=0; i<6*64; ++i) {
		Src1[i] = (i*i*3/8192)&(i/64)&1;  /* 'random' */
		Src2[i] = (i<3*64);               /* half-full */
		Src3[i] = ((i+32)>3*64);
		Src4[i] = (i==(3*64+2) || i==(5*64+9));
    Src5[i] = ieee_rand(0,1) ? -1 : 1;  /* +/- test */
	}

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		double t;
		int tst, cbp;

		if (!init_cpu(cpu))
			continue;

		TEST_CBP(calc_cbp, Src1, nb_tests);
		printf("%s -   calc_cbp#1 %.3f usec       cbp=0x%02x %s\n",
			   cpu->name, t, cbp, (cbp!=0x15)?"| ERROR": "");
		TEST_CBP(calc_cbp, Src2, nb_tests);
		printf("%s -   calc_cbp#2 %.3f usec       cbp=0x%02x %s\n",
			   cpu->name, t, cbp, (cbp!=0x38)?"| ERROR": "");
		TEST_CBP(calc_cbp, Src3, nb_tests);
		printf("%s -   calc_cbp#3 %.3f usec       cbp=0x%02x %s\n",
			   cpu->name, t, cbp, (cbp!=0x0f)?"| ERROR": "" );
		TEST_CBP(calc_cbp, Src4, nb_tests);
		printf("%s -   calc_cbp#4 %.3f usec       cbp=0x%02x %s\n",
			   cpu->name, t, cbp, (cbp!=0x05)?"| ERROR": "" );
		TEST_CBP(calc_cbp, Src5, nb_tests);
		printf("%s -   calc_cbp#4 %.3f usec       cbp=0x%02x %s\n",
			   cpu->name, t, cbp, (cbp!=0x3f)?"| ERROR": "" );
		printf( " --- \n" );
	}

	for(cpu = cpu_list; cpu->name!=0; ++cpu)  /* bench suggested by Carlo (carlo dot bramix at libero dot it) */
	{
		double t;
		int tst, cbp, err;

		if (!init_cpu(cpu))
			continue;

    err = 0;
    for(n=0; n<6; ++n)
    {
      for(m=0; m<64; ++m)
      {
        for(i=0; i<6*64; ++i)
          Src1[i] = (i== (m + n*64));

        TEST_CBP(calc_cbp, Src1, 1);
        if (cbp!= (((m!=0)<<(5-n))))
        {
          printf( "%s -   calc_cbp#5: ERROR at pos %d / %d!\n", cpu->name, n, m);
          err = 1;
          break;
        }
      }
    }
    if (!err)
      printf( " %s -    calc_cbp#5 : OK\n", cpu->name );

	}
}

/*********************************************************************
 * fdct/idct IEEE1180 compliance
 *********************************************************************/

typedef struct {
	long Errors[64];
	long Sqr_Errors[64];
	long Max_Errors[64];
	long Nb;
} STATS_8x8;

void init_stats(STATS_8x8 *S)
{
	int i;
	for(i=0; i<64; ++i) {
		S->Errors[i]     = 0;
		S->Sqr_Errors[i] = 0;
		S->Max_Errors[i] = 0;
	}
	S->Nb = 0;
}

void store_stats(STATS_8x8 *S, short Blk[64], short Ref[64])
{
	int i;
	for(i=0; i<64; ++i)
	{
		short Err = Blk[i] - Ref[i];
		S->Errors[i] += Err;
		S->Sqr_Errors[i] += Err * Err;
		if (Err<0) Err = -Err;
		if (S->Max_Errors[i]<Err)
			S->Max_Errors[i] = Err;
	}
	S->Nb++;
}

void print_stats(STATS_8x8 *S)
{
	int i;
	double Norm;

	assert(S->Nb>0);
	Norm = 1. / (double)S->Nb;
	printf("\n== Max absolute values of errors ==\n");
	for(i=0; i<64; i++) {
		printf("  %4ld", S->Max_Errors[i]);
		if ((i&7)==7) printf("\n");
	}

	printf("\n== Mean square errors ==\n");
	for(i=0; i<64; i++)
	{
		double Err = Norm * (double)S->Sqr_Errors[i];
		printf(" %.3f", Err);
		if ((i&7)==7) printf("\n");
	}

	printf("\n== Mean errors ==\n");
	for(i=0; i<64; i++)
	{
		double Err = Norm * (double)S->Errors[i];
		printf(" %.3f", Err);
		if ((i&7)==7) printf("\n");
	}
	printf("\n");
}

static const char *CHECK(double v, double l) {
	if (fabs(v)<=l) return "ok";
	else return "FAIL!";
}

void report_stats(STATS_8x8 *S, const double *Limits)
{
	int i;
	double Norm, PE, PMSE, OMSE, PME, OME;

	assert(S->Nb>0);
	Norm = 1. / (double)S->Nb;
	PE = 0.;
	for(i=0; i<64; i++) {
		if (PE<S->Max_Errors[i])
			PE = S->Max_Errors[i];
	}

	PMSE = 0.;
	OMSE = 0.;
	for(i=0; i<64; i++)
	{
		double Err = Norm * (double)S->Sqr_Errors[i];
		OMSE += Err;
		if (PMSE < Err) PMSE = Err;
	}
	OMSE /= 64.;

	PME = 0.;
	OME = 0.;
	for(i=0; i<64; i++)
	{
		double Err = Norm * (double)S->Errors[i];
		OME += Err;
		Err = fabs(Err);
		if (PME < Err) PME = Err;
	}
	OME /= 64.;

	printf( "Peak error:   %4.4f\n", PE );
	printf( "Peak MSE:     %4.4f\n", PMSE );
	printf( "Overall MSE:  %4.4f\n", OMSE );
	printf( "Peak ME:      %4.4f\n", PME );
	printf( "Overall ME:   %4.4f\n", OME );

	if (Limits!=0)
	{
		printf( "[PE<=%.4f %s]  ", Limits[0], CHECK(PE,   Limits[0]) );
		printf( "\n" );
		printf( "[PMSE<=%.4f %s]", Limits[1], CHECK(PMSE, Limits[1]) );
		printf( "[OMSE<=%.4f %s]", Limits[2], CHECK(OMSE, Limits[2]) );
		printf( "\n" );
		printf( "[PME<=%.4f %s] ", Limits[3], CHECK(PME , Limits[3]) );
		printf( "[OME<=%.4f %s] ", Limits[4], CHECK(OME , Limits[4]) );
		printf( "\n" );
	}
}

///* ////////////////////////////////////////////////////// */
/* Pseudo-random generator specified by IEEE 1180 */

static long ieee_seed = 1;
static void ieee_reseed(long s) {
	ieee_seed = s;
}
static long ieee_rand(int Min, int Max)
{
	static double z = (double) 0x7fffffff;

	long i,j;
	double x;

	ieee_seed = (ieee_seed * 1103515245) + 12345;
	i = ieee_seed & 0x7ffffffe;
	x = ((double) i) / z;
	x *= (Max-Min+1);
	j = (long)x;
	j = j + Min;
	assert(j>=Min && j<=Max);
	return (short)j;
}

#define CLAMP(x, M)   (x) = ((x)<-(M)) ? (-(M)) : ((x)>=(M) ? ((M)-1) : (x))

static double Cos[8][8];
static void init_ref_dct()
{
	int i, j;
	for(i=0; i<8; i++)
	{
		double scale = (i == 0) ? sqrt(0.125) : 0.5;
		for (j=0; j<8; j++)
			Cos[i][j] = scale*cos( (M_PI/8.0)*i*(j + 0.5) );
	}
}

void ref_idct(short *M)
{
	int i, j, k;
	double Tmp[8][8];

	for(i=0; i<8; i++) {
		for(j=0; j<8; j++)
		{
			double Sum = 0.0;
			for (k=0; k<8; k++) Sum += Cos[k][j]*M[8*i+k];
			Tmp[i][j] = Sum;
		}
	}
	for(i=0; i<8; i++) {
		for(j=0; j<8; j++) {
			double Sum = 0.0;
			for (k=0; k<8; k++) Sum += Cos[k][i]*Tmp[k][j];
			M[8*i+j] = (short)floor(Sum + .5);
		}
	}
}

void ref_fdct(short *M)
{
	int i, j, k;
	double Tmp[8][8];

	for(i=0; i<8; i++) {
		for(j=0; j<8; j++)
		{
			double Sum = 0.0;
			for (k=0; k<8; k++) Sum += Cos[j][k]*M[8*i+k];
			Tmp[i][j] = Sum;
		}
	}
	for(i=0; i<8; i++) {
		for(j=0; j<8; j++) {
			double Sum = 0.0;
			for (k=0; k<8; k++) Sum += Cos[i][k]*Tmp[k][j];
			M[8*i+j] = (short)floor(Sum + 0.5);
		}
	}
}

void test_IEEE1180_compliance(int Min, int Max, int Sign)
{
	static const double ILimits[5] = { 1., 0.06, 0.02, 0.015, 0.0015 };
	int Loops = 10000;
	int i, m, n;
	DECLARE_ALIGNED_MATRIX(Blk0, 8, 8, short, 16); /* reference */
	DECLARE_ALIGNED_MATRIX(Blk,  8, 8, short, 16);
	DECLARE_ALIGNED_MATRIX(iBlk, 8, 8, short, 16);
	DECLARE_ALIGNED_MATRIX(Ref_FDCT, 8, 8, short, 16);
	DECLARE_ALIGNED_MATRIX(Ref_IDCT, 8, 8, short, 16);

	STATS_8x8 FStats; /* forward dct stats */
	STATS_8x8 IStats; /* inverse dct stats */

	CPU *cpu;

	init_ref_dct();

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		if (!init_cpu(cpu))
			continue;

		printf( "\n===== IEEE test for %s ==== (Min=%d Max=%d Sign=%d Loops=%d)\n",
				cpu->name, Min, Max, Sign, Loops);

		init_stats(&IStats);
		init_stats(&FStats);

		ieee_reseed(1);
		for(n=0; n<Loops; ++n)
		{
			for(i=0; i<64; ++i)
				Blk0[i] = (short)ieee_rand(Min,Max) * Sign;

			/* hmm, I'm not quite sure this is exactly */
			/* the tests described in the norm. check... */

			memcpy(Ref_FDCT, Blk0, 64*sizeof(short));
			ref_fdct(Ref_FDCT);
			for(i=0; i<64; i++) CLAMP( Ref_FDCT[i], 2048 );

			memcpy(Blk, Blk0, 64*sizeof(short));
			emms(); fdct(Blk); emms();
			for(i=0; i<64; i++) CLAMP( Blk[i], 2048 );

			store_stats(&FStats, Blk, Ref_FDCT);


			memcpy(Ref_IDCT, Ref_FDCT, 64*sizeof(short));
			ref_idct(Ref_IDCT);
			for (i=0; i<64; i++) CLAMP( Ref_IDCT[i], 256 );

			memcpy(iBlk, Ref_FDCT, 64*sizeof(short));
			emms(); idct(iBlk); emms();
			for(i=0; i<64; i++) CLAMP( iBlk[i], 256 );

			store_stats(&IStats, iBlk, Ref_IDCT);
		}


		printf( "\n  -- FDCT report --\n" );
//    print_stats(&FStats);
		report_stats(&FStats, 0); /* so far I know, IEEE1180 says nothing for fdct */

		for(i=0; i<64; i++) Blk[i] = 0;
		emms(); fdct(Blk); emms();
		for(m=i=0; i<64; i++) if (Blk[i]!=0) m++;
		printf( "FDCT(0) == 0 ?  %s\n", (m!=0) ? "NOPE!" : "yup." );

		printf( "\n  -- IDCT report --\n" );
//    print_stats(&IStats);
		report_stats(&IStats, ILimits);


		for(i=0; i<64; i++) Blk[i] = 0;
		emms(); idct(Blk); emms();
		for(m=i=0; i<64; i++) if (Blk[i]!=0) m++;
		printf( "IDCT(0) == 0 ?  %s\n", (m!=0) ? "NOPE!" : "yup." );
	}
}


void test_dct_saturation(int Min, int Max)
{
/* test behaviour on input range fringe */

	int i, n, p;
	CPU *cpu;
//  const short IDCT_MAX =  2047;  /* 12bits input */
//  const short IDCT_MIN = -2048;
//  const short IDCT_OUT =   256;  /* 9bits ouput */
	const int Partitions = 4;
	const int Loops = 10000 / Partitions;

	init_ref_dct();

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		short Blk0[64], Blk[64];
		STATS_8x8 Stats;

		if (!init_cpu(cpu))
			continue;

		printf( "\n===== IEEE test for %s Min=%d Max=%d =====\n",
				cpu->name, Min, Max );

		/* FDCT tests // */

		init_stats(&Stats);

		/* test each computation channels separately */
		for(i=0; i<64; i++) Blk[i] = Blk0[i] = ((i/8)==(i%8)) ? Max : 0;
		ref_fdct(Blk0);
		emms(); fdct(Blk); emms();
		store_stats(&Stats, Blk, Blk0);

		for(i=0; i<64; i++) Blk[i] = Blk0[i] = ((i/8)==(i%8)) ? Min : 0;
		ref_fdct(Blk0);
		emms(); fdct(Blk); emms();
		store_stats(&Stats, Blk, Blk0);

		/* randomly saturated inputs */
		for(p=0; p<Partitions; ++p)
		{
			for(n=0; n<Loops; ++n)
			{
				for(i=0; i<64; ++i)
					Blk0[i] = Blk[i] = (ieee_rand(0,Partitions)>=p)? Max : Min;
				ref_fdct(Blk0);
				emms(); fdct(Blk); emms();
				store_stats(&Stats, Blk, Blk0);
			}
		}
		printf( "\n  -- FDCT saturation report --\n" );
		report_stats(&Stats, 0);


		/* IDCT tests // */
#if 0
		/* no finished yet */

		init_stats(&Stats);

/* test each computation channel separately */
		for(i=0; i<64; i++) Blk[i] = Blk0[i] = ((i/8)==(i%8)) ? IDCT_MAX : 0;
		ref_idct(Blk0);
		emms(); idct(Blk); emms();
		for(i=0; i<64; i++) { CLAMP(Blk0[i], IDCT_OUT); CLAMP(Blk[i], IDCT_OUT); }
		store_stats(&Stats, Blk, Blk0);

		for(i=0; i<64; i++) Blk[i] = Blk0[i] = ((i/8)==(i%8)) ? IDCT_MIN : 0;
		ref_idct(Blk0);
		emms(); idct(Blk); emms();
		for(i=0; i<64; i++) { CLAMP(Blk0[i], IDCT_OUT); CLAMP(Blk[i], IDCT_OUT); }
		store_stats(&Stats, Blk, Blk0);

		/* randomly saturated inputs */
		for(p=0; p<Partitions; ++p)
		{
			for(n=0; n<Loops; ++n)
			{
				for(i=0; i<64; ++i)
					Blk0[i] = Blk[i] = (ieee_rand(0,Partitions)>=p)? IDCT_MAX : IDCT_MIN;
				ref_idct(Blk0);
				emms(); idct(Blk); emms();
				for(i=0; i<64; i++) { CLAMP(Blk0[i],IDCT_OUT); CLAMP(Blk[i],IDCT_OUT); }
				store_stats(&Stats, Blk, Blk0);
			}
		}

		printf( "\n  -- IDCT saturation report --\n" );
		print_stats(&Stats);
		report_stats(&Stats, 0);
#endif
	}
}

/*********************************************************************
 * measure raw decoding speed
 *********************************************************************/

void test_dec(const char *name, int width, int height, int ref_chksum)
{
	FILE *f = 0;
	void *dechandle = 0;
	int xerr;
	xvid_gbl_init_t xinit;
	xvid_dec_create_t xparam;
	xvid_dec_frame_t xframe;
	double t = 0.;
	int nb = 0;
	uint8_t *buf = 0;
	uint8_t *yuv_out = 0;
	int buf_size, pos;
	uint32_t chksum = 0;
	int bps = (width+31) & ~31;

	memset(&xinit, 0, sizeof(xinit));
	xinit.cpu_flags = cpu_mask;
	xinit.version = XVID_VERSION;
	xvid_global(NULL, 0, &xinit, NULL);

	memset(&xparam, 0, sizeof(xparam));
	xparam.width  = width;
	xparam.height = height;
	xparam.version = XVID_VERSION;
	xerr = xvid_decore(NULL, XVID_DEC_CREATE, &xparam, NULL);
	if (xerr==XVID_ERR_FAIL) {
		printf("ERROR: can't init decoder (err=%d)\n", xerr);
		return;
	}
	dechandle = xparam.handle;


	f = fopen(name, "rb");
	if (f==0) {
		printf( "ERROR: can't open file '%s'\n", name);
		return;
	}
	fseek(f, 0, SEEK_END);
	buf_size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (buf_size<=0) {
		printf("ERROR: error while stating file\n");
		goto End;
	}

	buf = malloc(buf_size);
	yuv_out = calloc(1, bps*height*3/2 + 15);
	if (buf==0 || yuv_out==0) {
		printf( "ERROR: malloc failed!\n" );
		goto End;
	}

	if (fread(buf, buf_size, 1, f)!=1) {
		printf( "ERROR: file-read failed\n" );
		goto End;
	}

	nb = 0;
	pos = 0;
	t = -gettime_usec();
	while(1) {
	  int y;

		memset(&xframe, 0, sizeof(xframe));
		xframe.version = XVID_VERSION;
		xframe.bitstream = buf + pos;
		xframe.length = buf_size - pos;
		xframe.output.plane[0] = (uint8_t*)(((size_t)yuv_out + 15) & ~15);
		xframe.output.plane[1] = (uint8_t*)xframe.output.plane[0] + bps*height;
		xframe.output.plane[2] = (uint8_t*)xframe.output.plane[1] + bps/2;
		xframe.output.stride[0] = bps;
		xframe.output.stride[1] = bps;
		xframe.output.stride[2] = bps;
		xframe.output.csp = XVID_CSP_I420;
		xerr = xvid_decore(dechandle, XVID_DEC_DECODE, &xframe, 0);
		if (xerr<0) {
			printf("ERROR: decoding failed for frame #%d (err=%d)!\n", nb, xerr);
			break;
		}
		else if (xerr==0)
		  break;
    else if (verbose>0) printf("#%d %d\n", nb, xerr );

		pos += xerr;
		nb++;

    for(y=0; y<height/2; ++y) {
		  chksum = calc_crc((uint8_t*)xframe.output.plane[0] + (2*y+0)*bps, width, chksum);
			chksum = calc_crc((uint8_t*)xframe.output.plane[0] + (2*y+1)*bps, width, chksum);
			chksum = calc_crc((uint8_t*)xframe.output.plane[1] + y*bps, width/2, chksum);
			chksum = calc_crc((uint8_t*)xframe.output.plane[2] + y*bps, width/2, chksum);
		}
		if (pos==buf_size)
			break;
	}
	t += gettime_usec();
	if (ref_chksum==0) {
	  if (t>0.)
		  printf( "%d frames decoded in %.3f s -> %.1f FPS   Checksum:0x%.8x\n", nb, t*1.e-6f, (float)(nb*1.e6f/t), chksum );
  }
  else {    
		printf("FPS:%.1f Checksum: 0x%.8x Expected:0x%.8x | %s\n", 
		  t>0. ? (float)(nb*1.e6f/t) : 0.f, chksum, ref_chksum, (chksum==ref_chksum) ? "OK" : "ERROR");
  }

 End:
	if (yuv_out!=0) free(yuv_out);
	if (buf!=0) free(buf);
	if (dechandle!=0) {
		xerr= xvid_decore(dechandle, XVID_DEC_DESTROY, NULL, NULL);
		if (xerr==XVID_ERR_FAIL)
			printf("ERROR: destroy-decoder failed (err=%d)!\n", xerr);
	}
	if (f!=0) fclose(f);
}

/*********************************************************************
 * non-regression tests
 *********************************************************************/

void test_bugs1()
{
	CPU *cpu;
	uint16_t mpeg_quant_matrices[64*8];

	printf( "\n =====  (de)quant4_intra saturation bug? =====\n" );

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		int i;
		int16_t  Src[8*8], Dst[8*8];

		if (!init_cpu(cpu))
			continue;

		for(i=0; i<64; ++i) Src[i] = i-32;
		set_intra_matrix( mpeg_quant_matrices, get_default_intra_matrix() );
		dequant_mpeg_intra(Dst, Src, 31, 5, mpeg_quant_matrices);
		printf( "dequant_mpeg_intra with CPU=%s:  ", cpu->name);
		printf( "  Out[]= " );
		for(i=0; i<64; ++i) printf( "[%d]", Dst[i]);
		printf( "\n" );
	}

	printf( "\n =====  (de)quant4_inter saturation bug? =====\n" );

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		int i;
		int16_t  Src[8*8], Dst[8*8];

		if (!init_cpu(cpu))
			continue;

		for(i=0; i<64; ++i) Src[i] = i-32;
		set_inter_matrix( mpeg_quant_matrices, get_default_inter_matrix() );
		dequant_mpeg_inter(Dst, Src, 31, mpeg_quant_matrices);
		printf( "dequant_mpeg_inter with CPU=%s:  ", cpu->name);
		printf( "  Out[]= " );
		for(i=0; i<64; ++i) printf( "[%d]", Dst[i]);
		printf( "\n" ); 
	}
}

void test_dct_precision_diffs()
{
	CPU *cpu;
	DECLARE_ALIGNED_MATRIX(Blk, 8, 8, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Blk0, 8, 8, int16_t, 16);

	printf( "\n =====  fdct/idct precision diffs =====\n" );

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		int i;

		if (!init_cpu(cpu))
			continue;

		for(i=0; i<8*8; ++i) {
			Blk0[i] = (i*7-i*i) & 0x7f;
			Blk[i] = Blk0[i];
		}

		fdct(Blk);
		idct(Blk);
		printf( " fdct+idct diffs with CPU=%s: \n", cpu->name );
		for(i=0; i<8; ++i) {
			int j;
			for(j=0; j<8; ++j) printf( " %d ", Blk[i*8+j]-Blk0[i*8+j]); 
			printf("\n"); 
		}
		printf("\n"); 
	}
}

void test_quant_bug()
{
	const int max_Q = 31;
	int i, n, qm, q;
	CPU *cpu;
	DECLARE_ALIGNED_MATRIX(Src, 8, 8, int16_t, 16);
	DECLARE_ALIGNED_MATRIX(Dst, 8, 8, int16_t, 16);
	uint8_t Quant[8*8];
	CPU cpu_bug_list[] = { { "PLAINC", 0 }, { "MMX   ", XVID_CPU_MMX }, {0,0} };
	uint16_t Crcs_Inter[2][32];
	uint16_t Crcs_Intra[2][32];
	DECLARE_ALIGNED_MATRIX(mpeg_quant_matrices, 8, 64, uint16_t, 16);

	printf( "\n =====  test MPEG4-quantize bug =====\n" );

	for(i=0; i<64; ++i) Src[i] = 2048*(i-32)/32;

#if 1
	for(qm=1; qm<=255; ++qm)
	{
		for(i=0; i<8*8; ++i) Quant[i] = qm;
		set_inter_matrix( mpeg_quant_matrices, Quant );

		for(n=0, cpu = cpu_bug_list; cpu->name!=0; ++cpu, ++n)
		{
			uint16_t s;

			if (!init_cpu(cpu))
				continue;

			for(q=1; q<=max_Q; ++q) {
				emms();
				quant_mpeg_inter( Dst, Src, q, mpeg_quant_matrices );
				emms();
				for(s=0, i=0; i<64; ++i) s+=((uint16_t)Dst[i])^i;
				Crcs_Inter[n][q] = s;
			}
		}

		for(q=1; q<=max_Q; ++q)
			for(i=0; i<n-1; ++i)
				if (Crcs_Inter[i][q]!=Crcs_Inter[i+1][q])
					printf( "Discrepancy Inter: qm=%d, q=%d  -> %d/%d !\n",
							qm, q, Crcs_Inter[i][q], Crcs_Inter[i+1][q]);
	}
#endif

#if 1
	for(qm=1; qm<=255; ++qm)
	{
		for(i=0; i<8*8; ++i) Quant[i] = qm;
		set_intra_matrix( mpeg_quant_matrices, Quant );

		for(n=0, cpu = cpu_bug_list; cpu->name!=0; ++cpu, ++n)
		{
			uint16_t s;

			if (!init_cpu(cpu))
				continue;

			for(q=1; q<=max_Q; ++q) {
				emms();
				quant_mpeg_intra( Dst, Src, q, q, mpeg_quant_matrices);
				emms();
				for(s=0, i=0; i<64; ++i) s+=((uint16_t)Dst[i])^i;
				Crcs_Intra[n][q] = s;
			}
		}

		for(q=1; q<=max_Q; ++q)
			for(i=0; i<n-1; ++i)
				if (Crcs_Intra[i][q]!=Crcs_Intra[i+1][q])
					printf( "Discrepancy Intra: qm=%d, q=%d  -> %d/%d!\n",
							qm, q, Crcs_Inter[i][q], Crcs_Inter[i+1][q]);
	}
#endif
}

/*********************************************************************
 * test some YUV func
 *********************************************************************/
  
#define ENTER \
for(i=0; i<(int)sizeof(Dst0); ++i) Dst0[0][i] = 0;   \
t = gettime_usec();                   \
emms();

#define LEAVE \
emms();                             \
t = (gettime_usec() - t) / nb_tests;  \
	iCrc = calc_crc((uint8_t*)Dst0, sizeof(Dst0), CRC32_INITIAL)

#define TEST_YUYV(FUNC, S, FLIP)                \
ENTER                               \
for(tst=0; tst<nb_tests; ++tst) (FUNC)(Dst0[0], S*WIDTH, Src0[0], Src0[1], Src0[2], WIDTH, WIDTH/2, WIDTH, HEIGHT, (FLIP)); \
LEAVE

static const int yuv_CRCs[6][2] = {
	{0x0f4fb96b,0x780b6a68}
,	{0xa986b289,0x65e49b76}
,	{0x7f19c152,0xd539b86e}
,	{0x0f4fb96b,0x780b6a68}
,	{0xa986b289,0x65e49b76}
,	{0x36ab8b57,0x1cd92fee}
};

#define WIDTH 128
#define HEIGHT 32
void test_yuv()
{
	const int nb_tests = 200*speed_ref;
	CPU *cpu;
	uint8_t Src0[3][WIDTH*HEIGHT];
	uint8_t Dst0[4][WIDTH*HEIGHT];
	int i, j, with_flip;
	double t;
	int tst, iCrc;

	colorspace_init();
	ieee_reseed(1);
	for(i=0; i<(int)sizeof(Src0); ++i) Src0[0][i] = ieee_rand(0,255);
	for(i=0; i<(int)sizeof(Dst0); ++i) Dst0[0][i] = 0x5a;
	
        printf( "\n ===  test YUV ===\n" );

        for(with_flip=0; with_flip<=1; ++with_flip) {

		init_cpu(&cpu_list[0]);
		TEST_YUYV(yv12_to_yuyv_c, 4, with_flip);
		printf(" yv12_to_yuyv_c %.3f usec       crc32=0x%08x %s\n",
			   t, iCrc, (iCrc!=yuv_CRCs[0][with_flip])?"| ERROR": "" );
		TEST_YUYV(yv12_to_uyvy_c, 4, with_flip);
		printf(" yv12_to_uyvy_c %.3f usec       crc32=0x%08x %s\n",
		   	t, iCrc, (iCrc!=yuv_CRCs[1][with_flip])?"| ERROR": "" );
 
        	TEST_YUYV(yv12_to_bgra_c, 4, with_flip);
        	printf(" yv12_to_bgra_c %.3f usec       crc32=0x%08x %s\n",
               		t, iCrc, (iCrc!=yuv_CRCs[2][with_flip])?"| ERROR": "" );

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
		init_cpu(&cpu_list[1]);
		TEST_YUYV(yv12_to_yuyv_mmx, 4, with_flip);
		printf(" yv12_to_yuyv_mmx %.3f usec       crc32=0x%08x %s\n",
			t, iCrc, (iCrc!=yuv_CRCs[3][with_flip])?"| ERROR": "" );

		TEST_YUYV(yv12_to_uyvy_mmx, 4, with_flip);
		printf(" yv12_to_uyvy_mmx %.3f usec       crc32=0x%08x %s\n",
			t, iCrc, (iCrc!=yuv_CRCs[4][with_flip])?"| ERROR": "" );

        	TEST_YUYV(yv12_to_bgra_mmx, 4, with_flip);
        	printf(" yv12_to_bgra_mmx %.3f usec       crc32=0x%08x %s\n",
                	t, iCrc, (iCrc!=yuv_CRCs[5][with_flip])?"| ERROR": "" );

#endif

#ifdef ARCH_IS_PPC
		init_cpu(&cpu_list[1]);
		TEST_YUYV(yv12_to_yuyv_altivec_c, 4, with_flip);
		printf(" yv12_to_yuyv_altivec_c %.3f usec       crc32=0x%08x %s\n",
			t, iCrc, (iCrc!=yuv_CRCs[3][with_flip])?"| ERROR": "" );

		TEST_YUYV(yv12_to_uyvy_altivec_c, 4, with_flip);
		printf(" yv12_to_uyvy_altivec_c %.3f usec       crc32=0x%08x %s\n",
			t, iCrc, (iCrc!=yuv_CRCs[4][with_flip])?"| ERROR": "" );

         	TEST_YUYV(yv12_to_bgra_altivec_c, 4, with_flip);
		printf(" yv12_to_bgra_altivec_c %.3f usec       crc32=0x%08x %s\n",
                        t, iCrc, (iCrc!=yuv_CRCs[5][with_flip])?"| ERROR": "" );
#endif
	}
	printf( " --- \n" );
}

#define TEST_YV2(FUNC, WITH_UV, WITH_FLIP)        \
ENTER                               \
for(tst=0; tst<nb_tests; ++tst) (FUNC)(Dst0[0], Dst0[1], Dst0[2], WIDTH, WIDTH, \
	Src0[0], (WITH_UV) ? Src0[1] : 0, (WITH_UV) ? Src0[2] : 0,  WIDTH, WIDTH, \
	WIDTH-2, HEIGHT-2, WITH_FLIP); \
LEAVE

#define PRINT_NxN(DATA,W,H,STR)   {   \
	int i,j; \
	for(j=0; j<(H); ++j) { \
		for(i=0; i<(W); ++i) printf( "0x%.2x ", (DATA)[i+j*(STR)] );\
		printf("\n"); \
	} \
	printf("---\n"); \
}

static const int yv12_CRCs[2][2] = { 
	{0x5cab7cf0,0xdab46541} 
,       {0xe8bae865,0x1faf77b7}
};

void test_yuv2()
{
	const int nb_tests = 800*speed_ref;
	CPU *cpu;
	uint8_t Src0[3][WIDTH*HEIGHT];
	uint8_t Dst0[3][WIDTH*HEIGHT];
	int with_uv, with_flip;
	int i, j;
	double t;
	int tst, iCrc;

	colorspace_init();
	ieee_reseed(1);
	for(i=0; i<(int)sizeof(Src0); ++i) Src0[0][i] = ieee_rand(0,255);

	printf( "\n ===  test YV2 ===\n" );
        for(with_flip=0; with_flip<=1; ++with_flip) {
        	for(with_uv=0; with_uv<=1; ++with_uv) {
			init_cpu(&cpu_list[0]);
			TEST_YV2(yv12_to_yv12_c, with_uv, with_flip);
			printf(" yv12_to_yv12_c   %.3f usec      \tcrc32=0x%08x %s\n",
				t, iCrc, (iCrc!=yv12_CRCs[with_flip][with_uv])?"| ERROR": "" );
			/* if (!with_uv) PRINT_NxN(Dst0[1], WIDTH/2, HEIGHT/2, WIDTH ); */

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
			init_cpu(&cpu_list[1]);
			TEST_YV2(yv12_to_yv12_mmx, with_uv, with_flip);
			printf(" yv12_to_yv12_mmx %.3f usec     \tcrc32=0x%08x %s\n",
				t, iCrc, (iCrc!=yv12_CRCs[with_flip][with_uv])?"| ERROR": "" );
			/* if (!with_uv) PRINT_NxN(Dst0[1], WIDTH/2, HEIGHT/2, WIDTH ); */

			TEST_YV2(yv12_to_yv12_xmm, with_uv, with_flip);
			printf(" yv12_to_yv12_xmm %.3f usec     \tcrc32=0x%08x %s\n",
				t, iCrc, (iCrc!=yv12_CRCs[with_flip][with_uv])?"| ERROR": "" );
#endif
		}

		printf( " --- \n" );
	}
	printf( " ===== \n" );
}

#undef WIDTH
#undef HEIGHT
#undef ENTER
#undef LEAVE

/*********************************************************************/

static uint32_t __inline log2bin_v1(uint32_t value)
{
  int n = 0;
  while (value) {
    value >>= 1;
    n++;
  }
  return n;
}

static const uint8_t log2_tab_16[16] =  { 0, 1, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4 };

static uint32_t __inline log2bin_v2(uint32_t value)
{
  int n = 0;
  if (value & 0xffff0000) {
    value >>= 16;
    n += 16;
  }
  if (value & 0xff00) {
    value >>= 8;
    n += 8;
  }
  if (value & 0xf0) {
    value >>= 4;
    n += 4;
  }
 return n + log2_tab_16[value];
}

void test_log2bin()
{
	const int nb_tests = 3000*speed_ref;
  int n, crc1=0, crc2=0;
  uint32_t s, s0;
  double t1, t2;

  t1 = gettime_usec();
  s0 = (int)(t1*31.241);
  for(s=s0, n=0; n<nb_tests; ++n, s=(s*12363+31)&0x7fffffff)
    crc1 += log2bin_v1(s);
  t1 = (gettime_usec()-t1) / nb_tests;

  t2 = gettime_usec();
  for(s=s0, n=0; n<nb_tests; ++n, s=(s*12363+31)&0x7fffffff)
    crc2 += log2bin_v2(s);
  t2 = (gettime_usec() - t2) / nb_tests;

  printf( "log2bin_v1: %.3f sec  crc=%d\n", t1, crc1 );
  printf( "log2bin_v2: %.3f sec  crc=%d\n", t2, crc2 );
  if (crc1!=crc2) printf( " CRC ERROR !\n" );
}

/*********************************************************************/

static void __inline old_gcd(int *num, int *den)
{
 int i = *num;
  while (i > 1) {   
    if (*num % i == 0 && *den % i == 0) {
      *num /= i;   
      *den /= i;
      i = *num;
      continue;
    }
    i--;
  }
}

static uint32_t gcd(int num, int den)
{
  int tmp;
  while( (tmp=num%den) ) { num = den; den = tmp; }
  return den;
}
static void __inline new_gcd(int *num, int *den)
{
  const int div = gcd(*num, *den);
  if (num) {
    *num /= div;
    *den /= div;
  }
}

void test_gcd()
{
  const int nb_tests = 10*speed_ref;
  int i;
  uint32_t crc1=0, crc2=0;
  uint32_t n0, n, d0, d;
  double t1, t2;

  t1 = gettime_usec();
  n0 = 0xfffff & (int)(t1*31.241);
  d0 = 0xfffff & (int)( ((n0*4123)%17) | 1 );
  for(n=n0, d=d0, i=0; i<nb_tests; ++i) {
    old_gcd(&n, &d);
    crc1 = (((crc1>>4)^d) + ((crc1<<2)^n) ) & 0xffffff;
    n = d;
    d = (d*12363+31) & 0xffff;
    d |= !d;
  }
  t1 = (gettime_usec()-t1) / nb_tests;

  t2 = gettime_usec();
  for(n=n0, d=d0, i=0; i<nb_tests; ++i) {
    new_gcd(&n, &d);
    crc2 = (((crc2>>4)^d) + ((crc2<<2)^n) ) & 0xffffff;
    n = d;
    d = (d*12363+31) & 0xffff;
    d |= !d;
  }
  t2 = (gettime_usec() - t2) / nb_tests;

  printf( "old_gcd: %.3f sec  crc=%d\n", t1, crc1 );
  printf( "new_gcd: %.3f sec  crc=%d\n", t2, crc2 );
  if (crc1!=crc2) printf( " CRC ERROR !\n" );
}

/*********************************************************************
 * test compiler
 *********************************************************************/

void test_compiler() {
  int nb_err = 0;
  int32_t v;
  if (sizeof(uint16_t)<2) {
    printf( "ERROR: sizeof(uint16_t)<2 !!\n" );
    nb_err++;
  }
  if (sizeof(int16_t)<2) {
    printf( "ERROR: sizeof(int16_t)<2 !!\n" );
    nb_err++;
  }
  if (sizeof(uint8_t)!=1) {
    printf( "ERROR: sizeof(uint8_t)!=1 !!\n" );
    nb_err++;
  }
  if (sizeof(int8_t)!=1) {
    printf( "ERROR: sizeof(int8_t)!=1 !!\n" );
    nb_err++;
  }
  if (sizeof(uint32_t)<4) {
    printf( "ERROR: sizeof(uint32_t)<4 !!\n" );
    nb_err++;
  }
  if (sizeof(int32_t)<4) {
    printf( "ERROR: sizeof(int32_t)<4 !!\n" );
    nb_err++;
  }
         /* yes, i know, this test is silly. But better be safe than sorry. :) */
  for(v=1000; v>=0; v--) {
    if ( (v>>2) != v/4)
      nb_err++;
  }
  for(v=-1000; v!=-1; v++) {
    if ( (v>>2) != (v/4)-!!(v%4))
      nb_err++;
  }
  if (nb_err!=0) {
    printf( "ERROR! please post your platform/compiler specs to xvid-devel@xvid.org !\n" );
  }
}

/*********************************************************************
 * test SSIM functions
 *********************************************************************/

typedef int (*lumfunc)(uint8_t* ptr, int stride);
typedef void (*csfunc)(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);

extern int lum_8x8_c(uint8_t* ptr, int stride);
extern int lum_8x8_mmx(uint8_t* ptr, int stride);
extern int lum_2x8_c(uint8_t* ptr, int stride);
extern void consim_c(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);
extern void consim_mmx(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);
extern void consim_sse2(uint8_t* ptro, uint8_t* ptrc, int stride, int lumo, int lumc, int* pdevo, int* pdevc, int* pcorr);

void test_SSIM()
{
	const int nb_tests = 3000*speed_ref;
	int tst;
	CPU *cpu;
	int i;
	int devs[3];
	long lumo, lumc;
	DECLARE_ALIGNED_MATRIX(Ref1, 16, 16, uint8_t, 16);
	DECLARE_ALIGNED_MATRIX(Ref2, 16, 16, uint8_t, 16);
	lumfunc lum8x8;
	lumfunc lum2x8;
	csfunc  csim;

	ieee_reseed(1);
	printf( "\n ======  test SSIM ======\n" );
	for(i=0; i<16*16;++i) {
		long v1, v2;
		v1 = ieee_rand(-256, 511);
		v2 = ieee_rand(-256, 511);
		Ref1[i] = (v1<0) ? 0 : (v1>255) ? 255 : v1;
		Ref2[i] = (v2<0) ? 0 : (v2>255) ? 255 : v2;
	}
	lumc = ieee_rand(0, 255);
	lumo = ieee_rand(0, 255);

	for(cpu = cpu_list; cpu->name!=0; ++cpu)
	{
		double t;
		int m;
		if (!init_cpu(cpu))
			continue;
		lum8x8 = lum_8x8_c;
		lum2x8 = lum_2x8_c;
		csim   = consim_c;
#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
		if (cpu->cpu & XVID_CPU_MMX){
			lum8x8 = lum_8x8_mmx;
			csim = consim_mmx;
		}
		if (cpu->cpu & XVID_CPU_MMX){
			csim = consim_sse2;
		}
#endif
		t = gettime_usec();
		emms();
		for(tst=0; tst<nb_tests; ++tst) m = lum8x8(Ref1, 16);
		emms();
		t = (gettime_usec() - t) / nb_tests;
		printf("%s - ssim-lum8x8    %.3f usec       m=%d %s\n",
			   cpu->name, t, m,
			   (m!=8230)?"| ERROR": "" );

		t = gettime_usec();
		emms();
		for(tst=0; tst<nb_tests; ++tst) m = lum2x8(Ref1+8, 16);
		emms();
		t = (gettime_usec() - t) / nb_tests;
		printf("%s - ssim-lum2x8    %.3f usec       m=%d %s\n",
			   cpu->name, t, m,
			   (m!=681)?"| ERROR": "" );

		t = gettime_usec();
		emms();
		for(tst=0; tst<nb_tests; ++tst) csim(Ref1, Ref2, 16, lumo, lumc, devs+0, devs+1, devs+2);
		emms();
		t = (gettime_usec() - t) / nb_tests;
		printf("%s - ssim-consim    %.3f usec       devs=[0x%x 0x%x 0x%x] %s\n",
			   cpu->name, t, devs[0], devs[1], devs[2],
			   (devs[0]!=0x1bdf0f || devs[1]!=0x137258 ||  devs[2]!=0xcdb13)?"| ERROR": "" );
		printf( " --- \n" );
	}
}

/*********************************************************************
 * test bitstream functions
 *********************************************************************/

#define BIT_BUF_SIZE 2000

static void test_bits()
{
  const int nb_tests = 50*speed_ref;
  int tst;
  uint32_t Crc;
  uint8_t Buf[BIT_BUF_SIZE];
  uint32_t Extracted[BIT_BUF_SIZE*8]; /* worst case: bits read 1 by 1 */
  int Lens[BIT_BUF_SIZE*8];     
  double t1;


  printf( "\n ===  test bitstream ===\n" );
  ieee_reseed(1);
  Crc = 0;

  t1 = gettime_usec();
  for(tst=0; tst<nb_tests; ++tst) {
  	Bitstream bs;
  	int m, m2, l, l2;

	for(l=0; l<BIT_BUF_SIZE; ++l)
		Buf[l] = (uint8_t)ieee_rand(0,255);

  	l = BIT_BUF_SIZE - ieee_rand(1,BIT_BUF_SIZE/10);
  	BitstreamInit(&bs, (void*)(Buf+BIT_BUF_SIZE-l), l);


	BitstreamReset(&bs);
	for(l2=l*8, m=0; l2>0; m++) {
		const int b = ieee_rand(1,32);
		Lens[m] = b;
		l2 -= b;
		if (l2<0) break;
		Extracted[m] = BitstreamShowBits(&bs, b);
		BitstreamSkip(&bs, b);
//		printf( "<= %d: %d 0x%x\n", m, b, Extracted[m]);
	}

	BitstreamReset(&bs);
	for(m2=0; m2<m; ++m2) {
		const int b = Lens[m2];
		const uint32_t v = BitstreamGetBits(&bs, b);
		Crc |= (v!=Extracted[m2]);
//		printf( "=> %d: %d 0x%x %c\n", m2, b, v, " *"[Crc]);
	}
  }
  t1 = (gettime_usec() - t1) / nb_tests;
  printf(" test_bits   %.3f usec   %s\n", t1, (Crc!=0)?"| ERROR": "" );
}

/*********************************************************************
 * main
 *********************************************************************/

static void arg_missing(const char *opt)
{
  printf( "missing argument after option '%s'\n", opt);
  exit(-1);
}

int main(int argc, const char *argv[])
{
	int c, what = 0;
	int width, height;
	uint32_t chksum = 0;
	const char * test_bitstream = 0;
#if defined(WIN32) && defined(ARCH_IS_X86_64)
	DECLARE_ALIGNED_MATRIX(xmm_save, 2, 4, uint64_t, 16);
	// assumes xmm6 and xmm7 won't be falsely preserved by C code
	for(c=0;c<4;c++)
		xmm_save[c] = read_counter();
	prime_xmm(xmm_save);
#endif

	cpu_mask = 0;  // default => will use autodectect
	for(c=1; c<argc; ++c)
	{
	  if (!strcmp(argv[c], "-v")) verbose++;
	  else if (!strcmp(argv[c], "-c"))      cpu_mask = 0 /* PLAIN_C */ | XVID_CPU_FORCE;
	  else if (!strcmp(argv[c], "-mmx"))    cpu_mask = XVID_CPU_MMX    | XVID_CPU_FORCE;
	  else if (!strcmp(argv[c], "-mmxext")) cpu_mask = XVID_CPU_MMXEXT | XVID_CPU_MMX | XVID_CPU_FORCE;
	  else if (!strcmp(argv[c], "-sse2"))   cpu_mask = XVID_CPU_SSE2   | XVID_CPU_MMXEXT | XVID_CPU_MMX | XVID_CPU_FORCE;
	  else if (!strcmp(argv[c], "-sse3"))   cpu_mask = XVID_CPU_SSE3   | XVID_CPU_SSE2 | XVID_CPU_MMXEXT | XVID_CPU_MMX | XVID_CPU_FORCE;
	  else if (!strcmp(argv[c], "-sse4"))   cpu_mask = XVID_CPU_SSE41  | XVID_CPU_SSE3 | XVID_CPU_SSE2 | XVID_CPU_MMXEXT | XVID_CPU_MMX | XVID_CPU_FORCE;
      else if (!strcmp(argv[c], "-3dnow"))  cpu_mask = XVID_CPU_3DNOW  | XVID_CPU_FORCE;
	  else if (!strcmp(argv[c], "-3dnowe")) cpu_mask = XVID_CPU_3DNOW  | XVID_CPU_3DNOWEXT | XVID_CPU_FORCE;
	  else if (!strcmp(argv[c], "-altivec")) cpu_mask = XVID_CPU_ALTIVEC | XVID_CPU_FORCE;
	  else if (!strcmp(argv[c], "-spd")) {
      if (++c==argc) arg_missing( argv[argc-1] );
      speed_ref = atoi(argv[c]);
    }
	  else if (argv[c][0]!='-') {
	    what = atoi(argv[c]);
	    if (what==9) {
	      if (c+4>argc) {
	        printf("usage: %s %d bitstream width height (checksum)\n", argv[0], what);
	        exit(-1);
        }
        test_bitstream = argv[++c];
	      width  = atoi(argv[++c]);
	      height = atoi(argv[++c]);
	      if (c+1<argc && argv[c+1][0]!='-') {
	        if (sscanf(argv[c+1], "0x%x", &chksum)!=1) {
	          printf( "can't read checksum value.\n" );
	          exit(-1);
          }
          else c++;
        }
//        printf( "[%s] %dx%d (0x%.8x)\n", test_bitstream, width, height, chksum);
      }
    }
    else {
      printf( "unrecognized option '%s'\n", argv[c]);
      exit(-1);
    }
  }


	if (what==0 || what==1) test_dct();
	if (what==0 || what==2) test_mb();
	if (what==0 || what==3) test_sad();
	if (what==0 || what==4) test_transfer();
	if (what==0 || what==5) test_quant();
	if (what==0 || what==6) test_cbp();
	if (what==0 || what==10) test_sse();
	if (what==0 || what==11) test_log2bin();
	if (what==0 || what==12) test_gcd();
	if (what==0 || what==13) test_compiler();
	if (what==0 || what==14) test_yuv();
	if (what==0 || what==15) test_SSIM();
	if (what==0 || what==16) test_yuv2();
	if (what==0 || what==17) test_bits();

	if (what==7) {
		test_IEEE1180_compliance(-256, 255, 1);
		test_IEEE1180_compliance(-256, 255,-1);
		test_IEEE1180_compliance(  -5,   5, 1);
		test_IEEE1180_compliance(  -5,   5,-1);
		test_IEEE1180_compliance(-300, 300, 1);
		test_IEEE1180_compliance(-300, 300,-1);
	}
	if (what==8) test_dct_saturation(-256, 255);

	if (test_bitstream)
	  test_dec(test_bitstream, width, height, chksum);
	if (what==-1) {
		test_dct_precision_diffs();
		test_bugs1();
	}
	if (what==-2)
		test_quant_bug();

#if defined(WIN32) && defined(ARCH_IS_X86_64)
	get_xmm(xmm_save+4);
	if (memcmp(xmm_save, xmm_save+4, 4*sizeof(int64_t))) {
		printf("\nWIN64 ERROR: XMM6 and XMM7 contents not preserved!\n"
		       "        XMM6                             XMM7\n"
		       "Before: %.16I64X%.16I64X %.16I64X%.16I64X\n"
		       "After:  %.16I64X%.16I64X %.16I64X%.16I64X",
		        xmm_save[0],xmm_save[1],xmm_save[2],xmm_save[3],
		        xmm_save[4],xmm_save[5],xmm_save[6],xmm_save[7]);
	} else {
		printf("\nWIN64: XMM6 and XMM7 contents preserved correctly.\n");
	}
#endif

	if ((what >= 0 && what <= 6) || what == 10) {
		printf("\n\n"
			   "NB: If a function isn't optimised for a specific set of intructions,\n"
			   "    a C function is used instead. So don't panic if some functions\n"
			   "    may appear to be slow.\n");
	}

#if defined(ARCH_IS_IA32) || defined(ARCH_IS_X86_64)
	if (what == 0 || what == 5) {
		printf("\n"
			   "NB: MMX mpeg4 quantization is known to have very small errors (+/-1 magnitude)\n"
			   "    for 1 or 2 coefficients a block. This is mainly caused by the fact the unit\n"
			   "    test goes far behind the usual limits of real encoding. Please do not report\n"
			   "    this error to the developers.\n");
	}
#endif

	return 0;
}

/*********************************************************************/
