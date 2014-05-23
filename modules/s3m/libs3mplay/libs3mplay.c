/* KallistiOS ##version##

   libs3mplay.c
   (C) 2011 Josh Pearson
   
   Based on:
   2ndmix.c
   (c)2000-2002 Dan Potter

*/

#include <kos.h>
#include <stdlib.h>
#include <assert.h>

/**********************************************************/
#include "libs3mplay.h"
#include "s3mplay.h"

volatile unsigned long *snd_dbg = (unsigned long*)0xa080ffc0;
static int s3m_size=0;

/* Load and start an S3M file from disk or memory */
int s3m_play(char *fn) {
	int idx, r;
	FILE * fd;
		
	unsigned char buffer[2048];

	spu_disable();

	printf("LibS3MPlay: Loading %s\r\n", fn);
	fd = fopen( fn, "rb" );
    if (fd == 0)
		return S3M_ERROR_IO;
       	
	fseek( fd, 0, SEEK_END );
	s3m_size = ftell( fd );
	fseek( fd, 0, SEEK_SET );

	if( s3m_size > 1024*512*3 )
	    return S3M_ERROR_MEM;
	
	idx = 0x10000;
	/* Load 2048 bytes at a time */
	while ( (r=fread(buffer, 1, 2048, fd)) > 0) {
		spu_memload(idx, buffer, r);
		idx += r;
	}
	fclose(fd);
	
	printf("LibS3MPlay: Loading ARM program\r\n");
	spu_memload(0, s3mplay, sizeof(s3mplay));

	spu_enable();

	while (*snd_dbg != 3) 
		;

	while (*snd_dbg == 3)
		;
		
	printf("S3M File Now Plays on the AICA processor\r\n");
	
	return S3M_SUCCESS;
}

/* Stop the AICA SPU and reset the memory */
void s3m_stop()
{    
     printf("LibS3MPlay: Stopping\n"); 
     spu_disable();     
     spu_memset(0x10000, 0, s3m_size  );
     spu_memset(0, 0, sizeof(s3mplay) );
}

