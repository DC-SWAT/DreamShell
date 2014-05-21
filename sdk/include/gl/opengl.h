/**
	Header for OpenGL module
*/

#ifndef DCE_OPENGL
#define DCE_OPENGL

#include <math.h>
#include <dc/fmath.h>
#include <dc/matrix.h>
#include <dc/matrix3d.h>

#include <dc/pvr.h>
#include <dc/video.h>

#define DEG2RAD (F_PI / 180.0f)
#define RAD2DEG (180.0f / F_PI)

typedef int   vector2i[2];
typedef float vector2f[2];
typedef int   vector3i[3];
typedef float vector3f[3];
typedef float vector4f[4];
typedef float matrix4f[4][4];
typedef unsigned int   DWORD;
typedef long           LONG;
typedef short          BYTE;
typedef unsigned short WORD;

/* Primitive Types taken from GL for compatability */
#define GL_POINTS		    0x01
#define GL_LINES		    0x02
#define GL_LINE_LOOP		0x03
#define GL_LINE_STRIP		0x04
#define GL_TRIANGLES		0x05
#define GL_TRIANGLE_STRIP	0x06
#define GL_TRIANGLE_FAN		0x07 
#define GL_QUADS		    0x08
#define GL_QUAD_STRIP		0x09
#define GL_POLYGON		    0x0A

/* Matrix modes */
#define GL_MATRIX_MODE		  0x0BA0
#define GL_SCREENVIEW		  0x00
#define GL_MODELVIEW		  0x01
#define GL_PROJECTION		  0x02
#define GL_TEXTURE		      0x03
#define GL_IDENTITY           0x04
#define GL_RENDER             0x05
#define GL_MATRIX_COUNT		  0x04

/* "Depth buffer" -- we don't actually support a depth buffer because
   the PVR does all of that internally. But these constants are to
   ease porting. */
#define GL_NEVER		0x0200
#define GL_LESS			0x0201
#define GL_EQUAL		0x0202
#define GL_LEQUAL		0x0203
#define GL_GREATER		0x0204
#define GL_NOTEQUAL		0x0205
#define GL_GEQUAL		0x0206
#define GL_ALWAYS		0x0207

#define GL_DEPTH_TEST		0
#define GL_DEPTH_BITS		0
#define GL_DEPTH_CLEAR_VALUE	0
#define GL_DEPTH_FUNC		0
#define GL_DEPTH_RANGE		0
#define GL_DEPTH_WRITEMASK	0
#define GL_DEPTH_COMPONENT	0

/* Blending: not sure how we'll use these yet; the PVR supports a few
   of these so we'll want to eventually */
#define GL_BLEND				0x0BE2 /* capability bit */
#define GL_BLEND_SRC		2
#define GL_BLEND_DST		3

#define GL_ZERO			0
#define GL_ONE			1
#define GL_SRC_COLOR		0x0300
#define GL_ONE_MINUS_SRC_COLOR	0x0301
#define GL_SRC_ALPHA		0x0302
#define GL_ONE_MINUS_SRC_ALPHA	0x0303
#define GL_DST_ALPHA		0x0304
#define GL_ONE_MINUS_DST_ALPHA	0x0305
#define GL_DST_COLOR		0x0306
#define GL_ONE_MINUS_DST_COLOR	0x0307
/*#define GL_SRC_ALPHA_SATURATE	0x0308 unsupported */

/* Misc texture constants */
#define GL_TEXTURE_2D		0x0001		/* capability bit */
#define GL_KOS_AUTO_UV		0x8000		/* capability bit */
#define GL_TEXTURE_WRAP_S	0x2802
#define GL_TEXTURE_WRAP_T	0x2803
#define GL_TEXTURE_MAG_FILTER			0x2800
#define GL_TEXTURE_MIN_FILTER			0x2801
#define GL_TEXTURE_FILTER	GL_TEXTURE_MIN_FILTER
#define GL_FILTER_NONE		0
#define GL_FILTER_BILINEAR	1
#define GL_REPEAT		0x2901
#define GL_CLAMP		0x2900

/* Texture Environment */
#define GL_TEXTURE_ENV_MODE	0x2200
#define GL_REPLACE		0
#define GL_MODULATE		1
#define GL_DECAL		2
#define GL_MODULATEALPHA	3

