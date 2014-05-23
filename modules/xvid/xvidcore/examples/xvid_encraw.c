/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Console based test application  -
 *
 *  Copyright(C) 2002-2003 Christoph Lampert <gruel@web.de>
 *               2002-2003 Edouard Gomez <ed.gomez@free.fr>
 *               2003      Peter Ross <pross@xvid.org>
 *               2003-2010 Michael Militzer <isibaar@xvid.org>
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
 * $Id: xvid_encraw.c 1985 2011-05-18 09:02:35Z Isibaar $
 *
 ****************************************************************************/

/*****************************************************************************
 *  Application notes :
 *		                    
 *  A sequence of raw YUV I420 pics or YUV I420 PGM file format is encoded
 *  The speed is measured and frames' PSNR are taken from core. 
 *		                   
 *  The program is plain C and needs no libraries except for libxvidcore, 
 *  and maths-lib.
 *
 *  Use ./xvid_encraw -help for a list of options
 *	
 ************************************************************************/

#include <stdio.h>
//#include <io.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#include <vfw.h>
#include <time.h>
#define XVID_AVI_INPUT
#define XVID_AVI_OUTPUT
#endif

#include "xvid.h"
#include "portab.h" /* for pthread */

#ifdef XVID_MKV_OUTPUT
#include "matroska.cpp"
#endif

#undef READ_PNM

//#define USE_APP_LEVEL_THREADING /* Should xvid_encraw app use multi-threading? */

/*****************************************************************************
 *                            Quality presets
 ****************************************************************************/

// Equivalent to vfw's pmvfast_presets
static const int motion_presets[] = {
	/* quality 0 */
	0,

	/* quality 1 */
	0,

	/* quality 2 */
	0,

	/* quality 3 */
	0,

	/* quality 4 */
	0 | XVID_ME_HALFPELREFINE16 | 0,

	/* quality 5 */
	0 | XVID_ME_HALFPELREFINE16 | 0 | XVID_ME_ADVANCEDDIAMOND16,

	/* quality 6 */
	XVID_ME_HALFPELREFINE16 | XVID_ME_EXTSEARCH16 |	XVID_ME_HALFPELREFINE8 | 0 | XVID_ME_USESQUARES16

};
#define ME_ELEMENTS (sizeof(motion_presets)/sizeof(motion_presets[0]))

static const int vop_presets[] = {
	/* quality 0 */
	0,

	/* quality 1 */
	0,

	/* quality 2 */
	0,

	/* quality 3 */
	0,

	/* quality 4 */
	0,

	/* quality 5 */
	XVID_VOP_INTER4V,

	/* quality 6 */
	XVID_VOP_INTER4V,

};
#define VOP_ELEMENTS (sizeof(vop_presets)/sizeof(vop_presets[0]))

/*****************************************************************************
 *                     Command line global variables
 ****************************************************************************/

#define MAX_ZONES   64
#define MAX_ENC_INSTANCES 4
#define DEFAULT_QUANT 400

typedef struct
{
	int frame;

	int type;
	int mode;
	int modifier;

	unsigned int greyscale;
	unsigned int chroma_opt;
	unsigned int bvop_threshold;
	unsigned int cartoon_mode;
} zone_t;

typedef struct
{
	int count;
	int size;
	int quants[32];
} frame_stats_t;

typedef struct
{
	pthread_t handle;       /* thread's handle */

	int start_num;          /* begin/end of sequence */
	int stop_num; 
	
	char *outfilename;      /* output filename */
	char *statsfilename1;   /* pass1 statsfile */

	int input_num; 

	int totalsize;          /* encoder stats */
	double totalenctime; 
	float totalPSNR[3];
 	frame_stats_t framestats[7];
} enc_sequence_data_t;

/* Maximum number of frames to encode */
#define ABS_MAXFRAMENR -1 /* no limit */

#ifndef READ_PNM
#define IMAGE_SIZE(x,y) ((x)*(y)*3/2)
#else
#define IMAGE_SIZE(x,y) ((x)*(y)*3)
#endif

#define MAX(A,B) ( ((A)>(B)) ? (A) : (B) )
#define SMALL_EPS (1e-10)

#define SWAP(a) ( (((a)&0x000000ff)<<24) | (((a)&0x0000ff00)<<8) | \
                  (((a)&0x00ff0000)>>8)  | (((a)&0xff000000)>>24) )

static zone_t ZONES[MAX_ZONES];
static 	int NUM_ZONES = 0;

static 	int ARG_NUM_APP_THREADS = 1;
static 	int ARG_CPU_FLAGS = 0;
static 	int ARG_STATS = 0;
static 	int ARG_SSIM = -1;
static 	int ARG_PSNRHVSM = 0;
static 	char* ARG_SSIM_PATH = NULL;
static 	int ARG_DUMP = 0;
static 	int ARG_LUMIMASKING = 0;
static 	int ARG_BITRATE = 0;
static 	int ARG_TARGETSIZE = 0;
static 	int ARG_SINGLE = 1;
static 	char *ARG_PASS1 = 0;
static 	char *ARG_PASS2 = 0;
//static int ARG_QUALITY = ME_ELEMENTS - 1;
static 	int ARG_QUALITY = 6;
static 	float ARG_FRAMERATE = 0.f;
static 	int ARG_DWRATE = 25;
static 	int ARG_DWSCALE = 1;
static 	int ARG_MAXFRAMENR = ABS_MAXFRAMENR;
static 	int ARG_MAXKEYINTERVAL = 300;
static 	int ARG_STARTFRAMENR = 0;
static 	char *ARG_INPUTFILE = NULL;
static 	int ARG_INPUTTYPE = 0;
static 	int ARG_SAVEMPEGSTREAM = 0;
static 	int ARG_SAVEINDIVIDUAL = 0;
static 	char *ARG_OUTPUTFILE = NULL;
static 	char *ARG_AVIOUTPUTFILE = NULL;
static 	char *ARG_MKVOUTPUTFILE = NULL;
static 	char *ARG_TIMECODEFILE = NULL;
static 	int XDIM = 0;
static 	int YDIM = 0;
static 	int ARG_BQRATIO = 150;
static 	int ARG_BQOFFSET = 100;
static	int ARG_MAXBFRAMES = 2;
static 	int ARG_PACKED = 1;
static 	int ARG_DEBUG = 0;
static 	int ARG_VOPDEBUG = 0;
static 	int ARG_TRELLIS = 1;
static 	int ARG_QTYPE = 0;
static 	int ARG_QMATRIX = 0;
static 	int ARG_GMC = 0;
static 	int ARG_INTERLACING = 0;
static 	int ARG_QPEL = 0;
static 	int ARG_TURBO = 0;
static 	int ARG_VHQMODE = 1;
static 	int ARG_BVHQ = 0;
static 	int ARG_QMETRIC = 0;
static 	int ARG_CLOSED_GOP = 1;
static 	int ARG_CHROMAME = 1;
static 	int ARG_PAR = 1;
static 	int ARG_PARHEIGHT;
static 	int ARG_PARWIDTH;
static 	int ARG_QUANTS[6] = {2, 31, 2, 31, 2, 31};
static 	int ARG_FRAMEDROP = 0;
static 	double ARG_CQ = 0;
static 	int ARG_FULL1PASS = 0;
static 	int ARG_REACTION = 16;
static 	int ARG_AVERAGING = 100;
static 	int ARG_SMOOTHER = 100;
static 	int ARG_KBOOST = 10;
static 	int ARG_KREDUCTION = 20;
static 	int ARG_KTHRESH = 1;
static  int ARG_CHIGH = 0;
static 	int ARG_CLOW = 0;
static 	int ARG_OVERSTRENGTH = 5;
static 	int ARG_OVERIMPROVE = 5;
static 	int ARG_OVERDEGRADE = 5;
static 	int ARG_OVERHEAD = 0;
static 	int ARG_VBVSIZE = 0;
static 	int ARG_VBVMAXRATE = 0;
static 	int ARG_VBVPEAKRATE = 0;
static 	int ARG_THREADS = 0;
static 	int ARG_SLICES = 1;
static 	int ARG_VFR = 0;
static 	int ARG_PROGRESS = 0;
static 	int ARG_COLORSPACE = XVID_CSP_YV12;
	/* the path where to save output */
static char filepath[256] = "./";

static 	unsigned char qmatrix_intra[64];
static 	unsigned char qmatrix_inter[64];

/****************************************************************************
 *                     Nasty global vars ;-)
 ***************************************************************************/

static const int height_ratios[] = {1, 1, 11, 11, 11, 33};
static const int width_ratios[] = {1, 1, 12, 10, 16, 40};

const char userdata_start_code[] = "\0\0\x01\xb2";


/*****************************************************************************
 *               Local prototypes
 ****************************************************************************/

/* Prints program usage message */
static void usage();

/* Statistical functions */
static double msecond();
int gcd(int a, int b);
int minquant(int quants[32]);
int maxquant(int quants[32]);
double avgquant(frame_stats_t frame);

/* PGM related functions */
#ifndef READ_PNM
static int read_pgmheader(FILE * handle);
static int read_pgmdata(FILE * handle,
						unsigned char *image);
#else
static int read_pnmheader(FILE * handle);
static int read_pnmdata(FILE * handle,
						unsigned char *image);
#endif
static int read_yuvdata(FILE * handle,
						unsigned char *image);

/* Encoder related functions */
static void enc_gbl(int use_assembler);
static int  enc_init(void **enc_handle, char *stats_pass1, int start_num);
static int  enc_info();
static int  enc_stop(void *enc_handle);
static int  enc_main(void *enc_handle,
					 unsigned char *image,
			 		 unsigned char *bitstream,
					 int *key,
					 int *stats_type,
					 int *stats_quant,
					 int *stats_length,
					 int stats[3],
					 int framenum);
static void encode_sequence(enc_sequence_data_t *h);

/* Zone Related Functions */
static void apply_zone_modifiers(xvid_enc_frame_t * frame, int framenum);
static void prepare_full1pass_zones();
static void prepare_cquant_zones();
void sort_zones(zone_t * zones, int zone_num, int * sel);


void removedivxp(char *buf, int size);

/*****************************************************************************
 *               Main function
 ****************************************************************************/

