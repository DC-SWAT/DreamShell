#include <kos.h>
#include "pngpriv.h"

/* load an n x n texture from a png */

/* not to be used outside of here */
extern void _png_copy_texture(uint8 *buffer, uint16 *temp_tex,
                      uint32 channels, uint32 stride,
                      uint32 mask, uint32 w, uint32 h);

int png_load_texture(const char * filename, pvr_ptr_t *tex, uint32 mask, uint32 *w, uint32 *h)
{
    uint16 *temp_tex;

    /* More stuff */
    uint8 *buffer;            /* Output row buffer */
    uint32 row_stride;   /* physical row width in output buffer */
    uint32 channels;           /* 3 for RGB 4 for RGBA */

    FILE *infile;                /* source file */

    void * strs;		/* internal structs */

    //    readpng_version_info();
    
    if ((infile = fopen(filename, "r")) == 0) {
      printf("png_to_texture: can't open %s\n", filename);
      return -1;
    }

    /* Step 1: Initialize loader */

    strs = readpng_init(infile);
    if (!strs)
    {
      fclose(infile);
      return -1;
    }

    /* Step 2: Read file */

    buffer = readpng_get_image(strs, &channels, &row_stride, w, h);
    temp_tex = (uint16 *)malloc(sizeof(uint16)*(*w)*(*h));

    _png_copy_texture(buffer, temp_tex,
                     channels, row_stride,
                     mask, *w, *h);

    *tex = pvr_mem_malloc((*w)*(*h)*2);
    pvr_txr_load_ex(temp_tex, *tex, *w, *h, PVR_TXRLOAD_16BPP);

    /* Step 3: Finish decompression */
    free(buffer);
    readpng_cleanup(strs);

    fclose(infile);
    free(temp_tex);
    /* And we're done! */
    return 0;

}

int png_to_texture(const char * filename, pvr_ptr_t tex, uint32 mask)
{
  uint16 *temp_tex;
  
  /* More stuff */
  uint8 *buffer;            /* Output row buffer */
  uint32 row_stride;   /* physical row width in output buffer */
  uint32 channels;           /* 3 for RGB 4 for RGBA */
  uint32 w,h;
  
  FILE *infile;                /* source file */

  void * strs;		/* internal structs */

  //    readpng_version_info();
  
  if ((infile = fopen(filename, "r")) == 0) {
    printf("png_to_texture: can't open %s\n", filename);
    return -1;
  }

  /* Step 1: Initialize loader */
  strs = readpng_init(infile);
  if (!strs)
  {
    fclose(infile);
    return -1;
  }

  /* Step 2: Read file */

  buffer = readpng_get_image(strs, &channels, &row_stride,&w,&h);
  temp_tex = (uint16 *)malloc(sizeof(uint16)*w*h);

  _png_copy_texture(buffer, temp_tex,
                   channels, row_stride,
                   mask, w, h);

  pvr_txr_load_ex(temp_tex, tex, w, h, PVR_TXRLOAD_16BPP);

  /* Step 3: Finish decompression */
  free(buffer);
  readpng_cleanup(strs);

  fclose(infile);

  free(temp_tex);
  /* And we're done! */

  return 0;
}
