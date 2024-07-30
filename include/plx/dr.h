/* Parallax for KallistiOS ##version##

   dr.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Falco Girgis


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

static inline void plx_dr_init(plx_dr_state_t *state) {
    pvr_dr_init((pvr_dr_state_t *)state);
}

static inline plx_vertex_t *plx_dr_target(plx_dr_state_t *state) {
    return (plx_vertex_t *)pvr_dr_target(*(pvr_dr_state_t*)state);
}

static inline void plx_dr_commit(plx_vertex_t *vertex) {
    pvr_dr_commit((pvr_vertex_t *)vertex);
}

static inline void plx_dr_finish(void) {
    pvr_dr_finish();
}

static inline int plx_prim(void *data, int size) {
    return pvr_prim(data, size);
}

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
