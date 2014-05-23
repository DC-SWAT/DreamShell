/*****************************************************************************
 *
 *  XVID MPEG-4 VIDEO CODEC
 *  - Console based decoding test application  -
 *
 *  Copyright(C) 2002-2003 Christoph Lampert
 *               2002-2003 Edouard Gomez <ed.gomez@free.fr>
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
 * $Id: xvid_decraw.c 1985 2011-05-18 09:02:35Z Isibaar $
 *
 ****************************************************************************/

/*****************************************************************************
 *		                    
 *  Application notes :
 *		                    
 *  An MPEG-4 bitstream is read from an input file (or stdin) and decoded,
 *  the speed for this is measured.
 *
 *  The program is plain C and needs no libraries except for libxvidcore, 
 *  and maths-lib.
 *		                   
 *  Use ./xvid_decraw -help for a list of options
 * 
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef WIN32
#include <sys/time.h>
#else
#include <time.h>
#endif

#include "xvid.h"

/*****************************************************************************
 *               Global vars in module and constants
 ****************************************************************************/

#define USE_PNM 0
#define USE_TGA 1
#define USE_YUV 2

static int XDIM = 0;
static int YDIM = 0;
static int ARG_SAVEDECOUTPUT = 0;
static int ARG_SAVEMPEGSTREAM = 0;
static char *ARG_INPUTFILE = NULL;
static int CSP = XVID_CSP_I420;
static int BPP = 1;
static int FORMAT = USE_PNM;
static int POSTPROC = 0;
static 	int ARG_THREADS = 0;

static char filepath[256] = "./";
static void *dec_handle = NULL;

#define BUFFER_SIZE (2*1024*1024)

static const int display_buffer_bytes = 0;

#define MIN_USEFUL_BYTES 1

/*****************************************************************************
 *               Local prototypes
 ****************************************************************************/

static double msecond();
static int dec_init(int use_assembler, int debug_level);
static int dec_main(unsigned char *istream,
					unsigned char *ostream,
					int istream_size,
					xvid_dec_stats_t *xvid_dec_stats);
static int dec_stop();
static void usage();
static int write_image(char *prefix, unsigned char *image, int filenr);
static int write_pnm(char *filename, unsigned char *image);
static int write_tga(char *filename, unsigned char *image);
static int write_yuv(char *filename, unsigned char *image);

const char * type2str(int type)
{
    if (type==XVID_TYPE_IVOP)
        return "I";
    if (type==XVID_TYPE_PVOP)
        return "P";
    if (type==XVID_TYPE_BVOP)
        return "B";
    return "S";
}

/*****************************************************************************
 *        Main program
 ****************************************************************************/

