#ifndef __GL_XLU_H
#define __GL_GLU_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <gl/gl.h>

#define GLU_FALSE	0
#define GLU_TRUE	1

void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear,
	GLfloat zFar);

void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx,
          GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy,
          GLfloat upz);


GLint gluBuild2DMipmaps( GLenum target, GLint internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *data );
/*+HT*/


void
gluOrtho2D(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top);

void
gluPickMatrix(GLfloat x, GLfloat y,
	      GLfloat width, GLfloat height, GLint viewport[4]);



GLint
gluProject(GLfloat objx, GLfloat objy, GLfloat objz,
	      const GLfloat modelMatrix[16],
	      const GLfloat projMatrix[16],
           const GLint viewport[4],
	      GLfloat *winx, GLfloat *winy, GLfloat *winz);


/*-HT*/

__END_DECLS

#endif	/* __GL_GLU_H */