/* TextureMagFilter */
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601

/* Texture mapping */
#define GL_TEXTURE_ENV				0x2300

#define GL_TEXTURE_ENV_COLOR			0x2201

#define GL_NEAREST_MIPMAP_NEAREST		0x2700
#define GL_NEAREST_MIPMAP_LINEAR		0x2702
#define GL_LINEAR_MIPMAP_NEAREST		0x2701
#define GL_LINEAR_MIPMAP_LINEAR			0x2703

/* TextureUnit */
#define GL_TEXTURE0                       0x84C0
#define GL_TEXTURE1                       0x84C1
#define GL_TEXTURE2                       0x84C2
#define GL_TEXTURE3                       0x84C3
#define GL_TEXTURE4                       0x84C4
#define GL_TEXTURE5                       0x84C5
#define GL_TEXTURE6                       0x84C6
#define GL_TEXTURE7                       0x84C7
#define GL_TEXTURE8                       0x84C8
#define GL_TEXTURE9                       0x84C9
#define GL_TEXTURE10                      0x84CA
#define GL_TEXTURE11                      0x84CB
#define GL_TEXTURE12                      0x84CC
#define GL_TEXTURE13                      0x84CD
#define GL_TEXTURE14                      0x84CE
#define GL_TEXTURE15                      0x84CF
#define GL_TEXTURE16                      0x84D0
#define GL_TEXTURE17                      0x84D1
#define GL_TEXTURE18                      0x84D2
#define GL_TEXTURE19                      0x84D3
#define GL_TEXTURE20                      0x84D4
#define GL_TEXTURE21                      0x84D5
#define GL_TEXTURE22                      0x84D6
#define GL_TEXTURE23                      0x84D7
#define GL_TEXTURE24                      0x84D8
#define GL_TEXTURE25                      0x84D9
#define GL_TEXTURE26                      0x84DA
#define GL_TEXTURE27                      0x84DB
#define GL_TEXTURE28                      0x84DC
#define GL_TEXTURE29                      0x84DD
#define GL_TEXTURE30                      0x84DE
#define GL_TEXTURE31                      0x84DF

/* Misc bitfield things; we don't really use these either */
#define GL_COLOR_BUFFER_BIT	0
#define GL_DEPTH_BUFFER_BIT	0

/* Lighting constants */
#define GL_LIGHTING		0x0b50
#define GL_LIGHT0		0x0010		/* capability bit */
#define GL_LIGHT1		0x0000
#define GL_LIGHT2		0x0000
#define GL_LIGHT3		0x0000
#define GL_LIGHT4		0x0000
#define GL_LIGHT5		0x0000
#define GL_LIGHT6		0x0000
#define GL_LIGHT7		0x0000
#define GL_AMBIENT		0x1200
#define GL_DIFFUSE		0x1201
#define GL_SPECULAR		0
#define GL_SHININESS		0
#define GL_EMISSION		0
#define GL_POSITION		0x1203
#define GL_SHADE_MODEL		0x0b54
#define GL_FLAT			0x1d00
#define GL_SMOOTH		0x1d01

/* KOS near Z-CLIPPING */
#define GL_KOS_NEARZ_CLIPPING		0x0020		/* capability bit */

/* DCE-GL *********************************************************************/

#define GL_UNSIGNED_SHORT_5_6_5       PVR_TXRFMT_RGB565
#define GL_UNSIGNED_SHORT_5_6_5_REV   PVR_TXRFMT_RGB565
#define GL_UNSIGNED_SHORT_1_5_5_5     PVR_TXRFMT_ARGB1555
#define GL_UNSIGNED_SHORT_1_5_5_5_REV PVR_TXRFMT_ARGB1555
#define GL_UNSIGNED_SHORT_4_4_4_4     PVR_TXRFMT_ARGB4444
#define GL_UNSIGNED_SHORT_4_4_4_4_REV PVR_TXRFMT_ARGB4444

#define GL_RED  0x00
#define GL_RG   0x01
#define GL_RGB  0x02
#define GL_BGR  0x03
#define GL_RGBA 0x04
#define GL_BGRA 0x05

