/* DreamShell ##version##
   pvr.c
   SWAT
*/
          
#include "ds.h"


/* Open the pvr texture and send it to VRAM */
int pvr_to_img(const char *filename, kos_img_t *rv) {

	/* set up the files and buffers */
	file_t f;
	char header[32];
	uint16 *buffer = NULL;
	int header_len = 0, color = 0, fmt = 0;

	/* open the pvr file */
	f = fs_open(filename, O_RDONLY);

	if(f < 0) {
		ds_printf ("pvr_to_img: Can't open %s\n", filename);
		return -1;
	}

	/* Read the possible 0x00100000 byte header */
	fs_read(f, header, 32);
	
	if(header[0] == 'G' && header[1] == 'B' && header[2] == 'I' && header[3] == 'X') {
		/* GBIX = 0x00100000 byte header */
		header_len = 32;
	} else if (header[0] == 'P' && header[1] == 'V' && header[2] == 'R') {
		/* PVRT = 0x00010000 byte header */
		header_len = 16;
	}

	/* Move past the header */
	fs_seek(f, header_len, SEEK_SET);
	rv->byte_count = fs_total(f) - header_len;

	/* Allocate RAM to contain the PVR file */
	buffer = (uint16*) memalign(32, rv->byte_count);
	if (buffer == NULL) {
		ds_printf("pvr_to_img: Memory error\n");
		return -1;
	}
	rv->data = (void*) buffer;

	/* Read the pvr texture data into RAM and close the file */
	fs_read(f, buffer, rv->byte_count);
	fs_close(f);
	
	rv->byte_count = (rv->byte_count + 31) & ~31;

	/* Get PVR Colorspace */
	color = (unsigned int)header[header_len-8];
	switch(color) {
		  case 0x00: rv->fmt = PVR_TXRFMT_ARGB1555; break; //(bilevel translucent alpha 0,255)
		  case 0x01: rv->fmt = PVR_TXRFMT_RGB565; break; //(non translucent RGB565 )
		  case 0x02: rv->fmt = PVR_TXRFMT_ARGB4444; break; //(translucent alpha 0-255)
		  case 0x03: rv->fmt = PVR_TXRFMT_YUV422; break; //(non translucent UYVY )
		  case 0x04: rv->fmt = PVR_TXRFMT_BUMP; break; //(special bump-mapping format)
		  case 0x05: rv->fmt = PVR_TXRFMT_PAL4BPP; break; //(4-bit palleted texture)
		  case 0x06: rv->fmt = PVR_TXRFMT_PAL8BPP; break; //(8-bit palleted texture)
		  default: break;
	}

	/* Get PVR Format. Mip-Maps and Palleted Textures not Currently handled */
	fmt = (unsigned int)header[header_len-7];
	switch(fmt) {
		  case 0x01: rv->fmt |= PVR_TXRFMT_TWIDDLED; break;//SQUARE TWIDDLED
		  //case 0x02: rv->fmt |= SQUARE TWIDDLED & MIPMAP
		  case 0x03: rv->fmt |= PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_TWIDDLED; break;//VQ TWIDDLED
		  //case 0x04: rv->fmt |= VQ & MIPMAP
		  //case 0X05: rv->fmt |= 8-BIT CLUT TWIDDLED
		  //case 0X06: rv->fmt |= 4-BIT CLUT TWIDDLED
		  //case 0x07: rv->fmt |= 8-BIT DIRECT TWIDDLED
		  //case 0X08: rv->fmt |= 4-BIT DIRECT TWIDDLED
		  case 0x09: rv->fmt |= PVR_TXRFMT_NONTWIDDLED; break;//RECTANGLE
		  case 0x0B: rv->fmt |= PVR_TXRFMT_STRIDE | PVR_TXRFMT_NONTWIDDLED; break;//RECTANGULAR STRIDE
		  case 0x0D: rv->fmt |= PVR_TXRFMT_TWIDDLED; break;//RECTANGULAR TWIDDLED
		  case 0x10: rv->fmt |= PVR_TXRFMT_VQ_ENABLE | PVR_TXRFMT_NONTWIDDLED; break;//SMALL VQ
		  //case 0x11: pvr.txrFmt = SMALL VQ & MIPMAP
		  //case 0x12: pvr.txrFmt = SQUARE TWIDDLED & MIPMAP
		  default: rv->fmt |= PVR_TXRFMT_NONE; break;
	}

	/* Get PVR Texture Width */
	if( (unsigned int)header[header_len-4] == 0x08 && (unsigned int)header[header_len-3] == 0x00 )
		rv->w = 8;
	else if( (unsigned int)header[header_len-4] == 0x10 && (unsigned int)header[header_len-3] == 0x00 )
		rv->w = 16;
	else if( (unsigned int)header[header_len-4] == 0x20 && (unsigned int)header[header_len-3] == 0x00 )
		rv->w = 32;
	else if( (unsigned int)header[header_len-4] == 0x40 && (unsigned int)header[header_len-3] == 0x00 )
		rv->w = 64;
	else if( (unsigned int)header[header_len-4] == -0x80 && (unsigned int)header[header_len-3] == 0x00 )
		rv->w = 128;
	else if( (unsigned int)header[header_len-4] == 0x00 && (unsigned int)header[header_len-3] == 0x01 )
		rv->w = 256;
	else if( (unsigned int)header[header_len-4] == 0x00 && (unsigned int)header[header_len-3] == 0x02 )
		rv->w = 512;
	else if( (unsigned int)header[header_len-4] == 0x00 && (unsigned int)header[header_len-3] == 0x04 )
		rv->w = 1024;
	/* Get PVR Texture Height */
	if( (unsigned int)header[header_len-2] == 0x08 && (unsigned int)header[header_len-1] == 0x00 )
		rv->h = 8;
	else if( (unsigned int)header[header_len-2] == 0x10 && (unsigned int)header[header_len-1] == 0x00 )
		rv->h = 16;
	else if( (unsigned int)header[header_len-2] == 0x20 && (unsigned int)header[header_len-1] == 0x00 )
		rv->h = 32;
	else if( (unsigned int)header[header_len-2] == 0x40 && (unsigned int)header[header_len-1] == 0x00 )
		rv->h = 64;
	else if( (unsigned int)header[header_len-2] == -0x80 && (unsigned int)header[header_len-1] == 0x00 )
		rv->h = 128;
	else if( (unsigned int)header[header_len-2] == 0x00 && (unsigned int)header[header_len-1] == 0x01 )
		rv->h = 256;
	else if( (unsigned int)header[header_len-2] == 0x00 && (unsigned int)header[header_len-1] == 0x02 )
		rv->h = 512;
	else if( (unsigned int)header[header_len-2] == 0x00 && (unsigned int)header[header_len-1] == 0x04 )
		rv->h = 1024;

	return 0;
}