int main(int argc, char *argv[])
{
	unsigned char *mp4_buffer = NULL;
	unsigned char *mp4_ptr    = NULL;
	unsigned char *out_buffer = NULL;
	int useful_bytes;
	int chunk;
	xvid_dec_stats_t xvid_dec_stats;
	
	double totaldectime;
  
	long totalsize;
	int status;
  
	int use_assembler = 1;
	int debug_level = 0;
  
	char filename[256];
  
	FILE *in_file;
	int filenr;
	int i;

	printf("xvid_decraw - raw mpeg4 bitstream decoder ");
	printf("written by Christoph Lampert\n\n");

/*****************************************************************************
 * Command line parsing
 ****************************************************************************/

	for (i=1; i< argc; i++) {
 
		if (strcmp("-noasm", argv[i]) == 0 ) {
			use_assembler = 0;
		} else if (strcmp("-debug", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			if (sscanf(argv[i], "0x%x", &debug_level) != 1) {
				debug_level = atoi(argv[i]);
			}
		} else if (strcmp("-d", argv[i]) == 0) {
			ARG_SAVEDECOUTPUT = 1;
		} else if (strcmp("-i", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			ARG_INPUTFILE = argv[i];
		} else if (strcmp("-m", argv[i]) == 0) {
			ARG_SAVEMPEGSTREAM = 1;
		} else if (strcmp("-c", argv[i]) == 0  && i < argc - 1 ) {
			i++;
			if (strcmp(argv[i], "rgb16") == 0) {
				CSP = XVID_CSP_RGB555;
				BPP = 2;
			} else if (strcmp(argv[i], "rgb24") == 0) {
				CSP = XVID_CSP_BGR;
				BPP = 3;
			} else if (strcmp(argv[i], "rgb32") == 0) {
				CSP = XVID_CSP_BGRA;
				BPP = 4;
			} else if (strcmp(argv[i], "yv12") == 0) {
				CSP = XVID_CSP_YV12;
				BPP = 1;
			} else {
				CSP = XVID_CSP_I420;
				BPP = 1;
			}
		} else if (strcmp("-postproc", argv[i]) == 0 && i < argc - 1 ) {
			i++;
			POSTPROC = atoi(argv[i]);
			if (POSTPROC < 0) POSTPROC = 0;
			if (POSTPROC > 2) POSTPROC = 2;
		} else if (strcmp("-f", argv[i]) == 0 && i < argc -1) {
			i++;
			if (strcmp(argv[i], "tga") == 0) {
				FORMAT = USE_TGA;
			} else if (strcmp(argv[i], "yuv") == 0) {
				FORMAT = USE_YUV;
			} else {
				FORMAT = USE_PNM;
			}
		} else if (strcmp("-threads", argv[i]) == 0 && i < argc -1) {
			i++;
			ARG_THREADS = atoi(argv[i]);
		} else if (strcmp("-help", argv[i]) == 0) {
			usage();
			return(0);
		} else {
			usage();
			exit(-1);
		}
	}
  
#if defined(_MSC_VER)
	if (ARG_INPUTFILE==NULL) {
		fprintf(stderr, "Warning: MSVC build does not read EOF correctly from stdin. Use the -i switch.\n\n");
	}
#endif

/*****************************************************************************
 * Values checking
 ****************************************************************************/

	if ( ARG_INPUTFILE == NULL || strcmp(ARG_INPUTFILE, "stdin") == 0) {
		in_file = stdin;
	}
	else {

		in_file = fopen(ARG_INPUTFILE, "rb");
		if (in_file == NULL) {
			fprintf(stderr, "Error opening input file %s\n", ARG_INPUTFILE);
			return(-1);
		}
	}

	/* PNM/PGM format can't handle 16/32 bit data */
	if (BPP != 1 && BPP != 3 && FORMAT == USE_PNM) {
		FORMAT = USE_TGA;
	}
	if (BPP != 1 && FORMAT == USE_YUV) {
		FORMAT = USE_TGA;
	}

/*****************************************************************************
 *        Memory allocation
 ****************************************************************************/

	/* Memory for encoded mp4 stream */
	mp4_buffer = (unsigned char *) malloc(BUFFER_SIZE);
	mp4_ptr = mp4_buffer;
	if (!mp4_buffer)
		goto free_all_memory;	
    
/*****************************************************************************
 *        Xvid PART  Start
 ****************************************************************************/

	status = dec_init(use_assembler, debug_level);
	if (status) {
		fprintf(stderr,
				"Decore INIT problem, return value %d\n", status);
		goto release_all;
	}


/*****************************************************************************
 *	                         Main loop
 ****************************************************************************/

	/* Fill the buffer */
	useful_bytes = (int) fread(mp4_buffer, 1, BUFFER_SIZE, in_file);

	totaldectime = 0;
	totalsize = 0;
	filenr = 0;
	mp4_ptr = mp4_buffer;
	chunk = 0;
	
	do {
		int used_bytes = 0;
		double dectime;

		/*
		 * If the buffer is half empty or there are no more bytes in it
		 * then fill it.
		 */
		if (mp4_ptr > mp4_buffer + BUFFER_SIZE/2) {
			int already_in_buffer = (int)(mp4_buffer + BUFFER_SIZE - mp4_ptr);

			/* Move data if needed */
			if (already_in_buffer > 0)
				memcpy(mp4_buffer, mp4_ptr, already_in_buffer);

			/* Update mp4_ptr */
			mp4_ptr = mp4_buffer; 

			/* read new data */
            if(!feof(in_file)) {
				useful_bytes += (int) fread(mp4_buffer + already_in_buffer,
									        1, BUFFER_SIZE - already_in_buffer,
									        in_file);
			}
		}


		/* This loop is needed to handle VOL/NVOP reading */
		do {

			/* Decode frame */
			dectime = msecond();
			used_bytes = dec_main(mp4_ptr, out_buffer, useful_bytes, &xvid_dec_stats);
			dectime = msecond() - dectime;

			/* Resize image buffer if needed */
			if(xvid_dec_stats.type == XVID_TYPE_VOL) {

				/* Check if old buffer is smaller */
				if(XDIM*YDIM < xvid_dec_stats.data.vol.width*xvid_dec_stats.data.vol.height) {

					/* Copy new witdh and new height from the vol structure */
					XDIM = xvid_dec_stats.data.vol.width;
					YDIM = xvid_dec_stats.data.vol.height;

					/* Free old output buffer*/
					if(out_buffer) free(out_buffer);

					/* Allocate the new buffer */
					out_buffer = (unsigned char*)malloc(XDIM*YDIM*4);
					if(out_buffer == NULL)
						goto free_all_memory;

					fprintf(stderr, "Resized frame buffer to %dx%d\n", XDIM, YDIM);
				}

				/* Save individual mpeg4 stream if required */
				if(ARG_SAVEMPEGSTREAM) {
					FILE *filehandle = NULL;

					sprintf(filename, "%svolhdr.m4v", filepath);
					filehandle = fopen(filename, "wb");
					if(!filehandle) {
						fprintf(stderr,
								"Error writing vol header mpeg4 stream to file %s\n",
								filename);
					} else {
						fwrite(mp4_ptr, 1, used_bytes, filehandle);
						fclose(filehandle);
					}
				}
			}

			/* Update buffer pointers */
			if(used_bytes > 0) {
				mp4_ptr += used_bytes;
				useful_bytes -= used_bytes;

				/* Total size */
				totalsize += used_bytes;
			}

			if (display_buffer_bytes) {
				printf("Data chunk %d: %d bytes consumed, %d bytes in buffer\n", chunk++, used_bytes, useful_bytes);
			}
		} while (xvid_dec_stats.type <= 0 && useful_bytes > MIN_USEFUL_BYTES);

		/* Check if there is a negative number of useful bytes left in buffer
		 * This means we went too far */
        if(useful_bytes < 0)
            break;
		
    	/* Updated data - Count only usefull decode time */
		totaldectime += dectime;

			
		if (!display_buffer_bytes) {
			printf("Frame %5d: type = %s, dectime(ms) =%6.1f, length(bytes) =%7d\n",
					filenr, type2str(xvid_dec_stats.type), dectime, used_bytes);
		}

		/* Save individual mpeg4 stream if required */
		if(ARG_SAVEMPEGSTREAM) {
			FILE *filehandle = NULL;

			sprintf(filename, "%sframe%05d.m4v", filepath, filenr);
			filehandle = fopen(filename, "wb");
			if(!filehandle) {
				fprintf(stderr,
						"Error writing single mpeg4 stream to file %s\n",
						filename);
			}
			else {
				fwrite(mp4_ptr-used_bytes, 1, used_bytes, filehandle);
				fclose(filehandle);
			}
		}
				
		/* Save output frame if required */
		if (ARG_SAVEDECOUTPUT) {
			sprintf(filename, "%sdec", filepath);

			if(write_image(filename, out_buffer, filenr)) {
				fprintf(stderr,
						"Error writing decoded frame %s\n",
						filename);
			}
		}

		filenr++;

	} while (useful_bytes>MIN_USEFUL_BYTES || !feof(in_file));

	useful_bytes = 0; /* Empty buffer */

/*****************************************************************************
 *     Flush decoder buffers
 ****************************************************************************/

	do {

		/* Fake vars */
		int used_bytes;
		double dectime;

        do {
		    dectime = msecond();
		    used_bytes = dec_main(NULL, out_buffer, -1, &xvid_dec_stats);
		    dectime = msecond() - dectime;
			if (display_buffer_bytes) {
				printf("Data chunk %d: %d bytes consumed, %d bytes in buffer\n", chunk++, used_bytes, useful_bytes);
			}
        } while(used_bytes>=0 && xvid_dec_stats.type <= 0);

        if (used_bytes < 0) {   /* XVID_ERR_END */
            break;
        }

		/* Updated data - Count only usefull decode time */
		totaldectime += dectime;

		/* Prints some decoding stats */
		if (!display_buffer_bytes) {
			printf("Frame %5d: type = %s, dectime(ms) =%6.1f, length(bytes) =%7d\n",
					filenr, type2str(xvid_dec_stats.type), dectime, used_bytes);
		}

		/* Save output frame if required */
		if (ARG_SAVEDECOUTPUT) {
			sprintf(filename, "%sdec", filepath);

			if(write_image(filename, out_buffer, filenr)) {
				fprintf(stderr,
						"Error writing decoded frame %s\n",
						filename);
			}
		}

		filenr++;

	}while(1);
	
/*****************************************************************************
 *     Calculate totals and averages for output, print results
 ****************************************************************************/

	if (filenr>0) {
		totalsize    /= filenr;
		totaldectime /= filenr;
		printf("Avg: dectime(ms) =%7.2f, fps =%7.2f, length(bytes) =%7d\n",
			   totaldectime, 1000/totaldectime, (int)totalsize);
	}else{
		printf("Nothing was decoded!\n");
	}
		
/*****************************************************************************
 *      Xvid PART  Stop
 ****************************************************************************/

 release_all:
  	if (dec_handle) {
	  	status = dec_stop();
		if (status)    
			fprintf(stderr, "decore RELEASE problem return value %d\n", status);
	}

 free_all_memory:
	free(out_buffer);
	free(mp4_buffer);

	return(0);
}

/*****************************************************************************
 *               Usage function
 ****************************************************************************/

static void usage()
{

	fprintf(stderr, "Usage : xvid_decraw [OPTIONS]\n");
	fprintf(stderr, "Options :\n");
	fprintf(stderr, " -noasm         : don't use assembly optimizations (default=enabled)\n");
	fprintf(stderr, " -debug         : debug level (debug=0)\n");
	fprintf(stderr, " -i string      : input filename (default=stdin)\n");
	fprintf(stderr, " -d             : save decoder output\n");
	fprintf(stderr, " -c csp         : choose colorspace output (rgb16, rgb24, rgb32, yv12, i420)\n");
	fprintf(stderr, " -f format      : choose output file format (tga, pnm, pgm, yuv)\n");
	fprintf(stderr, " -postproc      : postprocessing level (0=off, 1=deblock, 2=deblock+dering)\n");
	fprintf(stderr, " -threads int   : number of threads\n");
	fprintf(stderr, " -m             : save mpeg4 raw stream to individual files\n");
	fprintf(stderr, " -help          : This help message\n");
	fprintf(stderr, " (* means default)\n");

}

/*****************************************************************************
 *               "helper" functions
 ****************************************************************************/

/* return the current time in milli seconds */
static double
msecond()
{	
#ifndef WIN32
	struct timeval  tv;
	gettimeofday(&tv, 0);
	return((double)tv.tv_sec*1.0e3 + (double)tv.tv_usec*1.0e-3);
#else
	clock_t clk;
	clk = clock();
	return(clk * 1000.0 / CLOCKS_PER_SEC);
#endif
}

/*****************************************************************************
 *              output functions
 ****************************************************************************/

static int write_image(char *prefix, unsigned char *image, int filenr)
{
	char filename[1024];
	char *ext;
	int ret;

	if (FORMAT == USE_PNM && BPP == 1) {
		ext = "pgm";
	} else if (FORMAT == USE_PNM && BPP == 3) {
		ext = "pnm";
	} else if (FORMAT == USE_YUV) {
		ext = "yuv";
	} else if (FORMAT == USE_TGA) {
		ext = "tga";
	} else {
		fprintf(stderr, "Bug: should not reach this path code -- please report to xvid-devel@xvid.org with command line options used");
		exit(-1);
	}

	if (FORMAT == USE_YUV) {
		sprintf(filename, "%s.%s", prefix, ext);

		if (!filenr) { 
			FILE *fp = fopen(filename, "wb"); 
			fclose(fp); 
		}
	} else
		sprintf(filename, "%s%05d.%s", prefix, filenr, ext);

	if (FORMAT == USE_PNM) {
		ret = write_pnm(filename, image);
	} else if (FORMAT == USE_YUV) {
		ret = write_yuv(filename, image);
	} else {
		ret = write_tga(filename, image);
	}

	return(ret);
}

static int write_tga(char *filename, unsigned char *image)
{
	FILE * f;
	char hdr[18];

	f = fopen(filename, "wb");
	if ( f == NULL) {
		return -1;
	}

	hdr[0]  = 0; /* ID length */
	hdr[1]  = 0; /* Color map type */
	hdr[2]  = (BPP>1)?2:3; /* Uncompressed true color (2) or greymap (3) */
	hdr[3]  = 0; /* Color map specification (not used) */
	hdr[4]  = 0; /* Color map specification (not used) */
	hdr[5]  = 0; /* Color map specification (not used) */
	hdr[6]  = 0; /* Color map specification (not used) */
	hdr[7]  = 0; /* Color map specification (not used) */
	hdr[8]  = 0; /* LSB X origin */
	hdr[9]  = 0; /* MSB X origin */
	hdr[10] = 0; /* LSB Y origin */
	hdr[11] = 0; /* MSB Y origin */
	hdr[12] = (XDIM>>0)&0xff; /* LSB Width */
	hdr[13] = (XDIM>>8)&0xff; /* MSB Width */
	if (BPP > 1) {
		hdr[14] = (YDIM>>0)&0xff; /* LSB Height */
		hdr[15] = (YDIM>>8)&0xff; /* MSB Height */
	} else {
		hdr[14] = ((YDIM*3)>>1)&0xff; /* LSB Height */
		hdr[15] = ((YDIM*3)>>9)&0xff; /* MSB Height */
	}
	hdr[16] = BPP*8;
	hdr[17] = 0x00 | (1<<5) /* Up to down */ | (0<<4); /* Image descriptor */
	
	/* Write header */
	fwrite(hdr, 1, sizeof(hdr), f);

#ifdef ARCH_IS_LITTLE_ENDIAN
	/* write first plane */
	fwrite(image, 1, XDIM*YDIM*BPP, f);
#else
	{
		int i;
		for (i=0; i<XDIM*YDIM*BPP;i+=BPP) {
			if (BPP == 1) {
				fputc(*(image+i), f);
			} else if (BPP == 2) {
				fputc(*(image+i+1), f);
				fputc(*(image+i+0), f);
			} else if (BPP == 3) {
				fputc(*(image+i+2), f);
				fputc(*(image+i+1), f);
				fputc(*(image+i+0), f);
			} else if (BPP == 4) {
				fputc(*(image+i+3), f);
				fputc(*(image+i+2), f);
				fputc(*(image+i+1), f);
				fputc(*(image+i+0), f);
			}
		}
	}
#endif

	/* Write Y and V planes for YUV formats */
	if (BPP == 1) {
		int i;

		/* Write the two chrominance planes */
		for (i=0; i<YDIM/2; i++) {
			fwrite(image+XDIM*YDIM + i*XDIM/2, 1, XDIM/2, f);
			fwrite(image+5*XDIM*YDIM/4 + i*XDIM/2, 1, XDIM/2, f);
		}
	}


	/* Close the file */
	fclose(f);

	return(0);
}

static int write_pnm(char *filename, unsigned char *image)
{
	FILE * f;

	f = fopen(filename, "wb");
	if ( f == NULL) {
		return -1;
	}

	if (BPP == 1) {
		int i;
		fprintf(f, "P5\n%i %i\n255\n", XDIM, YDIM*3/2);

		fwrite(image, 1, XDIM*YDIM, f);

		for (i=0; i<YDIM/2;i++) {
			fwrite(image+XDIM*YDIM + i*XDIM/2, 1, XDIM/2, f);
			fwrite(image+5*XDIM*YDIM/4 + i*XDIM/2, 1, XDIM/2, f);
		}
	} else if (BPP == 3) {
		int i;
		fprintf(f, "P6\n#xvid\n%i %i\n255\n", XDIM, YDIM);
		for (i=0; i<XDIM*YDIM*3; i+=3) {
#ifdef ARCH_IS_LITTLE_ENDIAN
			fputc(image[i+2], f);
			fputc(image[i+1], f);
			fputc(image[i+0], f);
#else
			fputc(image[i+0], f);
			fputc(image[i+1], f);
			fputc(image[i+2], f);
#endif
		}
	}

	fclose(f);

	return 0;
}

static int write_yuv(char *filename, unsigned char *image)
{
	FILE * f;

	f = fopen(filename, "ab+");
	if ( f == NULL) {
		return -1;
	}

	fwrite(image, 1, 3*XDIM*YDIM/2, f);

	fclose(f);

	return 0;
}

/*****************************************************************************
 * Routines for decoding: init decoder, use, and stop decoder
 ****************************************************************************/

/* init decoder before first run */
static int
dec_init(int use_assembler, int debug_level)
{
	int ret;

	xvid_gbl_init_t   xvid_gbl_init;
	xvid_dec_create_t xvid_dec_create;
	xvid_gbl_info_t   xvid_gbl_info;

	/* Reset the structure with zeros */
	memset(&xvid_gbl_init, 0, sizeof(xvid_gbl_init_t));
	memset(&xvid_dec_create, 0, sizeof(xvid_dec_create_t));
	memset(&xvid_gbl_info, 0, sizeof(xvid_gbl_info));

	/*------------------------------------------------------------------------
	 * Xvid core initialization
	 *----------------------------------------------------------------------*/

	xvid_gbl_info.version = XVID_VERSION;
	xvid_global(NULL, XVID_GBL_INFO, &xvid_gbl_info, NULL);

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
	if (!ARG_THREADS) ARG_THREADS = xvid_gbl_info.num_threads;
	fprintf(stderr, " using %d threads.\n", ARG_THREADS);

	/* Version */
	xvid_gbl_init.version = XVID_VERSION;

	/* Assembly setting */
	if(use_assembler)
#ifdef ARCH_IS_IA64
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE | XVID_CPU_IA64;
#else
	xvid_gbl_init.cpu_flags = 0;
#endif
	else
		xvid_gbl_init.cpu_flags = XVID_CPU_FORCE;

	xvid_gbl_init.debug = debug_level;

	xvid_global(NULL, 0, &xvid_gbl_init, NULL);

	/*------------------------------------------------------------------------
	 * Xvid decoder initialization
	 *----------------------------------------------------------------------*/

	/* Version */
	xvid_dec_create.version = XVID_VERSION;

	/*
	 * Image dimensions -- set to 0, xvidcore will resize when ever it is
	 * needed
	 */
	xvid_dec_create.width = 0;
	xvid_dec_create.height = 0;

	xvid_dec_create.num_threads = ARG_THREADS;

	ret = xvid_decore(NULL, XVID_DEC_CREATE, &xvid_dec_create, NULL);

	dec_handle = xvid_dec_create.handle;

	return(ret);
}

/* decode one frame  */
static int
dec_main(unsigned char *istream,
		 unsigned char *ostream,
		 int istream_size,
		 xvid_dec_stats_t *xvid_dec_stats)
{

	int ret;

	xvid_dec_frame_t xvid_dec_frame;

	/* Reset all structures */
	memset(&xvid_dec_frame, 0, sizeof(xvid_dec_frame_t));
	memset(xvid_dec_stats, 0, sizeof(xvid_dec_stats_t));

	/* Set version */
	xvid_dec_frame.version = XVID_VERSION;
	xvid_dec_stats->version = XVID_VERSION;

	/* No general flags to set */
	if (POSTPROC == 1)
		xvid_dec_frame.general          = XVID_DEBLOCKY | XVID_DEBLOCKUV;
	else if (POSTPROC==2)
		xvid_dec_frame.general          = XVID_DEBLOCKY | XVID_DEBLOCKUV | XVID_DERINGY | XVID_DERINGUV;
	else
		xvid_dec_frame.general          = 0;

	/* Input stream */
	xvid_dec_frame.bitstream        = istream;
	xvid_dec_frame.length           = istream_size;

	/* Output frame structure */
	xvid_dec_frame.output.plane[0]  = ostream;
	xvid_dec_frame.output.stride[0] = XDIM*BPP;
	xvid_dec_frame.output.csp = CSP;

	ret = xvid_decore(dec_handle, XVID_DEC_DECODE, &xvid_dec_frame, xvid_dec_stats);

	return(ret);
}

/* close decoder to release resources */
static int
dec_stop()
{
	int ret;

	ret = xvid_decore(dec_handle, XVID_DEC_DESTROY, NULL, NULL);

	return(ret);
}
