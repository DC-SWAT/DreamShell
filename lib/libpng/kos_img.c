/* KallistiOS ##version##

   kos_img.c
   (c)2002 Jeffrey McBeth, Dan Potter

   Based on Jeff's png_load_texture routine, but loads into a
   KOS plat-independent image.
*/
            

#include <kos.h>
#include <assert.h>
#include "pngpriv.h"

/* load an n x n texture from a png */

#define LOAD565(r, g, b) (((r>>3)<<11) | ((g>>2)<<5) | ((b>>3)))
#define LOAD1555(r, g, b, a) (((a>>7)<<15)|((r>>3)<<10)|((g>>3)<<5)|((b>>3)))
#define LOAD4444(r, g, b, a) (((a>>4)<<12)|((r>>4)<<8)|((g>>4)<<4)|((b>>4)))

/* not to be used outside of here */
void _png_copy_texture(uint8 *buffer, uint16 *temp_tex,
                      uint32 channels, uint32 stride,
                      uint32 mask, uint32 w, uint32 h)
{
  uint32 i,j;
  uint16 *ourbuffer;
  uint8 *pRow;
  
  for(i = 0; i < h; i++)
  {
    pRow = &buffer[i*stride];
    ourbuffer = &temp_tex[i*w];
    
    if (channels == 3)
    {
      if (mask == PNG_NO_ALPHA)
        for(j = 0; j < w; j++)
          ourbuffer[j] = LOAD565(pRow[j*3], pRow[j*3+1], pRow[j*3+2]);
      else if (mask == PNG_MASK_ALPHA)
        for(j = 0; j < w; j++)
          ourbuffer[j] = LOAD1555(pRow[j*3],pRow[j*3+1],pRow[j*3+2],255);
      else if (mask == PNG_FULL_ALPHA)
        for(j = 0; j < w; j++)
          ourbuffer[j] = LOAD4444(pRow[j*3],pRow[j*3+1],pRow[j*3+2],255);
          
    }
    else if (channels == 4)
    {
      if (mask == PNG_NO_ALPHA)
      {
        for(j = 0; j < w; j++)
          ourbuffer[j] = LOAD565(pRow[j*4], pRow[j*4+1], pRow[j*4+2]);
      }
      else if (mask == PNG_MASK_ALPHA)
        for(j = 0; j < w; j++)
          ourbuffer[j] = LOAD1555(pRow[j*4],pRow[j*4+1],pRow[j*4+2],pRow[j*4+3]);
      else if (mask == PNG_FULL_ALPHA)
        for(j = 0; j < w; j++)
          ourbuffer[j] = LOAD4444(pRow[j*4],pRow[j*4+1],pRow[j*4+2],pRow[j*4+3]);
    }
  }
}

int png_to_img(const char * filename, uint32 mask, kos_img_t * rv) {
	uint16 *temp_tex;

	/* More stuff */
	uint8	*buffer;	/* Output row buffer */
	uint32	row_stride;	/* physical row width in output buffer */
	uint32	channels;	/* 3 for RGB 4 for RGBA */

	FILE	*infile;	/* source file */

	void	*strs;		/* internal structs */

	assert( rv != NULL );

	if ((infile = fopen(filename, "r")) == 0) {
		dbglog(DBG_ERROR, "png_to_texture: can't open %s\n", filename);
		return -1;
	}

	/* Step 1: Initialize loader */
	strs = readpng_init(infile);
	if (!strs) {
		fclose(infile);
		return -2;
	}

	/* Step 1.5: Create output kos_img_t */
	/* rv = (kos_img_t *)malloc(sizeof(kos_img_t)); */

	/* Step 2: Read file */
	buffer = readpng_get_image(strs,&channels, &row_stride, &rv->w, &rv->h);
	temp_tex = (uint16 *)malloc(sizeof(uint16) * rv->w * rv->h);
	rv->data = (void *)temp_tex;
	rv->byte_count = rv->w * rv->h * 2;

	_png_copy_texture(buffer, temp_tex,
		channels, row_stride,
		mask, rv->w, rv->h);

	switch (mask) {
	case PNG_NO_ALPHA:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGB565, 0);
		break;
	case PNG_MASK_ALPHA:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB1555, 0);
		break;
	case PNG_FULL_ALPHA:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB4444, 0);
		break;
	}

	/* Step 3: Finish decompression */
	free(buffer);
	readpng_cleanup(strs);

	fclose(infile);

	/* And we're done! */
	return 0;
}

