/* Parallax for KallistiOS ##version##

   texture.h

   Copyright (C) 2002 Megan Potter


*/

#ifndef __PARALLAX_TEXTURE
#define __PARALLAX_TEXTURE

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
  \file Higher-level texture management routines. This module goes beyond
  what the hardware routines themselves provide by wrapping the texture
  information into a convienent struct. This struct contains information
  such as the hardware VRAM pointer, width and height, format information,
  etc. We also provide a texture loading facility, and another for creating
  "canvas" textures which can be written to in real-time to produce
  interesting effects.

  WARNING: The internal members of plx_texture_t may change from platform
  to platform below the "ARCH" line. So be wary of using that stuff directly.
 */

#include <dc/pvr.h>

/** Texture structure */
typedef struct plx_texture {
	pvr_ptr_t	ptr;		/**< Pointer to the PVR memory */
	int		w;		/**< Texture width */
	int		h;		/**< Texture height */
	int		fmt;		/**< PVR texture format (e.g., PVR_TXRFMT_ARGB4444) */

	/*** ARCH ***/
	pvr_poly_cxt_t	cxt_opaque,
			cxt_trans,
			cxt_pt;		/**< PVR polygon contexts for each list for this texture */
	pvr_poly_hdr_t	hdr_opaque,
			hdr_trans,
			hdr_pt;		/**< PVR polygon headers for each list for this texture */
} plx_texture_t;

/**
  Load a texture from the VFS and return a plx_texture_t structure. The
  file type will be autodetected (eventually). If you want the texture
  loader to create a texture with an alpha channel, then specify a
  non-zero value for use_alpha. The value for txrload_flags will be
  passed directly to pvr_txr_load_kimg().
 */
plx_texture_t * plx_txr_load(const char * fn, int use_alpha, int txrload_flags);

/**
  Create a texture from raw PVR memory as a "canvas" for the application
  to draw into. Specify all the relevant parameters.
 */
plx_texture_t * plx_txr_canvas(int w, int h, int fmt);

/**
  Destroy a previously created texture.
 */
void plx_txr_destroy(plx_texture_t * txr);

/**
  Edit the texture's header bits to specify the filtering type during
  scaling and perspective operations. All cached values will be updated.
  If you've already called send_hdr, you'll need to call it again.
 */
void plx_txr_setfilter(plx_texture_t * txr, int mode);

/* Constants for plx_txr_setfilter */
#define	PLX_FILTER_NONE		PVR_FILTER_NONE
#define PLX_FILTER_BILINEAR	PVR_FILTER_BILINEAR

/**
  Edit the texture's header bits to specify the UV clamp types. All cached
  values will be updated. If you've already called send_hdr, you'll need
  to call it again.
 */
void plx_txr_setuvclamp(plx_texture_t * txr, int umode, int vmode);

/* Constants for plx_txr_setuvclamp */
#define PLX_UV_REPEAT	0
#define PLX_UV_CLAMP	1

/**
  Re-create the cached pvr_poly_hdr_t's from the struct's pvr_poly_cxt_t's.
  This is helpful if you want to manually tweak the parameters in the user
  friendly context structs.
 */
void plx_txr_flush_hdrs(plx_texture_t * txr);

/**
  Submit the polygon header for the given texture to the TA. If you specify
  a non-zero value for the flush parameter, it will call plx_txr_flush_hdrs()
  for you before sending the values.
 */
void plx_txr_send_hdr(plx_texture_t * txr, int list, int flush);

__END_DECLS

#endif	/* __PARALLAX_TEXTURE */
