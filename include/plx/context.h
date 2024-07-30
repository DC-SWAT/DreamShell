/* Parallax for KallistiOS ##version##

   context.h

   Copyright (C) 2002 Megan Potter


*/

#ifndef __PARALLAX_CONTEXT
#define __PARALLAX_CONTEXT

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
  \file Here we implement a "context" system. This handles things like
  whether face culling is enabled, blending modes, selected texture (or
  lack thereof), etc. This slows your stuff down a bit but adds a lot
  of flexibility, so like everything else in Parallax it's optional.
 */

#include <dc/pvr.h>
#include "texture.h"

/** Initialize the context system */
void plx_cxt_init();

/**
  Select a texture for use with the context system. If you delete the
  texture this has selected and then try to use contexts without
  setting another texture, you'll probably get some gross garbage
  on your output. Specify a NULL texture here to disable texturing.
 */
void plx_cxt_texture(plx_texture_t * txr);

/**
  Set the blending mode to use with the context. What's available is
  platform dependent, but we have defines for DC below.
 */
void plx_cxt_blending(int src, int dst);

/* Constants for blending modes */
#define PLX_BLEND_ZERO		PVR_BLEND_ZERO
#define PLX_BLEND_ONE		PVR_BLEND_ONE
#define PLX_BLEND_DESTCOLOR	PVR_BLEND_DESTCOLOR
#define PLX_BLEND_INVDESTCOLOR	PVR_BLEND_INVDESTCOLOR
#define PLX_BLEND_SRCALPHA	PVR_BLEND_SRCALPHA
#define PLX_BLEND_INVSRCALPHA	PVR_BLEND_INVSRCALPHA
#define PLX_BLEND_DESTALPHA	PVR_BLEND_DESTALPHA
#define PLX_BLEND_INVDESTALPHA	PVR_BLEND_INVDESTALPHA

/**
  Set the culling mode.
 */
void plx_cxt_culling(int type);

/* Constants for culling modes */
#define PLX_CULL_NONE	PVR_CULLING_NONE	/**< Show everything */
#define PLX_CULL_CW	PVR_CULLING_CW		/**< Remove clockwise polys */
#define PLX_CULL_CCW	PVR_CULLING_CCW		/**< Remove counter-clockwise polys */

/**
  Set the fog mode.
 */
void plx_cxt_fog(int type);

/* Constants for fog modes */
#define PLX_FOG_NONE	PVR_FOG_DISABLE
#define PLX_FOG_TABLE	PVR_FOG_TABLE

/**
  Set the specular highlight mode.
 */
void plx_cxt_specular(int type);

/* Constants for specular modes */
#define PLX_SPECULAR_NONE	PVR_SPECULAR_DISABLE
#define PLX_SPECULAR		PVR_SPECULAR_ENABLE

/**
  Submit the selected context for rendering.
 */
void plx_cxt_send(int list);

__END_DECLS

#endif	/* __PARALLAX_TEXTURE */
