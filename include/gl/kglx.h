// (c) by Heinrich Tillack 2002 (http://a128.ch9.de)
// under GPL or new BSD license

#ifndef KALL_H
#define KALL_H

#include <kos.h>
#include <GL/gl.h>

typedef struct {
    uint32 flags;
    float x, y, z;
    float u, v;
    uint32 argb, oargb;
} gl_vertex_t;

extern GLuint vert_rgba;
extern GLfloat vert_u,vert_v;//KGL-X gldraw.c
extern gl_vertex_t *vtxbuf;
/* Post-xformed vertex buffer */
extern int vbuf_top,vbuf_size;

#define INLINE __inline

INLINE void kglVertex3f(float xx,float yy,float zz){
 //   vtxbuf[vbuf_top].flags = PVR_CMD_VERTEX;

    vtxbuf[vbuf_top].x = xx;
    vtxbuf[vbuf_top].y =  yy;
    vtxbuf[vbuf_top].z = zz;
  //  vtxbuf[vbuf_top].w =  1;

    vtxbuf[vbuf_top].u = vert_u;
    vtxbuf[vbuf_top].v = vert_v;


    vtxbuf[vbuf_top].argb = vert_rgba;
    vtxbuf[vbuf_top++].oargb = 0xff000000;

    //     //Normals
    //     vtxbuf[vbuf_top].nx = nx;
    //     vtxbuf[vbuf_top].ny = ny;
    //     vtxbuf[vbuf_top].nz = nz;

}


INLINE void  kglTexCoord2f(GLfloat u, GLfloat v) {
    vert_u = u;
    vert_v = v;
}

/* glopColor */

#define	SET_COLOR(red,green,blue,alpha)	\
	vert_rgba = ((alpha) << 24) | ((red) << 16) | ((green) << 8) | (blue);


INLINE void kglColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {


    SET_COLOR((int)(r*0xff),(int)(g*0xff),(int)(b*0xff),(int)(a*0xff))


}



INLINE void kglColor3f(GLfloat red, GLfloat green, GLfloat blue) {

    kglColor4f(red,green,blue,1.0f);

}

#endif