int
main(int argc,
	 char *argv[])
{
	double totalenctime = 0.;
	float totalPSNR[3] = {0., 0., 0.};

	FILE *statsfile;
	frame_stats_t framestats[7];

	int input_num = 0;
	int totalsize = 0;
	int use_assembler = 1;
	int i;

	printf("xvid_encraw - raw mpeg4 bitstream encoder ");
	printf("written by Christoph Lampert\n\n");

	/* Is there a dumb Xvid coder ? */
	if(ME_ELEMENTS != VOP_ELEMENTS) {
		fprintf(stderr, "Presets' arrays should have the same number of elements -- Please file a bug to xvid-devel@xvid.org\n");
		return(-1);
	}

	/* Clear framestats */
	memset(framestats, 0, sizeof(framestats));

/*****************************************************************************
 *                            Command line parsing
 ****************************************************************************/

	for (i = 1; i < argc; i++) {

		if (strcmp("-asm", argv[i]) == 0) {
			use_assembler = 1;
		} else if (strcmp("-noasm", argv[i]) == 0) {
			use_assembler = 0;
		} else if (strcmp("-w", argv[i]) == 0 && i < argc - 1) {
			i++;
			XDIM = atoi(argv[i]);
		} else if (strcmp("-h", argv[i]) == 0 && i < argc - 1) {
			i++;
			YDIM = atoi(argv[i]);
		} else if (strcmp("-csp",argv[i]) == 0 && i < argc - 1) {
			i++;
			if (strcmp(argv[i],"i420") == 0){
				ARG_COLORSPACE = XVID_CSP_I420;
			} else if(strcmp(argv[i],"yv12") == 0){
				ARG_COLORSPACE = XVID_CSP_YV12;
			} else {
				printf("Invalid colorspace\n");
				return 0;
			}
		} else if (strcmp("-bitrate", argv[i]) == 0) {
			if (i < argc - 1)
				ARG_BITRATE = atoi(argv[i+1]);
			if (ARG_BITRATE) {
				i++;
				if (ARG_BITRATE <= 20000)
					/* if given parameter is <= 20000, assume it means kbps */
					ARG_BITRATE *= 1000;
			}
			else
				ARG_BITRATE = 700000;
		} else if (strcmp("-size", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_TARGETSIZE = atoi(argv[i]);
        } else if (strcmp("-cq", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_CQ = atof(argv[i])*100;
		} else if (strcmp("-single", argv[i]) == 0) {
			ARG_SINGLE = 1;
			ARG_PASS1 = NULL;
			ARG_PASS2 = NULL;
		} else if (strcmp("-pass1", argv[i]) == 0) {
			ARG_SINGLE = 0;
			if ((i < argc - 1) && (*argv[i+1] != '-')) {
				i++;
				ARG_PASS1 = argv[i];
			} else {
				ARG_PASS1 = "xvid.stats";
			}
		} else if (strcmp("-full1pass", argv[i]) == 0) {
			ARG_FULL1PASS = 1;
		} else if (strcmp("-pass2", argv[i]) == 0) {
			ARG_SINGLE = 0;
			if ((i < argc - 1) && (*argv[i+1] != '-')) {
				i++;
				ARG_PASS2 = argv[i];
			} else {
				ARG_PASS2 = "xvid.stats";
			}
		} else if (strcmp("-max_bframes", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_MAXBFRAMES = atoi(argv[i]);
		} else if (strcmp("-par", argv[i]) == 0 && i < argc - 1) {
			i++;
			if (sscanf(argv[i], "%d:%d", &(ARG_PARWIDTH), &(ARG_PARHEIGHT))!=2)
				ARG_PAR = atoi(argv[i]);
			else {
				int div;
				ARG_PAR = 0;
				div = gcd(ARG_PARWIDTH, ARG_PARHEIGHT);
				ARG_PARWIDTH /= div;
				ARG_PARHEIGHT /= div;
			}
		} else if (strcmp("-nopacked", argv[i]) == 0) {
			ARG_PACKED = 0;
		} else if (strcmp("-packed", argv[i]) == 0) {
			ARG_PACKED = 2;
		} else if (strcmp("-nochromame", argv[i]) == 0) {
			ARG_CHROMAME = 0;
		} else if (strcmp("-threads", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_THREADS = atoi(argv[i]);
		} else if (strcmp("-slices", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_SLICES = atoi(argv[i]);
		} else if (strcmp("-bquant_ratio", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_BQRATIO = atoi(argv[i]);
		} else if (strcmp("-bquant_offset", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_BQOFFSET = atoi(argv[i]);

		} else if (strcmp("-zones", argv[i]) == 0 && i < argc -1) {
			char c;
			char *frameoptions, *rem;
			int startframe;
			char options[40];
			
			i++;

			do {
				rem = strrchr(argv[i], '/');
				if (rem==NULL)
					rem=argv[i];
				else {
					*rem = '\0';
					rem++;
				}
				if (sscanf(rem, "%d,%c,%s", &startframe, &c, options)<3) {
					fprintf(stderr, "Zone error, bad parameters %s\n", rem);
					continue;
				}
				if (NUM_ZONES >= MAX_ZONES) {
					fprintf(stderr, "warning: too many zones; zone ignored\n");
					continue;
				}
				memset(&ZONES[NUM_ZONES], 0, sizeof(zone_t));

				ZONES[NUM_ZONES].frame = startframe;
				ZONES[NUM_ZONES].modifier = (int)(atof(options)*100);
				if (toupper(c)=='Q')
					ZONES[NUM_ZONES].mode = XVID_ZONE_QUANT;
				else if (toupper(c)=='W')
					ZONES[NUM_ZONES].mode = XVID_ZONE_WEIGHT;
				else {
					fprintf(stderr, "Bad zone type %c\n", c);
					continue;
				}

				if ((frameoptions=strchr(options, ','))!=NULL) {
					int readchar=0, count;
					frameoptions++;
					while (readchar<(int)strlen(frameoptions)) {
						if (sscanf(frameoptions+readchar, "%d%n", &(ZONES[NUM_ZONES].bvop_threshold), &count)==1) {
							readchar += count;
						}
						else {
							if (toupper(frameoptions[readchar])=='K')
								ZONES[NUM_ZONES].type = XVID_TYPE_IVOP;
							else if (toupper(frameoptions[readchar])=='G')
								ZONES[NUM_ZONES].greyscale = 1;
							else if (toupper(frameoptions[readchar])=='O')
								ZONES[NUM_ZONES].chroma_opt = 1;
							else if (toupper(frameoptions[readchar])=='C')
								ZONES[NUM_ZONES].cartoon_mode = 1;
							else {
								fprintf(stderr, "Error in zone %s option %c\n", rem, frameoptions[readchar]);
								break;
							}
							readchar++;
						}
					}
				}
				NUM_ZONES++;
			} while (rem != argv[i]);


		} else if ((strcmp("-zq", argv[i]) == 0 || strcmp("-zw", argv[i]) == 0) && i < argc - 2) {

            if (NUM_ZONES >= MAX_ZONES) {
                fprintf(stderr,"warning: too many zones; zone ignored\n");
                continue;
            }
			memset(&ZONES[NUM_ZONES], 0, sizeof(zone_t));
			if (strcmp("-zq", argv[i])== 0) {
				ZONES[NUM_ZONES].mode = XVID_ZONE_QUANT;
			}
			else {
				ZONES[NUM_ZONES].mode = XVID_ZONE_WEIGHT;
			}
			ZONES[NUM_ZONES].modifier = (int)(atof(argv[i+2])*100);
			i++;
            ZONES[NUM_ZONES].frame = atoi(argv[i]);
			i++;
			ZONES[NUM_ZONES].type = XVID_TYPE_AUTO;
			ZONES[NUM_ZONES].greyscale = 0;
			ZONES[NUM_ZONES].chroma_opt = 0;
			ZONES[NUM_ZONES].bvop_threshold = 0;
			ZONES[NUM_ZONES].cartoon_mode = 0;

            NUM_ZONES++;
		} else if (strcmp("-quality", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_QUALITY = atoi(argv[i]);
		} else if (strcmp("-start", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_STARTFRAMENR = atoi(argv[i]);
		} else if (strcmp("-vhqmode", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_VHQMODE = atoi(argv[i]);
		} else if (strcmp("-metric", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_QMETRIC = atoi(argv[i]);
		} else if (strcmp("-framerate", argv[i]) == 0 && i < argc - 1) {
			int exponent;
			i++;
			ARG_FRAMERATE = (float) atof(argv[i]);
			exponent = (int) strcspn(argv[i], ".");
			if (exponent<(int)strlen(argv[i]))
				exponent=(int)pow(10.0, (int)(strlen(argv[i])-1-exponent));
			else
				exponent=1;
			ARG_DWRATE = (int)(atof(argv[i])*exponent);
			ARG_DWSCALE = exponent;
			exponent = gcd(ARG_DWRATE, ARG_DWSCALE);
			ARG_DWRATE /= exponent;
			ARG_DWSCALE /= exponent;
		} else if (strcmp("-max_key_interval", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_MAXKEYINTERVAL = atoi(argv[i]);
		} else if (strcmp("-i", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_INPUTFILE = argv[i];
		} else if (strcmp("-stats", argv[i]) == 0) {
			ARG_STATS = 1;
		} else if (strcmp("-ssim", argv[i]) == 0) {
			ARG_SSIM = 2;
			if ((i < argc - 1) && (*argv[i+1] != '-')) {
				i++;
				ARG_SSIM = atoi(argv[i]);
			}
		} else if (strcmp("-psnrhvsm", argv[i]) == 0) {
			ARG_PSNRHVSM = 1;
		} else if (strcmp("-ssim_file", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_SSIM_PATH = argv[i];
		} else if (strcmp("-timecode", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_TIMECODEFILE = argv[i];
		} else if (strcmp("-dump", argv[i]) == 0) {
			ARG_DUMP = 1;
		} else if (strcmp("-masking", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_LUMIMASKING = atoi(argv[i]);
		} else if (strcmp("-type", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_INPUTTYPE = atoi(argv[i]);
		} else if (strcmp("-frames", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_MAXFRAMENR = atoi(argv[i]);
		} else if (strcmp("-drop", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_FRAMEDROP = atoi(argv[i]);
		} else if (strcmp("-imin", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_QUANTS[0] = atoi(argv[i]);
		} else if (strcmp("-imax", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_QUANTS[1] = atoi(argv[i]);
		} else if (strcmp("-pmin", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_QUANTS[2] = atoi(argv[i]);
		} else if (strcmp("-pmax", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_QUANTS[3] = atoi(argv[i]);
		} else if (strcmp("-bmin", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_QUANTS[4] = atoi(argv[i]);
		} else if (strcmp("-bmax", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_QUANTS[5] = atoi(argv[i]);
		} else if (strcmp("-qtype", argv[i]) == 0 && i < argc - 1) {
			i++;
			ARG_QTYPE = atoi(argv[i]);
		} else if (strcmp("-qmatrix", argv[i]) == 0 && i < argc - 1) {
			FILE *fp = fopen(argv[++i], "rb");
			if (fp == NULL) {
				fprintf(stderr, "Error opening input file %s\n", argv[i]);
				return (-1);
			}			
			fseek(fp, 0, SEEK_END);
			if (ftell(fp) != 128) {
				fprintf(stderr, "Unexpected size of input file %s\n", argv[i]);
				return (-1);
			}			

			fseek(fp, 0, SEEK_SET);
			fread(qmatrix_intra, 1, 64, fp);
			fread(qmatrix_inter, 1, 64, fp);

			ARG_QMATRIX = 1;
			ARG_QTYPE = 1;
		} else if (strcmp("-save", argv[i]) == 0) {
			ARG_SAVEMPEGSTREAM = 1;
			ARG_SAVEINDIVIDUAL = 1;
		} else if (strcmp("-debug", argv[i]) == 0 && i < argc -1) {
			i++;
            if (!(sscanf(argv[i],"0x%x", &(ARG_DEBUG)))) 
				sscanf(argv[i],"%d", &(ARG_DEBUG));
		} else if (strcmp("-o", argv[i]) == 0 && i < argc - 1) {
			ARG_SAVEMPEGSTREAM = 1;
			i++;
			ARG_OUTPUTFILE = argv[i];
		} else if (strcmp("-avi", argv[i]) == 0 && i < argc - 1) {
#ifdef XVID_AVI_OUTPUT
			ARG_SAVEMPEGSTREAM = 1;
			i++;
			ARG_AVIOUTPUTFILE = argv[i];
#else
			fprintf( stderr, "Not compiled with AVI output support.\n");
			return(-1);
#endif
		} else if (strcmp("-mkv", argv[i]) == 0 && i < argc - 1) {
#ifdef XVID_MKV_OUTPUT
			ARG_SAVEMPEGSTREAM = 1;
			i++;
			ARG_MKVOUTPUTFILE = argv[i];
#else
			fprintf(stderr, "Not compiled with MKV output support.\n");
			return(-1);
#endif
		} else if (strcmp("-vop_debug", argv[i]) == 0) {
			ARG_VOPDEBUG = 1;
		} else if (strcmp("-notrellis", argv[i]) == 0) {
			ARG_TRELLIS = 0;
		} else if (strcmp("-bvhq", argv[i]) == 0) {
			ARG_BVHQ = 1;
		} else if (strcmp("-qpel", argv[i]) == 0) {
			ARG_QPEL = 1;
		} else if (strcmp("-turbo", argv[i]) == 0) {
			ARG_TURBO = 1;
		} else if (strcmp("-gmc", argv[i]) == 0) {
			ARG_GMC = 1;
		} else if (strcmp("-interlaced", argv[i]) == 0) {
			if ((i < argc - 1) && (*argv[i+1] != '-')) {
				i++;
				ARG_INTERLACING = atoi(argv[i]);
			} else {
				ARG_INTERLACING = 1;
			}
		} else if (strcmp("-noclosed_gop", argv[i]) == 0) {
			ARG_CLOSED_GOP = 0;
		} else if (strcmp("-closed_gop", argv[i]) == 0) {
			ARG_CLOSED_GOP = 2;
		} else if (strcmp("-vbvsize", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_VBVSIZE = atoi(argv[i]);
		} else if (strcmp("-vbvmax", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_VBVMAXRATE = atoi(argv[i]);
		} else if (strcmp("-vbvpeak", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_VBVPEAKRATE = atoi(argv[i]);
		} else if (strcmp("-reaction", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_REACTION = atoi(argv[i]);
		} else if (strcmp("-averaging", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_AVERAGING = atoi(argv[i]);
		} else if (strcmp("-smoother", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_SMOOTHER = atoi(argv[i]);
		} else if (strcmp("-kboost", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_KBOOST = atoi(argv[i]);
		} else if (strcmp("-kthresh", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_KTHRESH = atoi(argv[i]);
		} else if (strcmp("-chigh", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_CHIGH = atoi(argv[i]);
		} else if (strcmp("-clow", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_CLOW = atoi(argv[i]);
		} else if (strcmp("-ostrength", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_OVERSTRENGTH = atoi(argv[i]);
		} else if (strcmp("-oimprove", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_OVERIMPROVE = atoi(argv[i]);
		} else if (strcmp("-odegrade", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_OVERDEGRADE = atoi(argv[i]);
		} else if (strcmp("-overhead", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_OVERHEAD = atoi(argv[i]);
		} else if (strcmp("-kreduction", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_KREDUCTION = atoi(argv[i]);
        } else if (strcmp("-progress", argv[i]) == 0) {
			if (i < argc - 1)
				/* in kbps */
				ARG_PROGRESS = atoi(argv[i+1]);
			if (ARG_PROGRESS > 0)
				i++;
			else
				ARG_PROGRESS = 10;
		} else if (strcmp("-help", argv[i]) == 0) {
			usage();
			return (0);
		} else {
			usage();
			exit(-1);
		}

	}

/*****************************************************************************
 *                            Arguments checking
 ****************************************************************************/

	if (XDIM <= 0 || XDIM >= 4096 || YDIM <= 0 || YDIM >= 4096) {
		fprintf(stderr,
				"Trying to retrieve width and height from input header\n");
		if (!ARG_INPUTTYPE)
			ARG_INPUTTYPE = 1;		/* pgm */
	}

	if (ARG_QUALITY < 0 ) {
		ARG_QUALITY = 0;
	} else if (ARG_QUALITY >= ME_ELEMENTS) {
		ARG_QUALITY = ME_ELEMENTS - 1;
	}

	if (ARG_STARTFRAMENR < 0) {
		fprintf(stderr, "Bad starting frame number %d, cannot be negative\n", ARG_STARTFRAMENR);
		return(-1);
	}

	if (ARG_PASS2) {
		if (ARG_PASS2 == ARG_PASS1) {
			fprintf(stderr, "Can't use the same statsfile for pass1 and pass2: %s\n", ARG_PASS2);
			return(-1);
		}
		  statsfile = fopen(ARG_PASS2, "rb");
		  if (statsfile == NULL) {
			  fprintf(stderr, "Couldn't open statsfile '%s'!\n", ARG_PASS2);
			  return (-1);
		  }
		  fclose(statsfile);
	}

#ifdef XVID_AVI_OUTPUT
	if (ARG_AVIOUTPUTFILE == NULL && ARG_PACKED <= 1)
		ARG_PACKED = 0;
#endif

	if (ARG_BITRATE < 0) {
		fprintf(stderr, "Bad bitrate %d, cannot be negative\n", ARG_BITRATE);
		return(-1);
	}

	if (NUM_ZONES) {
		int i;
		sort_zones(ZONES, NUM_ZONES, &i);
	}

	if (ARG_PAR > 5) {
		fprintf(stderr, "Bad PAR: %d. Must be [1..5] or width:height\n", ARG_PAR);
		return(-1);
	}

	if (ARG_MAXFRAMENR == 0) {
		fprintf(stderr, "Wrong number of frames\n");
		return (-1);
	}

	if (ARG_INPUTFILE != NULL) {
#if defined(XVID_AVI_INPUT)
      if (strcmp(ARG_INPUTFILE+(strlen(ARG_INPUTFILE)-3), "avs")==0 ||
          strcmp(ARG_INPUTFILE+(strlen(ARG_INPUTFILE)-3), "avi")==0 ||
		  ARG_INPUTTYPE==2)
      {
		  PAVIFILE avi_in = NULL;
		  PAVISTREAM avi_in_stream = NULL;
		  PGETFRAME get_frame = NULL;
		  BITMAPINFOHEADER myBitmapInfoHeader;
		  AVISTREAMINFO avi_info;
		  FILE *avi_fp = fopen(ARG_INPUTFILE, "rb");

		  AVIFileInit();

		  if (avi_fp == NULL) {
			  fprintf(stderr, "Couldn't open file '%s'!\n", ARG_INPUTFILE);
			  return (-1);
		  }
		  fclose(avi_fp);

		  if (AVIFileOpen(&avi_in, ARG_INPUTFILE, OF_READ, NULL) != AVIERR_OK) {
			  fprintf(stderr, "Can't open avi/avs file %s\n", ARG_INPUTFILE);
			  AVIFileExit();
			  return(-1);
		  }

		  if (AVIFileGetStream(avi_in, &avi_in_stream, streamtypeVIDEO, 0) != AVIERR_OK) {
			  fprintf(stderr, "Can't open stream from file '%s'!\n", ARG_INPUTFILE);
			  AVIFileRelease(avi_in);
			  AVIFileExit();
			  return (-1);
		  }

		  AVIFileRelease(avi_in);

		  if(AVIStreamInfo(avi_in_stream, &avi_info, sizeof(AVISTREAMINFO)) != AVIERR_OK) {
			  fprintf(stderr, "Can't get stream info from file '%s'!\n", ARG_INPUTFILE);
			  AVIStreamRelease(avi_in_stream);
			  AVIFileExit();
			  return (-1);
		  }

	      if (avi_info.fccHandler != MAKEFOURCC('Y', 'V', '1', '2')) {
			  LONG size;
			  fprintf(stderr, "Non YV12 input colorspace %c%c%c%c! Attempting conversion...\n",
				  avi_info.fccHandler%256, (avi_info.fccHandler>>8)%256, (avi_info.fccHandler>>16)%256,
				  (avi_info.fccHandler>>24)%256);
			  size = sizeof(myBitmapInfoHeader);
			  AVIStreamReadFormat(avi_in_stream, 0, &myBitmapInfoHeader, &size);
			  if (size==0)
				  fprintf(stderr, "AVIStreamReadFormat read 0 bytes.\n");
			  else {
				  fprintf(stderr, "AVIStreamReadFormat read %d bytes.\n", size);
				  fprintf(stderr, "width = %d, height = %d, planes = %d\n", myBitmapInfoHeader.biWidth,
					  myBitmapInfoHeader.biHeight, myBitmapInfoHeader.biPlanes);
				  fprintf(stderr, "Compression = %c%c%c%c, %d\n",
					  myBitmapInfoHeader.biCompression%256, (myBitmapInfoHeader.biCompression>>8)%256,
					  (myBitmapInfoHeader.biCompression>>16)%256, (myBitmapInfoHeader.biCompression>>24)%256,
					  myBitmapInfoHeader.biCompression);
				  fprintf(stderr, "Bits Per Pixel = %d\n", myBitmapInfoHeader.biBitCount);
				  myBitmapInfoHeader.biCompression = MAKEFOURCC('Y', 'V', '1', '2');
				  myBitmapInfoHeader.biBitCount = 12;
				  myBitmapInfoHeader.biSizeImage = (myBitmapInfoHeader.biWidth*myBitmapInfoHeader.biHeight)*3/2;
				  get_frame = AVIStreamGetFrameOpen(avi_in_stream, &myBitmapInfoHeader);
			  }
			  if (get_frame == NULL) {
				AVIStreamRelease(avi_in_stream);
				AVIFileExit();
				return (-1);
			  } 
			  else {
				unsigned char *temp;
				fprintf(stderr, "AVIStreamGetFrameOpen successful.\n");
				temp = (unsigned char*)AVIStreamGetFrame(get_frame, 0);
				if (temp != NULL) {
					int i;
					for (i = 0; i < (int)((DWORD*)temp)[0]; i++) {
						fprintf(stderr, "%2d ", temp[i]);
					}
					fprintf(stderr, "\n");
				}
			  }
			  if (avi_info.fccHandler == MAKEFOURCC('D', 'I', 'B', ' ')) {
				  AVIStreamGetFrameClose(get_frame);
				  get_frame = NULL;
				  ARG_COLORSPACE = XVID_CSP_BGR | XVID_CSP_VFLIP;
			  }
		  }
		  
          if (ARG_MAXFRAMENR<0)
			ARG_MAXFRAMENR = avi_info.dwLength-ARG_STARTFRAMENR;
		  else
			ARG_MAXFRAMENR = min(ARG_MAXFRAMENR, (int) (avi_info.dwLength-ARG_STARTFRAMENR));

		  XDIM = avi_info.rcFrame.right - avi_info.rcFrame.left;
		  YDIM = avi_info.rcFrame.bottom - avi_info.rcFrame.top;
		  if (ARG_FRAMERATE==0) {
	 		ARG_FRAMERATE = (float) avi_info.dwRate / (float) avi_info.dwScale;
			ARG_DWRATE = avi_info.dwRate;
			ARG_DWSCALE = avi_info.dwScale;
		  }

		  ARG_INPUTTYPE = 2;

	  	  if (get_frame) AVIStreamGetFrameClose(get_frame);
		  if (avi_in_stream) AVIStreamRelease(avi_in_stream);
		  AVIFileExit();
      }
      else
#endif
		{
			FILE *in_file = fopen(ARG_INPUTFILE, "rb");
			int pos = 0;
			if (in_file == NULL) {
				fprintf(stderr, "Error opening input file %s\n", ARG_INPUTFILE);
				return (-1);
			}
#ifdef USE_APP_LEVEL_THREADING
			fseek(in_file, 0, SEEK_END); /* Determine input size */
			pos = ftell(in_file);
			ARG_MAXFRAMENR = pos / IMAGE_SIZE(XDIM, YDIM); /* PGM, header size ?? */
#endif
			fclose(in_file);
		}
	}

	if (ARG_FRAMERATE <= 0) {
		ARG_FRAMERATE = 25.00f; /* default value */
	}

	if (ARG_TARGETSIZE) {
		if (ARG_MAXFRAMENR <= 0) {
			fprintf(stderr, "Bad target size; number of input frames unknown\n");
			goto release_all;
		} else if (ARG_BITRATE) {
				fprintf(stderr, "Parameter conflict: Do not specify both -bitrate and -size\n");
				goto release_all;
		} else
			ARG_BITRATE = (int) (((ARG_TARGETSIZE * 8) / (ARG_MAXFRAMENR / ARG_FRAMERATE)) * 1024);
	}

		/* Set constant quant to default if no bitrate given for single pass */
	if (ARG_SINGLE && (!ARG_BITRATE) && (!ARG_CQ))
			ARG_CQ = DEFAULT_QUANT;

		/* Init xvidcore */
    enc_gbl(use_assembler);

#ifdef USE_APP_LEVEL_THREADING
	if (ARG_INPUTFILE == NULL || strcmp(ARG_INPUTFILE, "stdin") == 0 ||
	    ARG_NUM_APP_THREADS <= 1 || ARG_THREADS != 0 ||
	    ARG_TIMECODEFILE != NULL || ARG_AVIOUTPUTFILE != NULL ||
	    ARG_INPUTTYPE == 1 || ARG_MKVOUTPUTFILE != NULL)		/* TODO: PGM input */
#endif /* Spawn just one encoder instance */
	{		
		enc_sequence_data_t enc_data;
		memset(&enc_data, 0, sizeof(enc_sequence_data_t));
		
		if (!ARG_THREADS) ARG_THREADS = ARG_NUM_APP_THREADS;
		ARG_NUM_APP_THREADS = 1;

		enc_data.outfilename = ARG_OUTPUTFILE;
		enc_data.statsfilename1 = ARG_PASS1;
		enc_data.start_num = ARG_STARTFRAMENR;
		enc_data.stop_num = ARG_MAXFRAMENR;
		
			/* Encode input */
		encode_sequence(&enc_data);

			/* Copy back stats */
		input_num = enc_data.input_num;
		totalsize = enc_data.totalsize;
		totalenctime = enc_data.totalenctime;
		for (i=0; i < 3; i++) totalPSNR[i] = enc_data.totalPSNR[i];
		memcpy(framestats, enc_data.framestats, sizeof(framestats));
	}
#ifdef USE_APP_LEVEL_THREADING
	else { /* Split input into sequences and create multiple encoder instances */
		int k;
		void *status;
		FILE *f_out = NULL, *f_stats = NULL;

		enc_sequence_data_t enc_data[MAX_ENC_INSTANCES];
		char outfile[MAX_ENC_INSTANCES][256];
		char statsfilename[MAX_ENC_INSTANCES][256];

		for (k = 0; k < MAX_ENC_INSTANCES; k++)
			memset(&enc_data[k], 0, sizeof(enc_sequence_data_t));

			/* Overwrite internal encoder threading */
		if (ARG_NUM_APP_THREADS > MAX_ENC_INSTANCES) {
			ARG_THREADS = (int) (ARG_NUM_APP_THREADS / MAX_ENC_INSTANCES);
			ARG_NUM_APP_THREADS = MAX_ENC_INSTANCES;
		}
		else
			ARG_THREADS = -1;

		enc_data[0].outfilename = ARG_OUTPUTFILE;
		enc_data[0].statsfilename1 = ARG_PASS1;
		enc_data[0].start_num = ARG_STARTFRAMENR;
		enc_data[0].stop_num = (ARG_MAXFRAMENR-ARG_STARTFRAMENR)/ARG_NUM_APP_THREADS;

		for (k = 1; k < ARG_NUM_APP_THREADS; k++) {
			sprintf(outfile[k], "%s.%03d", ARG_OUTPUTFILE, k);
			enc_data[k].outfilename = outfile[k];
			if (ARG_PASS1) {
				sprintf(statsfilename[k], "%s.%03d", ARG_PASS1, k);
				enc_data[k].statsfilename1 = statsfilename[k];
			}
			enc_data[k].start_num = (k*(ARG_MAXFRAMENR-ARG_STARTFRAMENR))/ARG_NUM_APP_THREADS;
			enc_data[k].stop_num = ((k+1)*(ARG_MAXFRAMENR-ARG_STARTFRAMENR))/ARG_NUM_APP_THREADS;
		}

			/* Start multiple encoder threads in parallel */
		for (k = 1; k < ARG_NUM_APP_THREADS; k++) {
			pthread_create(&enc_data[k].handle, NULL, (void*)encode_sequence, (void*)&enc_data[k]);
		}

			/* Encode first sequence in this thread */
		encode_sequence(&enc_data[0]);

			/* Wait until encoder threads have finished */
		for (k = 1; k < ARG_NUM_APP_THREADS; k++) {
			pthread_join(enc_data[k].handle, &status);
		}

			/* Join encoder stats and encoder output files */
		if (ARG_OUTPUTFILE)
			f_out = fopen(enc_data[0].outfilename, "ab+");
		if (ARG_PASS1)
			f_stats = fopen(enc_data[0].statsfilename1, "ab+");

		for (k = 0; k < ARG_NUM_APP_THREADS; k++) {
				/* Join stats */
			input_num += enc_data[k].input_num;
			totalsize += enc_data[k].totalsize;
			totalenctime = MAX(totalenctime, enc_data[k].totalenctime);

			for (i=0; i < 3; i++) totalPSNR[i] += enc_data[k].totalPSNR[i];
			for (i=0; i < 8; i++) {
				int l;
				framestats[i].count += enc_data[k].framestats[i].count;
				framestats[i].size += enc_data[k].framestats[i].size;
				for (l=0; l < 32; l++)
					framestats[i].quants[l] += enc_data[k].framestats[i].quants[l];
			}
				/* Join output files */
			if ((k > 0) && (f_out != NULL)) {
				int ch;
				FILE *f = fopen(enc_data[k].outfilename, "rb");
				while((ch = fgetc(f)) != EOF) { fputc(ch, f_out); }
				fclose(f);
				remove(enc_data[k].outfilename);
			}
				/* Join first pass stats files */
			if ((k > 0) && (f_stats != NULL)) {
				char str[256];
				FILE *f = fopen(enc_data[k].statsfilename1, "r");
				while(fgets(str, sizeof(str), f) != NULL) {
					if (str[0] != '#' && strlen(str) > 3)
						fputs(str, f_stats);
				}
				fclose(f);
				remove(enc_data[k].statsfilename1);
			}
		}
		if (f_out) fclose(f_out);
		if (f_stats) fclose(f_stats);
	}
#endif

/*****************************************************************************
 *         Calculate totals and averages for output, print results
 ****************************************************************************/

 printf("\n");
	printf("Tot: enctime(ms) =%7.2f,               length(bytes) = %7d\n",
		   totalenctime, (int) totalsize);

	if (input_num > 0) {
		totalsize /= input_num;
		totalenctime /= input_num;
		totalPSNR[0] /= input_num;
		totalPSNR[1] /= input_num;
		totalPSNR[2] /= input_num;
	} else {
		totalsize = -1;
		totalenctime = -1;
	}

	printf("Avg: enctime(ms) =%7.2f, fps =%7.2f, length(bytes) = %7d",
		   totalenctime, 1000 / totalenctime, (int) totalsize);
   if (ARG_STATS) {
       printf(", psnr y = %2.2f, psnr u = %2.2f, psnr v = %2.2f",
    		  totalPSNR[0],totalPSNR[1],totalPSNR[2]);
	}
	printf("\n");
	if (framestats[XVID_TYPE_IVOP].count) {
		printf("I frames: %6d frames, size = %7d/%7d, quants = %2d / %.2f / %2d\n", \
			framestats[XVID_TYPE_IVOP].count, framestats[XVID_TYPE_IVOP].size/framestats[XVID_TYPE_IVOP].count, \
			framestats[XVID_TYPE_IVOP].size, minquant(framestats[XVID_TYPE_IVOP].quants), \
			avgquant(framestats[XVID_TYPE_IVOP]), maxquant(framestats[XVID_TYPE_IVOP].quants));
	}
	if (framestats[XVID_TYPE_PVOP].count) {
		printf("P frames: %6d frames, size = %7d/%7d, quants = %2d / %.2f / %2d\n", \
			framestats[XVID_TYPE_PVOP].count, framestats[XVID_TYPE_PVOP].size/framestats[XVID_TYPE_PVOP].count, \
			framestats[XVID_TYPE_PVOP].size, minquant(framestats[XVID_TYPE_PVOP].quants), \
			avgquant(framestats[XVID_TYPE_PVOP]), maxquant(framestats[XVID_TYPE_PVOP].quants));
	}
	if (framestats[XVID_TYPE_BVOP].count) {
		printf("B frames: %6d frames, size = %7d/%7d, quants = %2d / %.2f / %2d\n", \
			framestats[XVID_TYPE_BVOP].count, framestats[XVID_TYPE_BVOP].size/framestats[XVID_TYPE_BVOP].count, \
			framestats[XVID_TYPE_BVOP].size, minquant(framestats[XVID_TYPE_BVOP].quants), \
			avgquant(framestats[XVID_TYPE_BVOP]), maxquant(framestats[XVID_TYPE_BVOP].quants));
	}
	if (framestats[XVID_TYPE_SVOP].count) {
		printf("S frames: %6d frames, size = %7d/%7d, quants = %2d / %.2f / %2d\n", \
			framestats[XVID_TYPE_SVOP].count, framestats[XVID_TYPE_SVOP].size/framestats[XVID_TYPE_SVOP].count, \
			framestats[XVID_TYPE_SVOP].size, minquant(framestats[XVID_TYPE_SVOP].quants), \
			avgquant(framestats[XVID_TYPE_SVOP]), maxquant(framestats[XVID_TYPE_SVOP].quants));
	}
	if (framestats[5].count) {
		printf("N frames: %6d frames, size = %7d/%7d\n", \
			framestats[5].count, framestats[5].size/framestats[5].count, \
			framestats[5].size);
	}


/*****************************************************************************
 *                            Xvid PART  Stop
 ****************************************************************************/

  release_all:

	return (0);
}

/*****************************************************************************
 *               Encode a sequence
 ****************************************************************************/

void encode_sequence(enc_sequence_data_t *h) {

	/* Internal structures (handles) for encoding */
	void *enc_handle = NULL;

	int start_num = h->start_num; 
	int stop_num = h->stop_num; 
	char *outfilename = h->outfilename; 
	float *totalPSNR = h->totalPSNR;

	int input_num;
	int totalsize; 
	double totalenctime = 0.; 

	unsigned char *mp4_buffer = NULL;
	unsigned char *in_buffer = NULL;
	unsigned char *out_buffer = NULL;

	double enctime;

	int result;
	int output_num;
	int nvop_counter;
	int m4v_size;
	int key;
	int stats_type;
	int stats_quant;
	int stats_length;
	int fakenvop = 0;

	FILE *in_file = stdin;
	FILE *out_file = NULL;
	FILE *time_file = NULL;

	char filename[256];

#ifdef XVID_MKV_OUTPUT
	PMKVFILE myMKVFile = NULL;
	PMKVSTREAM myMKVStream = NULL;
	MKVSTREAMINFO myMKVStreamInfo;
#endif
#if defined(XVID_AVI_INPUT)
	PAVIFILE avi_in = NULL;
	PAVISTREAM avi_in_stream = NULL;
 	PGETFRAME get_frame = NULL;
	BITMAPINFOHEADER myBitmapInfoHeader;
#else
#define get_frame NULL
#endif
#if defined(XVID_AVI_OUTPUT)
	int avierr;
	PAVIFILE myAVIFile = NULL;
	PAVISTREAM myAVIStream = NULL;
	AVISTREAMINFO myAVIStreamInfo;
#endif
#if defined(XVID_AVI_INPUT) || defined(XVID_AVI_OUTPUT)
	if (ARG_NUM_APP_THREADS > 1)
		CoInitializeEx(0, COINIT_MULTITHREADED);
	AVIFileInit();
#endif

	if (ARG_INPUTFILE == NULL || strcmp(ARG_INPUTFILE, "stdin") == 0) {
		in_file = stdin;
	} else {
#ifdef XVID_AVI_INPUT
      if (strcmp(ARG_INPUTFILE+(strlen(ARG_INPUTFILE)-3), "avs")==0 ||
          strcmp(ARG_INPUTFILE+(strlen(ARG_INPUTFILE)-3), "avi")==0 ||
		  ARG_INPUTTYPE==2)
      {
		  AVISTREAMINFO avi_info;
		  FILE *avi_fp = fopen(ARG_INPUTFILE, "rb");

		  if (avi_fp == NULL) {
			  fprintf(stderr, "Couldn't open file '%s'!\n", ARG_INPUTFILE);
			  return;
		  }
		  fclose(avi_fp);

		  if (AVIFileOpen(&avi_in, ARG_INPUTFILE, OF_READ, NULL) != AVIERR_OK) {
			  fprintf(stderr, "Can't open avi/avs file %s\n", ARG_INPUTFILE);
			  AVIFileExit();
			  return;
		  }

		  if (AVIFileGetStream(avi_in, &avi_in_stream, streamtypeVIDEO, 0) != AVIERR_OK) {
			  fprintf(stderr, "Can't open stream from file '%s'!\n", ARG_INPUTFILE);
			  AVIFileRelease(avi_in);
			  AVIFileExit();
			  return;
		  }

		  AVIFileRelease(avi_in);

		  if(AVIStreamInfo(avi_in_stream, &avi_info, sizeof(AVISTREAMINFO)) != AVIERR_OK) {
			  fprintf(stderr, "Can't get stream info from file '%s'!\n", ARG_INPUTFILE);
			  AVIStreamRelease(avi_in_stream);
			  AVIFileExit();
			  return;
		  }

	      if (avi_info.fccHandler != MAKEFOURCC('Y', 'V', '1', '2')) {
			  LONG size;
			  fprintf(stderr, "Non YV12 input colorspace %c%c%c%c! Attempting conversion...\n",
				  avi_info.fccHandler%256, (avi_info.fccHandler>>8)%256, (avi_info.fccHandler>>16)%256,
				  (avi_info.fccHandler>>24)%256);
			  size = sizeof(myBitmapInfoHeader);
			  AVIStreamReadFormat(avi_in_stream, 0, &myBitmapInfoHeader, &size);
			  if (size==0)
				  fprintf(stderr, "AVIStreamReadFormat read 0 bytes.\n");
			  else {
				  fprintf(stderr, "AVIStreamReadFormat read %d bytes.\n", size);
				  fprintf(stderr, "width = %d, height = %d, planes = %d\n", myBitmapInfoHeader.biWidth,
					  myBitmapInfoHeader.biHeight, myBitmapInfoHeader.biPlanes);
				  fprintf(stderr, "Compression = %c%c%c%c, %d\n",
					  myBitmapInfoHeader.biCompression%256, (myBitmapInfoHeader.biCompression>>8)%256,
					  (myBitmapInfoHeader.biCompression>>16)%256, (myBitmapInfoHeader.biCompression>>24)%256,
					  myBitmapInfoHeader.biCompression);
				  fprintf(stderr, "Bits Per Pixel = %d\n", myBitmapInfoHeader.biBitCount);
				  myBitmapInfoHeader.biCompression = MAKEFOURCC('Y', 'V', '1', '2');
				  myBitmapInfoHeader.biBitCount = 12;
				  myBitmapInfoHeader.biSizeImage = (myBitmapInfoHeader.biWidth*myBitmapInfoHeader.biHeight)*3/2;
				  get_frame = AVIStreamGetFrameOpen(avi_in_stream, &myBitmapInfoHeader);
			  }
			  if (get_frame == NULL) {
				AVIStreamRelease(avi_in_stream);
				AVIFileExit();
				return;
			  } 
			  else {
				unsigned char *temp;
				fprintf(stderr, "AVIStreamGetFrameOpen successful.\n");
				temp = (unsigned char*)AVIStreamGetFrame(get_frame, 0);
				if (temp != NULL) {
					int i;
					for (i = 0; i < (int)((DWORD*)temp)[0]; i++) {
						fprintf(stderr, "%2d ", temp[i]);
					}
					fprintf(stderr, "\n");
				}
			  }
			  if (avi_info.fccHandler == MAKEFOURCC('D', 'I', 'B', ' ')) {
				  AVIStreamGetFrameClose(get_frame);
				  get_frame = NULL;
				  ARG_COLORSPACE = XVID_CSP_BGR | XVID_CSP_VFLIP;
			  }
		  }
    }
    else
#endif
		{
			in_file = fopen(ARG_INPUTFILE, "rb");
			if (in_file == NULL) {
				fprintf(stderr, "Error opening input file %s\n", ARG_INPUTFILE);
				return;
			}
		}
	}

	// This should be after the avi input opening stuff
	if (ARG_TIMECODEFILE != NULL) {
		time_file = fopen(ARG_TIMECODEFILE, "r");
		if (time_file==NULL) {
			fprintf(stderr, "Couldn't open timecode file '%s'!\n", ARG_TIMECODEFILE);
			return;
		}
		else {
			fscanf(time_file, "# timecode format v2\n");
		}
	}

	if (ARG_INPUTTYPE==1) {
#ifndef READ_PNM
		if (read_pgmheader(in_file)) {
#else
		if (read_pnmheader(in_file)) {
#endif
			fprintf(stderr,
					"Wrong input format, I want YUV encapsulated in PGM\n");
			return;
		}
	}

	/* Jump to the starting frame */
	if (ARG_INPUTTYPE == 0) /* TODO: Other input formats ??? */
		fseek(in_file, start_num*IMAGE_SIZE(XDIM, YDIM), SEEK_SET);


		/* now we know the sizes, so allocate memory */
	if (get_frame == NULL) 
	{
		in_buffer = (unsigned char *) malloc(4*XDIM*YDIM);
		if (!in_buffer)
			goto free_all_memory;
	}

	/* this should really be enough memory ! */
	mp4_buffer = (unsigned char *) malloc(IMAGE_SIZE(XDIM, YDIM) * 2);
	if (!mp4_buffer)
		goto free_all_memory;

/*****************************************************************************
 *                            Xvid PART  Start
 ****************************************************************************/


	result = enc_init(&enc_handle, h->statsfilename1, h->start_num);
	if (result) {
		fprintf(stderr, "Encore INIT problem, return value %d\n", result);
		goto release_all;
	}

/*****************************************************************************
 *                            Main loop
 ****************************************************************************/

	if (ARG_SAVEMPEGSTREAM) {

		if (outfilename) {
			if ((out_file = fopen(outfilename, "w+b")) == NULL) {
				fprintf(stderr, "Error opening output file %s\n", outfilename);
				goto release_all;
			}
		}

#ifdef XVID_AVI_OUTPUT
		if (ARG_AVIOUTPUTFILE != NULL ) {
			{
				/* Open the .avi output then close it */
				/* Resets the file size to 0, which AVIFile doesn't seem to do */
				FILE *scrub;
				if ((scrub = fopen(ARG_AVIOUTPUTFILE, "w+b")) == NULL) {
					fprintf(stderr, "Error opening output file %s\n", ARG_AVIOUTPUTFILE);
					goto release_all;
				}
				else
					fclose(scrub);
			}
			memset(&myAVIStreamInfo, 0, sizeof(AVISTREAMINFO));
			myAVIStreamInfo.fccType = streamtypeVIDEO;
			myAVIStreamInfo.fccHandler = MAKEFOURCC('x', 'v', 'i', 'd');
			myAVIStreamInfo.dwScale = ARG_DWSCALE;
			myAVIStreamInfo.dwRate = ARG_DWRATE;
			myAVIStreamInfo.dwLength = ARG_MAXFRAMENR;
			myAVIStreamInfo.dwQuality = 10000;
			SetRect(&myAVIStreamInfo.rcFrame, 0, 0, XDIM, YDIM);

			if (avierr=AVIFileOpen(&myAVIFile, ARG_AVIOUTPUTFILE, OF_CREATE|OF_WRITE, NULL)) {
				fprintf(stderr, "AVIFileOpen failed opening output file %s, error code %d\n", ARG_AVIOUTPUTFILE, avierr);
				goto release_all;
			}

			if (avierr=AVIFileCreateStream(myAVIFile, &myAVIStream, &myAVIStreamInfo)) {
				fprintf(stderr, "AVIFileCreateStream failed, error code %d\n", avierr);
				goto release_all;
			}
	
			memset(&myBitmapInfoHeader, 0, sizeof(BITMAPINFOHEADER));
			myBitmapInfoHeader.biHeight = YDIM;
			myBitmapInfoHeader.biWidth = XDIM;
			myBitmapInfoHeader.biPlanes = 1;
			myBitmapInfoHeader.biSize = sizeof(BITMAPINFOHEADER);
			myBitmapInfoHeader.biCompression = MAKEFOURCC('X', 'V', 'I', 'D');
			myBitmapInfoHeader.biBitCount = 12;
			myBitmapInfoHeader.biSizeImage = 6*XDIM*YDIM;
			if (avierr=AVIStreamSetFormat(myAVIStream, 0, &myBitmapInfoHeader, sizeof(BITMAPINFOHEADER))) {
				fprintf(stderr, "AVIStreamSetFormat failed, error code %d\n", avierr);
				goto release_all;
			}
		}
#endif
#ifdef XVID_MKV_OUTPUT
		if (ARG_MKVOUTPUTFILE != NULL) {
			{
				/* Open the .mkv output then close it */
				/* Just to make sure we can write to it */
				FILE *scrub;
				if ((scrub = fopen(ARG_MKVOUTPUTFILE, "w+b")) == NULL) {
					fprintf(stderr, "Error opening output file %s\n", ARG_MKVOUTPUTFILE);
					goto release_all;
				}
				else
					fclose(scrub);
			}

			MKVFileOpen(&myMKVFile, ARG_MKVOUTPUTFILE, OF_CREATE|OF_WRITE, NULL);
			if (ARG_PAR) {
				myMKVStreamInfo.display_height = YDIM*height_ratios[ARG_PAR];
				myMKVStreamInfo.display_width = XDIM*width_ratios[ARG_PAR];
			}
			else {
				myMKVStreamInfo.display_height = YDIM*ARG_PARHEIGHT;
				myMKVStreamInfo.display_width = XDIM*ARG_PARWIDTH;
			}
			myMKVStreamInfo.height = YDIM;
			myMKVStreamInfo.width = XDIM;
			myMKVStreamInfo.framerate = ARG_DWRATE;
			myMKVStreamInfo.framescale = ARG_DWSCALE;
			myMKVStreamInfo.length = ARG_MAXFRAMENR;
			MKVFileCreateStream(myMKVFile, &myMKVStream, &myMKVStreamInfo);
		}
#endif
	} else {
		out_file = NULL;
	}


/*****************************************************************************
 *                       Encoding loop
 ****************************************************************************/

	totalsize = 0;

	result = 0;

	input_num = 0;                      /* input frame counter */
	output_num = start_num;             /* output frame counter */

	nvop_counter = 0;

	do {

		char *type;
		int sse[3];

		if ((input_num+start_num) >= stop_num && stop_num > 0) {
			result = 1;
		}

		if (!result) {
#ifdef XVID_AVI_INPUT
			if (ARG_INPUTTYPE==2) {
				/* read avs/avi data (YUV-format) */
				if (get_frame != NULL) {
					in_buffer = (unsigned char*)AVIStreamGetFrame(get_frame, input_num+start_num);
					if (in_buffer == NULL)
						result = 1;
					else
						in_buffer += ((DWORD*)in_buffer)[0];
				} else {
					if(AVIStreamRead(avi_in_stream, input_num+start_num, 1, in_buffer, 4*XDIM*YDIM, NULL, NULL ) != AVIERR_OK)
						result = 1;
				}
			} else
#endif
			if (ARG_INPUTTYPE==1) {
				/* read PGM data (YUV-format) */
#ifndef READ_PNM
				result = read_pgmdata(in_file, in_buffer);
#else
				result = read_pnmdata(in_file, in_buffer);
#endif
			} else {
				/* read raw data (YUV-format) */
				result = read_yuvdata(in_file, in_buffer);
			}
		}

/*****************************************************************************
 *                       Encode and decode this frame
 ****************************************************************************/

		if ((unsigned int)(input_num+start_num) >= (unsigned int)(stop_num-1) && ARG_MAXBFRAMES) {
			stats_type = XVID_TYPE_PVOP;
		}
		else
			stats_type = XVID_TYPE_AUTO;

		enctime = msecond();
		m4v_size =
			enc_main(enc_handle, !result ? in_buffer : 0, mp4_buffer, &key, &stats_type,
					 &stats_quant, &stats_length, sse, input_num);
		enctime = msecond() - enctime;

		/* Write the Frame statistics */

		if (stats_type > 0) {	/* !XVID_TYPE_NOTHING */
			switch (stats_type) {
				case XVID_TYPE_IVOP:
					type = "I";
					break;
				case XVID_TYPE_PVOP:
					type = "P";
					break;
				case XVID_TYPE_BVOP:
					type = "B";
					if (ARG_PACKED)
						fakenvop = 1;
					break;
				case XVID_TYPE_SVOP:
					type = "S";
					break;
				default:
					type = "U";
					break;
			}

			if (stats_length > 8) {
				h->framestats[stats_type].count++;
				h->framestats[stats_type].quants[stats_quant]++;
				h->framestats[stats_type].size += stats_length;
			}
			else {
				h->framestats[5].count++;
				h->framestats[5].quants[stats_quant]++;
				h->framestats[5].size += stats_length;
			}

#define SSE2PSNR(sse, width, height) ((!(sse))?0.0f : 48.131f - 10*(float)log10((float)(sse)/((float)((width)*(height)))))

			if (ARG_PROGRESS == 0) {
				printf("%5d: key=%i, time= %6.0f, len= %7d", !result ? (input_num+start_num) : -1,
					key, (float) enctime, (int) m4v_size);
				printf(" | type=%s, quant= %2d, len= %7d", type, stats_quant,
				   stats_length);


				if (ARG_STATS) {
					printf(", psnr y = %2.2f, psnr u = %2.2f, psnr v = %2.2f",
						SSE2PSNR(sse[0], XDIM, YDIM), SSE2PSNR(sse[1], XDIM / 2, YDIM / 2),
						SSE2PSNR(sse[2], XDIM / 2, YDIM / 2));
				}
				printf("\n");
			} else {
				if ((input_num) % ARG_PROGRESS == 1) {
					if (stop_num > 0) {
						fprintf(stderr, "\r%7d frames(%3d%%) encoded, %6.2f fps, Average Bitrate = %5.0fkbps", \
							(ARG_NUM_APP_THREADS*input_num), (input_num)*100/(stop_num-start_num), (ARG_NUM_APP_THREADS*input_num)*1000/(totalenctime), \
							((((totalsize)/1000)*ARG_FRAMERATE)*8)/(input_num));
					} else {
						fprintf(stderr, "\r%7d frames encoded, %6.2f fps, Average Bitrate = %5.0fkbps", \
							(ARG_NUM_APP_THREADS*input_num), (ARG_NUM_APP_THREADS*input_num)*1000/(totalenctime), \
							((((totalsize)/1000)*ARG_FRAMERATE)*8)/(input_num));
					}
				}
			}

			if (ARG_STATS) {
				totalPSNR[0] += SSE2PSNR(sse[0], XDIM, YDIM);
				totalPSNR[1] += SSE2PSNR(sse[1], XDIM/2, YDIM/2);
				totalPSNR[2] += SSE2PSNR(sse[2], XDIM/2, YDIM/2);
			}
#undef SSE2PSNR
		}

		if (m4v_size < 0)
			break;

		/* Update encoding time stats */
		totalenctime += enctime;
		totalsize += m4v_size;

/*****************************************************************************
 *                       Save stream to file
 ****************************************************************************/

		if (m4v_size > 0 && ARG_SAVEMPEGSTREAM) {
			char timecode[50];

			if (time_file != NULL) {
				if (fscanf(time_file, "%s\n", timecode) != 1) {
					fprintf(stderr, "Error reading timecode file, frame %d\n", output_num);
					goto release_all;
				}
			}
			else
				sprintf(timecode, "%f", ((double)ARG_DWSCALE/ARG_DWRATE)*1000*output_num);

			/* Save single files */
			if (ARG_SAVEINDIVIDUAL) {
				FILE *out;
				sprintf(filename, "%sframe%05d.m4v", filepath, output_num);
				out = fopen(filename, "w+b");
				fwrite(mp4_buffer, m4v_size, 1, out);
				fclose(out);
			}
#ifdef XVID_AVI_OUTPUT
			if (ARG_AVIOUTPUTFILE && myAVIStream) {
				int output_frame;

				if (time_file == NULL)
					output_frame = output_num;
				else {
					output_frame = (int)(atof(timecode)/1000/((double)ARG_DWSCALE/ARG_DWRATE)+.5);
				}
				if (AVIStreamWrite(myAVIStream, output_frame, 1, mp4_buffer, m4v_size, key ? AVIIF_KEYFRAME : 0, NULL, NULL)) {
					fprintf(stderr, "AVIStreamWrite failed writing frame %d\n", output_num);
					goto release_all;
				}
			}
#endif

				if (key && ARG_PACKED)
					removedivxp((char*)mp4_buffer, m4v_size);

				/* Save ES stream */
				if (outfilename && out_file && !(fakenvop && m4v_size <= 8)) {
						fwrite(mp4_buffer, 1, m4v_size, out_file);
				}
#ifdef XVID_MKV_OUTPUT
				if (ARG_MKVOUTPUTFILE && myMKVStream) {
					MKVStreamWrite(myMKVStream, atof(timecode), 1, (ARG_PACKED && fakenvop && (m4v_size <= 8)) ? NULL : mp4_buffer, m4v_size, key ? AVIIF_KEYFRAME : 0, NULL, NULL);
				}
#endif

			output_num++;
			if (stats_type != XVID_TYPE_BVOP)
				fakenvop=0;
		}

		if (!result)
			(input_num)++;

		/* Read the header if it's pgm stream */
		if (!result && (ARG_INPUTTYPE==1))
#ifndef READ_PNM
 			result = read_pgmheader(in_file);
#else
 			result = read_pnmheader(in_file);
#endif
	} while (1);


  release_all:

  h->input_num = input_num;
  h->totalenctime = totalenctime;
  h->totalsize = totalsize;

#ifdef XVID_AVI_INPUT
	if (get_frame) AVIStreamGetFrameClose(get_frame);
	if (avi_in_stream) AVIStreamRelease(avi_in_stream);
#endif

	if (enc_handle) {
		result = enc_stop(enc_handle);
		if (result)
			fprintf(stderr, "Encore RELEASE problem return value %d\n",
					result);
	}

	if (in_file)
		fclose(in_file);
	if (out_file)
		fclose(out_file);
	if (time_file)
		fclose(time_file);

#ifdef XVID_AVI_OUTPUT
	if (myAVIStream) AVIStreamRelease(myAVIStream);
	if (myAVIFile) AVIFileRelease(myAVIFile);
#endif
#ifdef XVID_MKV_OUTPUT
	if (myMKVStream) MKVStreamRelease(myMKVStream);
	if (myMKVFile) MKVFileRelease(myMKVFile);
#endif
#if defined(XVID_AVI_INPUT) || defined(XVID_AVI_OUTPUT)
	AVIFileExit();
#endif

  free_all_memory:
	free(out_buffer);
	free(mp4_buffer);
	free(in_buffer);
}

/*****************************************************************************
 *                        "statistical" functions
 *
 *  these are not needed for encoding or decoding, but for measuring
 *  time and quality, there in nothing specific to Xvid in these
 *
 *****************************************************************************/

/* Return time elapsed time in miliseconds since the program started */
static double
msecond()
{
#ifndef WIN32
	struct timeval tv;

	gettimeofday(&tv, 0);
	return (tv.tv_sec * 1.0e3 + tv.tv_usec * 1.0e-3);
#else
	clock_t clk;

	clk = clock();
	return (clk * 1000.0 / CLOCKS_PER_SEC);
#endif
}

int
gcd(int a, int b)
{
	int r ;

	if (b > a) {
		r = a;
		a = b;
		b = r;
	}

	while ((r = a % b)) {
		a = b;
		b = r;
	}
	return b;
}

int minquant(int quants[32])
{
	int i = 1;
	while (quants[i] == 0) {
		i++;
	}
	return i;
}

int maxquant(int quants[32])
{
	int i = 31;
	while (quants[i] == 0) {
		i--;
	}
	return i;
}

double avgquant(frame_stats_t frame)
{
	double avg=0;
	int i;
	for (i=1; i < 32; i++) {
		avg += frame.quants[i]*i;
	}
	avg /= frame.count;
	return avg;
}

/*****************************************************************************
 *                             Usage message
 *****************************************************************************/

static void
usage()
{
	fprintf(stderr, "xvid_encraw built at %s on %s\n", __TIME__, __DATE__);
	fprintf(stderr, "Usage : xvid_encraw [OPTIONS]\n\n");
	fprintf(stderr, "Input options:\n");
	fprintf(stderr, " -i      string : input filename (stdin)\n");
#ifdef XVID_AVI_INPUT
	fprintf(stderr, " -type   integer: input data type (yuv=0, pgm=1, avi/avs=2)\n");
#else
	fprintf(stderr, " -type   integer: input data type (yuv=0, pgm=1)\n");
#endif
	fprintf(stderr, " -w      integer: frame width ([1.2048])\n");
	fprintf(stderr, " -h      integer: frame height ([1.2048])\n");
	fprintf(stderr, " -csp    string : colorspace of raw input file i420, yv12 (default)\n");
	fprintf(stderr, " -frames integer: number of frames to encode\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Output options:\n");
	fprintf(stderr, " -dump      : save decoder output\n");
	fprintf(stderr, " -save      : save an Elementary Stream file per frame\n");
	fprintf(stderr, " -o string  : save an Elementary Stream for the complete sequence\n");
#ifdef XVID_AVI_OUTPUT
	fprintf(stderr, " -avi string: save an AVI file for the complete sequence\n");
#endif
	fprintf(stderr, " -mkv string: save a MKV file for the complete sequence\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "BFrames options:\n");
	fprintf(stderr, " -max_bframes   integer: max bframes (2)\n");
	fprintf(stderr,	" -bquant_ratio  integer: bframe quantizer ratio (150)\n");
	fprintf(stderr,	" -bquant_offset integer: bframe quantizer offset (100)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Rate control options:\n");
	fprintf(stderr, " -framerate float               : target framerate (auto)\n");
	fprintf(stderr,	" -bitrate   [integer]           : target bitrate in kbps (700)\n");
	fprintf(stderr, " -size      integer			 : target size in kilobytes\n");
    fprintf(stderr,	" -single                        : single pass mode (default)\n");
	fprintf(stderr, " -cq        float               : single pass constant quantizer\n");
	fprintf(stderr, " -pass1     [filename]          : twopass mode (first pass)\n");
	fprintf(stderr, " -full1pass                     : perform full first pass\n");
	fprintf(stderr,	" -pass2     [filename]          : twopass mode (2nd pass)\n");
	fprintf(stderr,	" -zq starting_frame float       : bitrate zone; quant\n");
	fprintf(stderr,	" -zw starting_frame float       : bitrate zone; weight\n");
    fprintf(stderr, " -max_key_interval integer      : maximum keyframe interval (300)\n");
    fprintf(stderr, "\n");
	fprintf(stderr, "Single Pass options:\n");
	fprintf(stderr, "-reaction   integer             : reaction delay factor (16)\n");
	fprintf(stderr, "-averaging  integer             : averaging period (100)\n");
	fprintf(stderr, "-smoother   integer             : smoothing buffer (100)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Second Pass options:\n");
	fprintf(stderr, "-kboost     integer             : I frame boost (10)\n");
	fprintf(stderr, "-kthresh    integer             : I frame reduction threshold (1)\n");
	fprintf(stderr, "-kreduction integer             : I frame reduction amount (20)\n");
	fprintf(stderr, "-ostrength  integer             : overflow control strength (5)\n");
	fprintf(stderr, "-oimprove   integer             : max overflow improvement (5)\n");
	fprintf(stderr, "-odegrade   integer             : max overflow degradation (5)\n");
	fprintf(stderr, "-chigh      integer             : high bitrate scenes degradation (0)\n");
	fprintf(stderr, "-clow       integer             : low bitrate scenes improvement (0)\n");
	fprintf(stderr, "-overhead   integer             : container frame overhead (0)\n");
	fprintf(stderr, "-vbvsize    integer             : use vbv buffer size\n");
	fprintf(stderr, "-vbvmax     integer             : vbv max bitrate\n");
	fprintf(stderr, "-vbvpeak    integer             : vbv peak bitrate over 1 second\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "Other options\n");
	fprintf(stderr, " -noasm                         : do not use assembly optmized code\n");
	fprintf(stderr, " -turbo                         : use turbo presets for higher encoding speed\n");
	fprintf(stderr, " -quality integer               : quality ([0..%d]) (6)\n", ME_ELEMENTS - 1);
	fprintf(stderr, " -vhqmode integer               : level of R-D optimizations ([0..4]) (1)\n");
	fprintf(stderr, " -bvhq                          : use R-D optimizations for B-frames\n");
	fprintf(stderr, " -metric integer                : distortion metric for R-D opt (PSNR:0, PSNRHVSM: 1)\n");
	fprintf(stderr, " -qpel                          : use quarter pixel ME\n");
	fprintf(stderr, " -gmc                           : use global motion compensation\n");
	fprintf(stderr, " -qtype   integer               : quantization type (H263:0, MPEG4:1) (0)\n");
	fprintf(stderr, " -qmatrix filename              : use custom MPEG4 quantization matrix\n");
	fprintf(stderr, " -interlaced [integer]          : interlaced encoding (BFF:1, TFF:2) (1)\n");
	fprintf(stderr, " -nopacked                      : Disable packed mode\n");
	fprintf(stderr, " -noclosed_gop                  : Disable closed GOP mode\n");
	fprintf(stderr, " -masking [integer]             : HVS masking mode (None:0, Lumi:1, Variance:2) (0)\n");
	fprintf(stderr, " -stats                         : print stats about encoded frames\n");
	fprintf(stderr, " -ssim [integer]                : prints ssim for every frame (accurate: 0 fast: 4) (2)\n");
	fprintf(stderr, " -ssim_file filename            : outputs the ssim stats into a file\n");
	fprintf(stderr, " -psnrhvsm                      : prints PSNRHVSM metric for every frame\n");
	fprintf(stderr, " -debug                         : activates xvidcore internal debugging output\n");
	fprintf(stderr, " -vop_debug                     : print some info directly into encoded frames\n");
	fprintf(stderr, " -nochromame                    : Disable chroma motion estimation\n");
	fprintf(stderr, " -notrellis                     : Disable trellis quantization\n");
	fprintf(stderr, " -imin    integer               : Minimum I Quantizer (1..31) (2)\n");
	fprintf(stderr, " -imax    integer               : Maximum I quantizer (1..31) (31)\n");
	fprintf(stderr, " -bmin    integer               : Minimum B Quantizer (1..31) (2)\n");
	fprintf(stderr, " -bmax    integer               : Maximum B quantizer (1..31) (31)\n");
	fprintf(stderr, " -pmin    integer               : Minimum P Quantizer (1..31) (2)\n");
	fprintf(stderr, " -pmax    integer               : Maximum P quantizer (1..31) (31)\n");
	fprintf(stderr, " -drop    integer               : Frame Drop Ratio (0..100) (0)\n");
	fprintf(stderr, " -start   integer               : Starting frame number\n");
	fprintf(stderr, " -threads integer               : Number of threads\n");
	fprintf(stderr, " -slices  integer               : Number of slices\n");
	fprintf(stderr, " -progress [integer]            : Show progress updates every n frames (10)\n");
	fprintf(stderr, " -par     integer[:integer]     : Set Pixel Aspect Ratio.\n");
	fprintf(stderr, "                                  1 = 1:1\n");
	fprintf(stderr, "                                  2 = 12:11 (4:3 PAL)\n");
	fprintf(stderr, "                                  3 = 10:11 (4:3 NTSC)\n");
	fprintf(stderr, "                                  4 = 16:11 (16:9 PAL)\n");
	fprintf(stderr, "                                  5 = 40:33 (16:9 NTSC)\n");
	fprintf(stderr, "                              other = custom (width:height)\n");
	fprintf(stderr, " -help                          : prints this help message\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "NB: You can define %d zones repeating the -z[qw] option as needed.\n", MAX_ZONES);
}

/*****************************************************************************
 *                       Input and output functions
 *
 *      the are small and simple routines to read and write PGM and YUV
 *      image. It's just for convenience, again nothing specific to Xvid
 *
 *****************************************************************************/

#ifndef READ_PNM
static int
read_pgmheader(FILE * handle)
{
	int bytes, xsize, ysize, depth;
	char dummy[2];

	bytes = (int) fread(dummy, 1, 2, handle);

	if ((bytes < 2) || (dummy[0] != 'P') || (dummy[1] != '5'))
		return (1);

	fscanf(handle, "%d %d %d", &xsize, &ysize, &depth);
	if ((xsize > 4096) || (ysize > 4096*3/2) || (depth != 255)) {
		fprintf(stderr, "%d %d %d\n", xsize, ysize, depth);
		return (2);
	}
	if ((XDIM == 0) || (YDIM == 0)) {
		XDIM = xsize;
		YDIM = ysize * 2 / 3;
	}

	return (0);
}

static int
read_pgmdata(FILE * handle,
			 unsigned char *image)
{
	int i;
	char dummy;

	unsigned char *y = image;
	unsigned char *u = image + XDIM * YDIM;
	unsigned char *v = image + XDIM * YDIM + XDIM / 2 * YDIM / 2;

	/* read Y component of picture */
	fread(y, 1, XDIM * YDIM, handle);

	for (i = 0; i < YDIM / 2; i++) {
		/* read U */
		fread(u, 1, XDIM / 2, handle);

		/* read V */
		fread(v, 1, XDIM / 2, handle);

		/* Update pointers */
		u += XDIM / 2;
		v += XDIM / 2;
	}

	/*  I don't know why, but this seems needed */
	fread(&dummy, 1, 1, handle);

	return (0);
}
#else
static int
read_pnmheader(FILE * handle)
{
	int bytes, xsize, ysize, depth;
	char dummy[2];

	bytes = fread(dummy, 1, 2, handle);

	if ((bytes < 2) || (dummy[0] != 'P') || (dummy[1] != '6'))
		return (1);

	fscanf(handle, "%d %d %d", &xsize, &ysize, &depth);
	if ((xsize > 1440) || (ysize > 2880) || (depth != 255)) {
		fprintf(stderr, "%d %d %d\n", xsize, ysize, depth);
		return (2);
	}

	XDIM = xsize;
	YDIM = ysize;

	return (0);
}

static int
read_pnmdata(FILE * handle,
			 unsigned char *image)
{
	int i;
	char dummy;

	/* read Y component of picture */
	fread(image, 1, XDIM * YDIM * 3, handle);

	/*  I don't know why, but this seems needed */
	fread(&dummy, 1, 1, handle);

	return (0);
}
#endif

static int
read_yuvdata(FILE * handle,
			 unsigned char *image)
{

	if (fread(image, 1, IMAGE_SIZE(XDIM, YDIM), handle) !=
		(unsigned int) IMAGE_SIZE(XDIM, YDIM))
		return (1);
	else
		return (0);
}

/*****************************************************************************
 *     Routines for encoding: init encoder, frame step, release encoder
 ****************************************************************************/

/* sample plugin */

int
rawenc_debug(void *handle,
			 int opt,
			 void *param1,
			 void *param2)
{
	switch (opt) {
	case XVID_PLG_INFO:
		{
			xvid_plg_info_t *info = (xvid_plg_info_t *) param1;

			info->flags = XVID_REQDQUANTS;
			return 0;
		}

	case XVID_PLG_CREATE:
	case XVID_PLG_DESTROY:
	case XVID_PLG_BEFORE:
		return 0;

	case XVID_PLG_AFTER:
		{
			xvid_plg_data_t *data = (xvid_plg_data_t *) param1;
			int i, j;

			printf("---[ frame: %5i   quant: %2i   length: %6i ]---\n",
				   data->frame_num, data->quant, data->length);
			for (j = 0; j < data->mb_height; j++) {
				for (i = 0; i < data->mb_width; i++)
					printf("%2i ", data->dquant[j * data->dquant_stride + i]);
				printf("\n");
			}

			return 0;
		}
	}

	return XVID_ERR_FAIL;
}


#define FRAMERATE_INCR 1001

/* Gobal encoder init, once per process */
void 
enc_gbl(int use_assembler)
{
	xvid_gbl_init_t xvid_gbl_init;

	/*------------------------------------------------------------------------
	 * Xvid core initialization
	 *----------------------------------------------------------------------*/

	/* Set version -- version checking will done by xvidcore */
	memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init));
	xvid_gbl_init.version = XVID_VERSION;
    xvid_gbl_init.debug = ARG_DEBUG;


	/* Do we have to enable ASM optimizations ? */
	if (use_assembler) {

#ifdef ARCH_IS_IA64
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE | XVID_CPU_ASM;
#else
		xvid_gbl_init.cpu_flags = 0;
#endif
	} else {
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE;
	}

	/* Initialize Xvid core -- Should be done once per __process__ */
	xvid_global(NULL, XVID_GBL_INIT, &xvid_gbl_init, NULL);
    ARG_CPU_FLAGS = xvid_gbl_init.cpu_flags;
	enc_info();
}

/* Initialize encoder for first use, pass all needed parameters to the codec */
static int
enc_init(void **enc_handle, char *stats_pass1, int start_num)
{
	int xerr;
	//xvid_plugin_cbr_t cbr;
    xvid_plugin_single_t single;
	xvid_plugin_2pass1_t rc2pass1;
	xvid_plugin_2pass2_t rc2pass2;
	xvid_plugin_ssim_t ssim;
    xvid_plugin_lumimasking_t masking;
	//xvid_plugin_fixed_t rcfixed;
	xvid_enc_plugin_t plugins[8];
	xvid_enc_create_t xvid_enc_create;
	int i;

	/*------------------------------------------------------------------------
	 * Xvid encoder initialization
	 *----------------------------------------------------------------------*/

	/* Version again */
	memset(&xvid_enc_create, 0, sizeof(xvid_enc_create));
	xvid_enc_create.version = XVID_VERSION;

	/* Width and Height of input frames */
	xvid_enc_create.width = XDIM;
	xvid_enc_create.height = YDIM;
	xvid_enc_create.profile = 0xf5; /* Unrestricted */

	/* init plugins  */
//    xvid_enc_create.zones = ZONES;
//    xvid_enc_create.num_zones = NUM_ZONES;

	xvid_enc_create.plugins = plugins;
	xvid_enc_create.num_plugins = 0;

	if (ARG_SINGLE) {
		memset(&single, 0, sizeof(xvid_plugin_single_t));
		single.version = XVID_VERSION;
		single.bitrate = ARG_BITRATE;
		single.reaction_delay_factor = ARG_REACTION;
		single.averaging_period = ARG_AVERAGING;
		single.buffer = ARG_SMOOTHER;
		

		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_single;
		plugins[xvid_enc_create.num_plugins].param = &single;
		xvid_enc_create.num_plugins++;
		if (!ARG_BITRATE)
			prepare_cquant_zones();
	}

	if (ARG_PASS2) {
		memset(&rc2pass2, 0, sizeof(xvid_plugin_2pass2_t));
		rc2pass2.version = XVID_VERSION;
		rc2pass2.filename = ARG_PASS2;
		rc2pass2.bitrate = ARG_BITRATE;

		rc2pass2.keyframe_boost = ARG_KBOOST;
		rc2pass2.curve_compression_high = ARG_CHIGH;
		rc2pass2.curve_compression_low = ARG_CLOW;
		rc2pass2.overflow_control_strength = ARG_OVERSTRENGTH;
		rc2pass2.max_overflow_improvement = ARG_OVERIMPROVE;
		rc2pass2.max_overflow_degradation = ARG_OVERDEGRADE;
		rc2pass2.kfreduction = ARG_KREDUCTION;
		rc2pass2.kfthreshold = ARG_KTHRESH;
		rc2pass2.container_frame_overhead = ARG_OVERHEAD;

//		An example of activating VBV could look like this 
		rc2pass2.vbv_size     =  ARG_VBVSIZE;
		rc2pass2.vbv_initial  =  (ARG_VBVSIZE*3)/4;
		rc2pass2.vbv_maxrate  =  ARG_VBVMAXRATE;
		rc2pass2.vbv_peakrate =  ARG_VBVPEAKRATE;


		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass2;
		plugins[xvid_enc_create.num_plugins].param = &rc2pass2;
		xvid_enc_create.num_plugins++;
	}

	if (stats_pass1) {
		memset(&rc2pass1, 0, sizeof(xvid_plugin_2pass1_t));
		rc2pass1.version = XVID_VERSION;
		rc2pass1.filename = stats_pass1;
		if (ARG_FULL1PASS)
			prepare_full1pass_zones();
		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_2pass1;
		plugins[xvid_enc_create.num_plugins].param = &rc2pass1;
		xvid_enc_create.num_plugins++;
	}

	/* Zones stuff */
	xvid_enc_create.zones = (xvid_enc_zone_t*)malloc(sizeof(xvid_enc_zone_t) * NUM_ZONES);
	xvid_enc_create.num_zones = NUM_ZONES;
	for (i=0; i < xvid_enc_create.num_zones; i++) {
		xvid_enc_create.zones[i].frame = ZONES[i].frame;
		xvid_enc_create.zones[i].base = 100;
		xvid_enc_create.zones[i].mode = ZONES[i].mode;
		xvid_enc_create.zones[i].increment = ZONES[i].modifier;
	}


	if (ARG_LUMIMASKING) {
		memset(&masking, 0, sizeof(xvid_plugin_lumimasking_t));
		masking.method = (ARG_LUMIMASKING==2);
		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_lumimasking;
		plugins[xvid_enc_create.num_plugins].param = &masking;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_DUMP) {
		plugins[xvid_enc_create.num_plugins].func = xvid_plugin_dump;
		plugins[xvid_enc_create.num_plugins].param = NULL;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_SSIM>=0 || ARG_SSIM_PATH != NULL) {
        memset(&ssim, 0, sizeof(xvid_plugin_ssim_t));

        plugins[xvid_enc_create.num_plugins].func = xvid_plugin_ssim;

		if( ARG_SSIM >=0){
			ssim.b_printstat = 1;
			ssim.acc = ARG_SSIM;
		} else {
			ssim.b_printstat = 0;
			ssim.acc = 2;
		}

		if(ARG_SSIM_PATH != NULL){		
			ssim.stat_path = ARG_SSIM_PATH;
		}

        ssim.cpu_flags = ARG_CPU_FLAGS;
		ssim.b_visualize = 0;
		plugins[xvid_enc_create.num_plugins].param = &ssim;
		xvid_enc_create.num_plugins++;
	}

	if (ARG_PSNRHVSM>0) {
        plugins[xvid_enc_create.num_plugins].func = xvid_plugin_psnrhvsm;
		plugins[xvid_enc_create.num_plugins].param = NULL;
		xvid_enc_create.num_plugins++;
	}

#if 0
	if (ARG_DEBUG) {
		plugins[xvid_enc_create.num_plugins].func = rawenc_debug;
		plugins[xvid_enc_create.num_plugins].param = NULL;
		xvid_enc_create.num_plugins++;
	}
#endif

	xvid_enc_create.num_threads = ARG_THREADS;
	xvid_enc_create.num_slices  = ARG_SLICES;

	/* Frame rate  */
	xvid_enc_create.fincr = ARG_DWSCALE;
	xvid_enc_create.fbase = ARG_DWRATE;

	/* Maximum key frame interval */
    if (ARG_MAXKEYINTERVAL > 0) {
        xvid_enc_create.max_key_interval = ARG_MAXKEYINTERVAL;
    }else {
	    xvid_enc_create.max_key_interval = (int) ARG_FRAMERATE *10;
    }

	xvid_enc_create.min_quant[0]=ARG_QUANTS[0];
	xvid_enc_create.min_quant[1]=ARG_QUANTS[2];
	xvid_enc_create.min_quant[2]=ARG_QUANTS[4];
	xvid_enc_create.max_quant[0]=ARG_QUANTS[1];
	xvid_enc_create.max_quant[1]=ARG_QUANTS[3];
	xvid_enc_create.max_quant[2]=ARG_QUANTS[5];

	/* Bframes settings */
	xvid_enc_create.max_bframes = ARG_MAXBFRAMES;
	xvid_enc_create.bquant_ratio = ARG_BQRATIO;
	xvid_enc_create.bquant_offset = ARG_BQOFFSET;

	/* Frame drop ratio */
	xvid_enc_create.frame_drop_ratio = ARG_FRAMEDROP;

	/* Start frame number */
	xvid_enc_create.start_frame_num = start_num;

	/* Global encoder options */
	xvid_enc_create.global = 0;

	if (ARG_PACKED)
		xvid_enc_create.global |= XVID_GLOBAL_PACKED;

	if (ARG_CLOSED_GOP)
		xvid_enc_create.global |= XVID_GLOBAL_CLOSED_GOP;

	if (ARG_STATS)
		xvid_enc_create.global |= XVID_GLOBAL_EXTRASTATS_ENABLE;

	/* I use a small value here, since will not encode whole movies, but short clips */
	xerr = xvid_encore(NULL, XVID_ENC_CREATE, &xvid_enc_create, NULL);

	/* Retrieve the encoder instance from the structure */
	*enc_handle = xvid_enc_create.handle;

	free(xvid_enc_create.zones);

	return (xerr);
}

static int
enc_info()
{
	xvid_gbl_info_t xvid_gbl_info;
	int ret;

	memset(&xvid_gbl_info, 0, sizeof(xvid_gbl_info));
	xvid_gbl_info.version = XVID_VERSION;
	ret = xvid_global(NULL, XVID_GBL_INFO, &xvid_gbl_info, NULL);
	if (xvid_gbl_info.build != NULL) {
		fprintf(stderr, "xvidcore build version: %s\n", xvid_gbl_info.build);
	}
	fprintf(stderr, "Bitstream version: %d.%d.%d\n", XVID_VERSION_MAJOR(xvid_gbl_info.actual_version), XVID_VERSION_MINOR(xvid_gbl_info.actual_version), XVID_VERSION_PATCH(xvid_gbl_info.actual_version));
	fprintf(stderr, "Detected CPU flags: ");
	if (xvid_gbl_info.cpu_flags & XVID_CPU_ASM)
		fprintf(stderr, "ASM ");
	if (xvid_gbl_info.cpu_flags & XVID_CPU_MMX)
		fprintf(stderr, "MMX ");
	if (xvid_gbl_info.cpu_flags & XVID_CPU_MMXEXT)
		fprintf(stderr, "MMXEXT ");
	if (xvid_gbl_info.cpu_flags & XVID_CPU_SSE)
		fprintf(stderr, "SSE ");
	if (xvid_gbl_info.cpu_flags & XVID_CPU_SSE2)
		fprintf(stderr, "SSE2 ");
	if (xvid_gbl_info.cpu_flags & XVID_CPU_SSE3)
		fprintf(stderr, "SSE3 ");
	if (xvid_gbl_info.cpu_flags & XVID_CPU_SSE41)
		fprintf(stderr, "SSE41 ");
    if (xvid_gbl_info.cpu_flags & XVID_CPU_3DNOW)
		fprintf(stderr, "3DNOW ");
	if (xvid_gbl_info.cpu_flags & XVID_CPU_3DNOWEXT)
		fprintf(stderr, "3DNOWEXT ");
	if (xvid_gbl_info.cpu_flags & XVID_CPU_TSC)
		fprintf(stderr, "TSC ");
	fprintf(stderr, "\n");
	fprintf(stderr, "Detected %d cpus,", xvid_gbl_info.num_threads);
	ARG_NUM_APP_THREADS = xvid_gbl_info.num_threads;
	fprintf(stderr, " using %d threads.\n", (!ARG_THREADS) ? ARG_NUM_APP_THREADS : ARG_THREADS);
	return ret;
}

static int
enc_stop(void *enc_handle)
{
	int xerr;

	/* Destroy the encoder instance */
	xerr = xvid_encore(enc_handle, XVID_ENC_DESTROY, NULL, NULL);

	return (xerr);
}

static int
enc_main(void *enc_handle,
		 unsigned char *image,
		 unsigned char *bitstream,
		 int *key,
		 int *stats_type,
		 int *stats_quant,
		 int *stats_length,
		 int sse[3],
		 int framenum)
{
	int ret;

	xvid_enc_frame_t xvid_enc_frame;
	xvid_enc_stats_t xvid_enc_stats;

	/* Version for the frame and the stats */
	memset(&xvid_enc_frame, 0, sizeof(xvid_enc_frame));
	xvid_enc_frame.version = XVID_VERSION;

	memset(&xvid_enc_stats, 0, sizeof(xvid_enc_stats));
	xvid_enc_stats.version = XVID_VERSION;

	/* Bind output buffer */
	xvid_enc_frame.bitstream = bitstream;
	xvid_enc_frame.length = -1;

	/* Initialize input image fields */
	if (image) {
		xvid_enc_frame.input.plane[0] = image;
#ifndef READ_PNM
		xvid_enc_frame.input.csp = ARG_COLORSPACE;
		xvid_enc_frame.input.stride[0] = XDIM;
#else
		xvid_enc_frame.input.csp = XVID_CSP_BGR;
		xvid_enc_frame.input.stride[0] = XDIM*3;
#endif
	} else {
		xvid_enc_frame.input.csp = XVID_CSP_NULL;
	}

	/* Set up core's general features */
	xvid_enc_frame.vol_flags = 0;
	if (ARG_STATS)
		xvid_enc_frame.vol_flags |= XVID_VOL_EXTRASTATS;
	if (ARG_QTYPE) {
		xvid_enc_frame.vol_flags |= XVID_VOL_MPEGQUANT;
		if (ARG_QMATRIX) {
			xvid_enc_frame.quant_intra_matrix = qmatrix_intra;
			xvid_enc_frame.quant_inter_matrix = qmatrix_inter;
		}
		else {
			/* We don't use special matrices */
			xvid_enc_frame.quant_intra_matrix = NULL;
			xvid_enc_frame.quant_inter_matrix = NULL;
		}
	}

	if (ARG_PAR)
		xvid_enc_frame.par = ARG_PAR;
	else {
		xvid_enc_frame.par = XVID_PAR_EXT;
		xvid_enc_frame.par_width = ARG_PARWIDTH;
		xvid_enc_frame.par_height = ARG_PARHEIGHT;
	}


	if (ARG_QPEL) {
		xvid_enc_frame.vol_flags |= XVID_VOL_QUARTERPEL;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE16 | XVID_ME_QUARTERPELREFINE8;
	}
	if (ARG_GMC) {
		xvid_enc_frame.vol_flags |= XVID_VOL_GMC;
		xvid_enc_frame.motion |= XVID_ME_GME_REFINE;
	}

	/* Set up core's general features */
	xvid_enc_frame.vop_flags = vop_presets[ARG_QUALITY];

	if (ARG_INTERLACING) {
		xvid_enc_frame.vol_flags |= XVID_VOL_INTERLACING;
		if (ARG_INTERLACING == 2)
			xvid_enc_frame.vop_flags |= XVID_VOP_TOPFIELDFIRST;
	}

	xvid_enc_frame.vop_flags |= XVID_VOP_HALFPEL;
	xvid_enc_frame.vop_flags |= XVID_VOP_HQACPRED;

    if (ARG_VOPDEBUG) {
        xvid_enc_frame.vop_flags |= XVID_VOP_DEBUG;
    }

    if (ARG_TRELLIS) {
        xvid_enc_frame.vop_flags |= XVID_VOP_TRELLISQUANT;
    }

	/* Frame type -- taken from function call parameter */
	/* Sometimes we might want to force the last frame to be a P Frame */
	xvid_enc_frame.type = *stats_type;

	/* Force the right quantizer -- It is internally managed by RC plugins */
	xvid_enc_frame.quant = 0;

	if (ARG_CHROMAME)
		xvid_enc_frame.motion |= XVID_ME_CHROMA_PVOP + XVID_ME_CHROMA_BVOP;

	/* Set up motion estimation flags */
	xvid_enc_frame.motion |= motion_presets[ARG_QUALITY];

	if (ARG_TURBO)
		xvid_enc_frame.motion |= XVID_ME_FASTREFINE16 | XVID_ME_FASTREFINE8 | 
								 XVID_ME_SKIP_DELTASEARCH | XVID_ME_FAST_MODEINTERPOLATE | 
								 XVID_ME_BFRAME_EARLYSTOP;

	if (ARG_BVHQ) 
		xvid_enc_frame.vop_flags |= XVID_VOP_RD_BVOP;

	if (ARG_QMETRIC == 1)
		xvid_enc_frame.vop_flags |= XVID_VOP_RD_PSNRHVSM;

	switch (ARG_VHQMODE) /* this is the same code as for vfw */
	{
	case 1: /* VHQ_MODE_DECISION */
		xvid_enc_frame.vop_flags |= XVID_VOP_MODEDECISION_RD;
		break;

	case 2: /* VHQ_LIMITED_SEARCH */
		xvid_enc_frame.vop_flags |= XVID_VOP_MODEDECISION_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE16_RD;
		break;

	case 3: /* VHQ_MEDIUM_SEARCH */
		xvid_enc_frame.vop_flags |= XVID_VOP_MODEDECISION_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE8_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE8_RD;
		xvid_enc_frame.motion |= XVID_ME_CHECKPREDICTION_RD;
		break;

	case 4: /* VHQ_WIDE_SEARCH */
		xvid_enc_frame.vop_flags |= XVID_VOP_MODEDECISION_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_HALFPELREFINE8_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE16_RD;
		xvid_enc_frame.motion |= XVID_ME_QUARTERPELREFINE8_RD;
		xvid_enc_frame.motion |= XVID_ME_CHECKPREDICTION_RD;
		xvid_enc_frame.motion |= XVID_ME_EXTSEARCH_RD;
		break;

	default :
		break;
	}

	/* Not sure what this does */
	// force keyframe spacing in 2-pass 1st pass
	if (ARG_QUALITY == 0)
		xvid_enc_frame.type = XVID_TYPE_IVOP;

	/* frame-based stuff */
	apply_zone_modifiers(&xvid_enc_frame, framenum);


	/* Encode the frame */
	ret = xvid_encore(enc_handle, XVID_ENC_ENCODE, &xvid_enc_frame,
					  &xvid_enc_stats);

	*key = (xvid_enc_frame.out_flags & XVID_KEYFRAME);
	*stats_type = xvid_enc_stats.type;
	*stats_quant = xvid_enc_stats.quant;
	*stats_length = xvid_enc_stats.length;
	sse[0] = xvid_enc_stats.sse_y;
	sse[1] = xvid_enc_stats.sse_u;
	sse[2] = xvid_enc_stats.sse_v;

	return (ret);
}

void
sort_zones(zone_t * zones, int zone_num, int * sel)
{
	int i, j;
	zone_t tmp;
	for (i = 0; i < zone_num; i++) {
		int cur = i;
		int min_f = zones[i].frame;
		for (j = i + 1; j < zone_num; j++) {
			if (zones[j].frame < min_f) {
				min_f = zones[j].frame;
				cur = j;
			}
		}
		if (cur != i) {
			tmp = zones[i];
			zones[i] = zones[cur];
			zones[cur] = tmp;
			if (i == *sel) *sel = cur;
			else if (cur == *sel) *sel = i;
		}
	}
}

/* constant-quant zones for fixed quant encoding */
static void
prepare_cquant_zones() {
	
	int i = 0;
	if (NUM_ZONES == 0 || ZONES[0].frame != 0) {
		/* first zone does not start at frame 0 or doesn't exist */

		if (NUM_ZONES >= MAX_ZONES) NUM_ZONES--; /* we sacrifice last zone */

		ZONES[NUM_ZONES].frame = 0;
		ZONES[NUM_ZONES].mode = XVID_ZONE_QUANT;
		ZONES[NUM_ZONES].modifier = (int) ARG_CQ;
		ZONES[NUM_ZONES].type = XVID_TYPE_AUTO;
		ZONES[NUM_ZONES].greyscale = 0;
		ZONES[NUM_ZONES].chroma_opt = 0;
		ZONES[NUM_ZONES].bvop_threshold = 0;
		ZONES[NUM_ZONES].cartoon_mode = 0;
		NUM_ZONES++;

		sort_zones(ZONES, NUM_ZONES, &i);
	}

	/* step 2: let's change all weight zones into quant zones */
	
	for(i = 0; i < NUM_ZONES; i++)
		if (ZONES[i].mode == XVID_ZONE_WEIGHT) {
			ZONES[i].mode = XVID_ZONE_QUANT;
			ZONES[i].modifier = (int) ((100*ARG_CQ) / ZONES[i].modifier);
		}
}

/* full first pass zones */
static void
prepare_full1pass_zones() {
	
	int i = 0;
	if (NUM_ZONES == 0 || ZONES[0].frame != 0) {
		/* first zone does not start at frame 0 or doesn't exist */

		if (NUM_ZONES >= MAX_ZONES) NUM_ZONES--; /* we sacrifice last zone */

		ZONES[NUM_ZONES].frame = 0;
		ZONES[NUM_ZONES].mode = XVID_ZONE_QUANT;
		ZONES[NUM_ZONES].modifier = 200;
		ZONES[NUM_ZONES].type = XVID_TYPE_AUTO;
		ZONES[NUM_ZONES].greyscale = 0;
		ZONES[NUM_ZONES].chroma_opt = 0;
		ZONES[NUM_ZONES].bvop_threshold = 0;
		ZONES[NUM_ZONES].cartoon_mode = 0;
		NUM_ZONES++;

		sort_zones(ZONES, NUM_ZONES, &i);
	}

	/* step 2: let's change all weight zones into quant zones */
	
	for(i = 0; i < NUM_ZONES; i++)
		if (ZONES[i].mode == XVID_ZONE_WEIGHT) {
			ZONES[i].mode = XVID_ZONE_QUANT;
			ZONES[i].modifier = 200;
		}
}

static void apply_zone_modifiers(xvid_enc_frame_t * frame, int framenum)
{
	int i;

	for (i=0; i<NUM_ZONES && ZONES[i].frame <= framenum; i++) ;

	if (--i < 0) return; /* there are no zones, or we're before the first zone */

	if (framenum == ZONES[i].frame)
		frame->type = ZONES[i].type;

	if (ZONES[i].greyscale) {
		frame->vop_flags |= XVID_VOP_GREYSCALE;
	}

	if (ZONES[i].chroma_opt) {
		frame->vop_flags |= XVID_VOP_CHROMAOPT;
	}

	if (ZONES[i].cartoon_mode) {
		frame->vop_flags |= XVID_VOP_CARTOON;
		frame->motion |= XVID_ME_DETECT_STATIC_MOTION;
	}

	if (ARG_MAXBFRAMES) {
		frame->bframe_threshold = ZONES[i].bvop_threshold;
	}
}

void removedivxp(char *buf, int bufsize) {
	int i;
	char* userdata;

	for (i=0; i <= (int)(bufsize-sizeof(userdata_start_code)); i++) {
		if (memcmp((void*)userdata_start_code, (void*)(buf+i), strlen(userdata_start_code))==0) {
			if ((userdata = strstr(buf+i+4, "DivX"))!=NULL) {
				userdata[strlen(userdata)-1] = '\0';
				return;
			}
		}
	}
}
