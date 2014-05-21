/* Parallax for KallistiOS ##version##

   dr.h

   (c)2002 Dan Potter


*/

#ifndef __PARALLAX_DR
#define __PARALLAX_DR

#include <sys/cdefs.h>
__BEGIN_DECLS

/**
  \file Direct render stuff. This, like matrix.h, is just here to try to
  keep client code as platform independent as possible for porting. This
  will pretty much all get optimized out by the compiler.
 */

#include <dc/pvr.h>

typedef pvr_dr_state_t plx_dr_state_t;
typedef pvr_vertex_t plx_vertex_t;

#define PLX_VERT	PVR_CMD_VERTEX
#define PLX_VERT_EOS	PVR_CMD_VERTEX_EOL

#define plx_dr_init(a) pvr_dr_init(*a)
#define plx_dr_target(a) pvr_dr_target(*a)
#define plx_dr_commit(a) pvr_dr_commit(a)

#define plx_prim pvr_prim

static inline void plx_scene_begin() {
	pvr_wait_ready();
	pvr_scene_begin();
}

static inline void plx_list_begin(int type) {
	pvr_list_begin(type);
}

static inline void plx_scene_end() {
	pvr_scene_finish();
}

__END_DECLS

#endif	/* __PARALLAX_DR */