#define GLint    int
#define GLfloat  float
#define GLdouble float
#define GLvoid   void
#define GLuint   unsigned int
#define GLenum   unsigned int
#define GLsizei  unsigned int
#define GLfixed  const unsigned int
#define GLclampf float
#define GLubyte  unsigned short
#define GLboolean   int
#define GL_FALSE	0
#define GL_TRUE		1

#define GL_DOUBLE 0xa0
#define GL_FLOAT  0xa0
#define GL_UNSIGNED_INT 0xa1

#define GL_RGB565_TWID PVR_TXRFMT_RGB565 | PVR_TXRFMT_TWIDDLED
#define GL_ARGB4444_TWID PVR_TXRFMT_ARGB4444 | PVR_TXRFMT_TWIDDLED

void glColor1ui( uint32 c );
void glColor3f( float r, float g, float b );
void glColor3fv( float * rgb );
void glColor4fv( float * rgba );
void glColor4f( float r, float g, float b,float a );
void glColor4ub( GLubyte a, GLubyte r, GLubyte g, GLubyte b );

void glTexCoord2f( float u, float v );
void glTexCoord2fv( float *uv );

void glVertex2f( float x, float y );
void glVertex2fv( float *xy );
void glVertex3f( float x, float y, float z );
void glVertex3fv( float *xyz );

void glNormal3f( float x, float y, float z );

void glGenTextures( GLsizei n, GLuint * textures ); 

void glKosTex2D( GLint internal_fmt, GLsizei width, GLsizei height,
                 pvr_ptr_t txr_address );

void glTexImage2D( GLenum target, GLint level, GLint internalFormat,
                     GLsizei width, GLsizei height, GLint border,
                     GLenum format, GLenum type, GLvoid * data );

void glBindTexture( GLenum  target, GLuint texture );

void glTexParameterf( GLenum target, GLenum pname, GLfloat param ); 
void glTexParameteri( GLenum target, GLenum pname, GLint param ); 

void glTexEnvf( GLenum target, GLenum pname, GLfloat param ); 
void glTexEnvi( GLenum target, GLenum pname, GLint param ); 

void glBlendFunc( GLenum sfactor, GLenum dfactor ); 

void glEnable( int mode );
void glDisable( int mode );

void glEnd();
void glBegin( int mode );

void glShadeModel( GLenum   mode); 

void glDepthMask(GLboolean flag);

void glDepthFunc(GLenum func);

void glClearDepthf(GLfloat depth);
#define glClearDepth glClearDepthf

void glClearColor( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );

void glClear( int mode );

void glKosBeginFrame();
void glKosFinishFrame();

void glKosInit();

/* Transformation / Matrix Functions */

void glLoadIdentity();
void glMatrixMode( GLenum   mode );

void glPushMatrix();
void glPopMatrix();

void glTranslatef( GLfloat x, GLfloat y, GLfloat z );
#define glTranslated glTranslatef

void glScalef( GLfloat x, GLfloat y, GLfloat z );
#define glScaled glScalef

void glRotatef( GLfloat angle, GLfloat x, GLfloat  y, GLfloat z ); 
#define glRotated glRotatef

void glOrtho(GLfloat left, GLfloat right,
             GLfloat bottom, GLfloat top,
             GLfloat znear, GLfloat zfar);

void gluPerspective( GLfloat angle, GLfloat aspect,
                     GLfloat znear, GLfloat zfar );

void gluLookAt(GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx,
          GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy,
          GLfloat upz);

void glDepthRange( GLclampf n, GLclampf f );
void glViewport( GLint x, GLint y, GLsizei width, GLsizei height );
void glFrustum( GLfloat left, GLfloat right,
                GLfloat bottom, GLfloat top,
                GLfloat znear, GLfloat zfar );

void glKosFinishList();

/* GL Array API */
void glVertexPointer( GLint size, GLenum type,
                      GLsizei stride, const GLvoid * pointer ); 
                      
void glTexCoordPointer( GLint size, GLenum type,
                        GLsizei stride, const GLvoid * pointer ); 

void glColorPointer( GLint size, GLenum type,
                     GLsizei stride, const GLvoid * pointer ); 

void glDrawArrays( GLenum mode, GLint first, GLsizei count ); 


#endif

