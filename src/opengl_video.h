/*
 * (c) 2015 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __OPENGL_VIDEO_H__
#define __OPENGL_VIDEO_H__

#define MAX_OPENGL_SHAPES	32
#define MAX_OPENGL_SHAPE_PLACES	32
#define MAX_OPENGL_TEXTURES	16
#define MAX_OPENGL_VECTORS	16
#define MAX_OPENGL_VBOS		64
#define OVERLAY_GLSHAPE_LEVELS	10

#define WITH_OSMESA 1
#define WITH_GLX 1

#ifdef WITH_GLX
#include <GL/glx.h>
#endif

//#include <math.h>
#ifdef WITH_OSMESA
# include "GL/osmesa.h"
# ifdef HAVE_MACOSX
#  include <OpenGL/OpenGL.h>
#  ifdef HAVE_GLU
#    include <OpenGL/glu.h>
#  endif
#  ifdef HAVE_GLUT
#   include <GLUT/glut.h>
#  endif
# else
#  ifdef HAVE_GLU
#    include <GL/glu.h>
#  endif
#  ifdef HAVE_GLUT
#   include <GL/glut.h>
#  endif
# endif
#endif


//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
//#ifdef WIN32
//#include "windows_util.h"
//#else
//#include <sys/time.h>
//#endif

//#include "snowmix.h"
//#include "monitor.h"
//#include "audio_feed.h"
//#include "audio_mixer.h"
//#include "audio_sink.h"
//#include "video_feed.h"
//#include "video_text.h"
//#include "video_scaler.h"
#include "controller.h"
//#include "video_image.h"
//#include "video_shape.h"
//#include "virtual_feed.h"
//#include "tcl_interface.h"
//#include "cairo_graphic.h"
//#include "video_output.h"

//#include "video_mixer.h"


/*
 * glEnable()	  : GL_ALPHA_TEST, GL_AUTO_NORMAL, GL_BLEND, GL_CLIP_PLANEi,
                    GL_COLOR_LOGIC_OP, GL_COLOR_MATERIAL, GL_COLOR_SUM, GL_COLOR_TABLE,
                    GL_CONVOLUTION_1D, GL_CONVOLUTION_2D, GL_CULL_FACE, GL_DEPTH_TEST,
                    GL_DITHER, GL_FOG, GL_HISTOGRAM, GL_INDEX_LOGIC_OP, GL_LIGHTi,
                    GL_LIGHTING, GL_LINE_SMOOTH, GL_LINE_STIPPLE, GL_MAP1_COLOR_4,
                    GL_MAP1_INDEX, GL_MAP1_NORMAL, GL_MAP1_TEXTURE_COORD_1,
		    GL_MAP1_TEXTURE_COORD_2, GL_MAP1_TEXTURE_COORD_3, GL_MAP1_TEXTURE_COORD_4,
                    GL_MAP1_VERTEX_3, GL_MAP1_VERTEX_4, GL_MAP2_COLOR_4, GL_MAP2_INDEX,
                    GL_MAP2_NORMAL, GL_MAP2_TEXTURE_COORD_1, GL_MAP2_TEXTURE_COORD_2,
                    GL_MAP2_TEXTURE_COORD_3, GL_MAP2_TEXTURE_COORD_4, GL_MAP2_VERTEX_3,
                    GL_MAP2_VERTEX_4, GL_MINMAX, GL_MULTISAMPLE,
                    GL_NORMALIZE, GL_POINT_SMOOTH, GL_POINT_SPRITE, GL_POLYGON_OFFSET_FILL,
                    GL_POLYGON_OFFSET_LINE, GL_POLYGON_OFFSET_POINT, GL_POLYGON_SMOOTH,
                    GL_POLYGON_STIPPLE, GL_POST_COLOR_MATRIX_COLOR_TABLE,
		    GL_POST_CONVOLUTION_COLOR_TABLE, GL_RESCALE_NORMAL,
                    GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_ALPHA_TO_ONE, GL_SAMPLE_COVERAGE,
                    GL_SEPARABLE_2D, GL_SCISSOR_TEST, GL_STENCIL_TEST, GL_TEXTURE_1D,
                    GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_GEN_Q,
                    GL_TEXTURE_GEN_R, GL_TEXTURE_GEN_S, GL_TEXTURE_GEN_T,
                    GL_VERTEX_PROGRAM_POINT_SIZE, GL_VERTEX_PROGRAM_TWO_SIDE,

 * glDisable()		: As for glEnable()
 * glClear()		: GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT 
 * glMatrixMode()	: GL_MODELVIEW, GL_PROJECTION
 * glTexParameteri()	: GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER/GL_TEXTURE_MAG_FILTER/
			  GL_TEXTURE_WRAP_S/GL_TEXTURE_WRAP_T, GL_NEAREST/GL_LINEAR/
			  GL_NEAREST_MIPMAP_NEAREST/GL_LINEAR_MIPMAP_NEAREST/
			  GL_NEAREST_MIPMAP_LINEAR/GL_LINEAR_MIPMAP_LINEAR
 * glBegin()		: GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP,
			  GL_TRIANGLES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUADS,
			  GL_QUAD_STRIP, GL_POLYGON
 * gluQuadricNormals	: GLU_NONE, GLU_FLAT, and GLU_SMOOTH
 *
 *
 */
enum opengl_operation_enum_t {
	SM_GL_ARCXZ,			//  Arc in XZ, rectangle mapped onto a partial cylinder
	SM_GL_ARCYZ,			//  Arc in YZ, rectangle mapped onto a partial cylinder
	SM_GL_ARCXY,			//  Arc in XY, rectangle mapped onto a partial cylinder
	SM_GL_BEGIN,			//- glBegin(mode)
	SM_GL_BLENDFUNC,		//- glBlendFunc(GLenum s,GLenum d)
	SM_GL_BINDTEXTURE,		//- glBindTexture(GLenum  target,  GLuint  texture)
	SM_GL_CLEAR,			//- glClear(mode)
	SM_GL_CLEARCOLOR,		//- glClearColor(red, green, blue, alpha)
	SM_GL_COLOR,			//- call to glColorX
	SM_GL_COLOR3,			//- glColor3f(red,green,blue)
	SM_GL_COLOR4,			//- glColor4f(red,green,blue,alpha)
	SM_GL_COLORMATERIAL,		//  glColorMaterial(GLenum face, GLenum mode)
	SM_GL_DISABLE,			//- glDisable(mode)
	SM_GL_ENABLE,			//- glEnable(mode)
	SM_GL_END,			//- glEnd(void)
	SM_GL_ENTRY,			//- Set entry active/inactive in a shape
	SM_GL_FINISH,			//- glFinish(void)
	SM_GL_FLUSH,			//- glFlush(void)
	SM_GL_GLSHAPE,			//- link to other glshape
	SM_GL_IFGLX,			//- conditional glx
	SM_GL_IFOSMESA,			//- conditional osmesa
	SM_GL_LIGHT,			//  glLightf(GLenum  light,  GLenum  pname,  GLfloat  param);
					//  glLighti(GLenum  light,  GLenum  pname,  GLint  param);
	SM_GL_LIGHTV,			//- glLightfv(GLenum  light,  GLenum  pname,  const GLfloat *  params)
					//- glLightiv(GLenum  light,  GLenum  pname,  const GLint *  params);

	SM_GL_MATERIALV,		//- glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
	SM_GL_LINEWIDTH,		//  glLineWidth(GLfloat  width);
	SM_GL_LINESTIPPLE,		//  glLineStipple(GLint factor, GLushort pattern)
	SM_GL_LOADIDENTITY,		//- glLoadIdentity(void)
	SM_GL_MATRIXMODE,		//- glMatrixMode(mode)
	SM_GL_MODIFY,			//- For modifying a value in an entry in a shape
	SM_GL_NORMAL,			//- glNormal3f(x,y,z)
	SM_GL_ORTHO,			//- glOrthof(double left, double right, double, bottom, double top, double near, double far)
	SM_GL_POPMATRIX,		//- glPopMatrix(void)
	SM_GL_PUSHMATRIX,		//- glPushMatrix(void)
	SM_GL_ROTATE,			//- glRotatef(angle,x,y,z)
	SM_GL_RECURSION,		//- Set level of allowed recursion
	SM_GL_SCALE,			//- glScalef(x,y,z)
	SM_GL_SCISSOR,			//- glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
	SM_GL_SNOWMIX,			//- call the Snowmix parser
	SM_GL_TEXCOORD,			//- call to glTexCoordX
	SM_GL_TEXCOORD1,		//- glTexCoord1f(s)
	SM_GL_TEXCOORD2,		//- glTexCoord2f(s,t)
	SM_GL_TEXCOORD3,		//- glTexCoord3f(s,t,r)
	SM_GL_TEXCOORD4,		//- glTexCoord4f(s,t,r,q)
	SM_GL_TEXFILTER2D,		//- glTexParameteri on MIN_FILTER and MAG_FILTER
	SM_GL_TRANSLATE,		//- glTranslatef(x,y,z)
	SM_GL_VBO,			//- call Vertex Buffer Object
	SM_GL_VERTEX,			//- call to glVertexX
	//SM_GL_VERTEX1,			//- glVertex1f(x)
	SM_GL_VERTEX2,			//- glVertex2f(x,y)
	SM_GL_VERTEX3,			//- glVertex3f(x,y,z)
	SM_GL_VERTEX4,			//- glVertex4f(x,y,z,w)

	SM_GL_VIEWPORT,			//- glViewport(GLint x,GLint y, GLsizei width,
					//	GLsizei height)
	SM_GL_SHADEMODEL,		//- glShadeModel(GLenum mode);
	SM_GL_TEXPARAMETERI,		// glTexParameteri(GLenum target, GLenum pname,
					//	GLint param)
	SM_GL_TEXPARAMETERF,		// glTexParameterf(GLenum target, GLenum pname,
					//	GLfloat param)
	SM_GL_NOOP,			//- No Operation
#  ifdef HAVE_GLUT
	SM_GLUT_TEAPOT,			//  glutSolidTeapot/glutWireTeapot(double size)
#endif

	SM_GLU_PERSPECTIVE,		//- gluPerspective(GLdouble  fovy,  GLdouble  aspect,
					//	 GLdouble  zNear,  GLdouble  zFar)
	SM_GLU_NORMALS,			//-  gluQuadricNormals(q, GLU_SMOOTH);
	SM_GLU_TEXTURE,			//-  gluQuadricTexture(q,  GLboolean  texture);
	SM_GLU_ORIENTATION,		//-  gluQuadricOrientation(q, GLenum  orientation);
	SM_GLU_DRAWSTYLE,		//-  gluQuadricDrawStyle(q,  GLenum  draw)
	SM_GLU_SPHERE,			//-  gluSphere(q, radius, slices, stacks);
	SM_GLU_CYLINDER,		//-  gluCylinder(q,GLdouble base, GLdouble top,
					//	GLdouble height, GLint slices, GLint stacks);;
	SM_GLU_DISK,			//  gluDisk(quad, GLdouble inner, GLdouble outer,
					//	GLint slices, GLint loops);
	SM_GLU_PARTIALDISK		//  gluPartialDisk(q,GLdouble inner,GLdouble outer,
					//	 GLint slices, GLint loops, GLdouble start,
					//	 GLdouble sweep)

};

//#define	SM_OPENGL_PARM_TYPE_INT
#define	SM_OPENGL_PARM_TYPE_FLOAT
//#define	SM_OPENGL_PARM_TYPE_DOUBLE


#ifdef SM_OPENGL_PARM_TYPE_INT // All integer GL functions except scale and rotate
  typedef	int sm_opengl_parm_t;
  #define SM_OPENGL_PARM_GL_TYPE	GL_INT
  #define SM_OPENGL_PARM		"%d"
  #define SM_OPENGL_PARM_PRINT		"%d"
  #define SM_OPENGL_PARM_ROTATE_SCALE	"%f"
  #define SM_OPENGL_PARM_RS_PRINT	"%.4f"
  #define SM_GL_OPER_SCALE		glScalef
  #define SM_GL_OPER_ROTATE		glRotatef
  #define SM_GL_OPER_COLOR3		glColor3i
  #define SM_GL_OPER_COLOR4		glColor4i
  #define SM_GL_OPER_GETV		glGetIntegerv
  #define SM_GL_OPER_LIGHT		glLighti
  #define SM_GL_OPER_LIGHTV		glLightiv
  #define SM_GL_OPER_NORMAL		glNormal3i
  #define SM_GL_OPER_TEXCOORD1		glTexCoord1i
  #define SM_GL_OPER_TEXCOORD2		glTexCoord2i
  #define SM_GL_OPER_TEXCOORD3		glTexCoord3i
  #define SM_GL_OPER_TEXCOORD4		glTexCoord4i
  #define SM_GL_OPER_TRANSLATE		glTranslatef
  #define SM_GL_OPER_VERTEX2		glVertex2i
  #define SM_GL_OPER_VERTEX3		glVertex3i
  #define SM_GL_OPER_VERTEX4		glVertex4i
#else
  #ifdef SM_OPENGL_PARM_TYPE_FLOAT	// Use float GL functions
    typedef	float sm_opengl_parm_t;
    #define SM_OPENGL_PARM_GL_TYPE	GL_FLOAT
    #define SM_OPENGL_PARM		"%f"
    #define SM_OPENGL_PARM_PRINT	"%.4f"
    #define SM_GL_OPER_SCALE		glScalef
    #define SM_GL_OPER_ROTATE		glRotatef
    #define SM_GL_OPER_COLOR3		glColor3f
    #define SM_GL_OPER_COLOR4		glColor4f
    #define SM_GL_OPER_GETV		glGetFloatv
    #define SM_GL_OPER_LIGHT		glLightf
    #define SM_GL_OPER_LIGHTV		glLightfv
    #define SM_GL_OPER_NORMAL		glNormal3f
    #define SM_GL_OPER_TEXCOORD1	glTexCoord1f
    #define SM_GL_OPER_TEXCOORD2	glTexCoord2f
    #define SM_GL_OPER_TEXCOORD3	glTexCoord3f
    #define SM_GL_OPER_TEXCOORD4	glTexCoord4f
    #define SM_GL_OPER_TRANSLATE	glTranslatef
    #define SM_GL_OPER_VERTEX2		glVertex2f
    #define SM_GL_OPER_VERTEX3		glVertex3f
    #define SM_GL_OPER_VERTEX4		glVertex4f
  #else
    typedef	double sm_opengl_parm_t;	// use double GL functions
    #define SM_OPENGL_PARM_GL_TYPE	GL_DOUBLE
    #define SM_OPENGL_PARM		"%lf"
    #define SM_OPENGL_PARM_PRINT	"%.4lf"
    #define SM_GL_OPER_SCALE		glScaled
    #define SM_GL_OPER_ROTATE		glRotated
    #define SM_GL_OPER_COLOR3		glColor3d
    #define SM_GL_OPER_COLOR4		glColor4d
    #define SM_GL_OPER_GETV		glGetDoublev
    #define SM_GL_OPER_LIGHT		glLightf
    #define SM_GL_OPER_LIGHTV		glLightfv
    #define SM_GL_OPER_NORMAL		glNormal3d
    #define SM_GL_OPER_TEXCOORD1	glTexCoord1d
    #define SM_GL_OPER_TEXCOORD2	glTexCoord2d
    #define SM_GL_OPER_TEXCOORD3	glTexCoord3d
    #define SM_GL_OPER_TEXCOORD4	glTexCoord4d
    #define SM_GL_OPER_TRANSLATE	glTranslated
    #define SM_GL_OPER_VERTEX2		glVertex2d
    #define SM_GL_OPER_VERTEX3		glVertex3d
    #define SM_GL_OPER_VERTEX4		glVertex4d
  #endif
#endif

enum texture_source_enum_t {
	TEXTURE_SOURCE_NONE,
	TEXTURE_SOURCE_FEED,
	TEXTURE_SOURCE_IMAGE,
	TEXTURE_SOURCE_FRAME
};

// Vertex Buffer Object
// glshabe vbo add <vbo id> <name>\n"
// glshabe vbo config <vbo id> [<entry count> (quads|triangles) (n3 | t2 | t3 | v3 | c3 | c4) ...]\n"
// glshabe vbo data <vbo id> [<data 0> <data 1> ...]\n"
// glshabe vbo indices <vbo id> [<index 0> <index 1> ...]\n"
// glshabe vbo <vbo id> (static | dynamic | stream)
struct opengl_vbo_t {
	char*			name;		// Name of the VBO
	u_int32_t		id;		// ID in the array
	char*			config;		// t2 v3..
	GLenum			type;		// GL_TRIANGLES, GL_QUADS
	GLenum			usage;		// GL_STATIC_DRAW, GL_DYNAMIC_DRAW, GL_STREAM_DRAW
	int			offset_texture;
	int			offset_normal;
	int			offset_vertex;
	int			offset_color;
	int			n_texture;
	int			n_normal;
	int			n_vertex;
	int			n_color;
	int			n_per_set;
	u_int32_t		n_data;
	u_int32_t		n_indices;
	sm_opengl_parm_t*	pData;
	short*			pIndices;
	bool			initialized;
	GLuint			DataBuffer_id;
	GLuint			IndexBuffer_id;
	GLuint			vao_id;
};

struct opengl_vector_t {
	char			*name;
	u_int32_t		id;
	u_int32_t		n_params;
#ifdef SM_OPENGL_PARM_TYPE_INT
	GLint			param[4];
#else
	GLfloat			param[4];
#endif
};

struct opengl_texture_t {
	char			*name;
	u_int32_t		id;
	GLuint			real_texture_id;
	texture_source_enum_t	source;
	u_int32_t		source_id;
	u_int32_t		source_seq_no;
};

struct opengl_draw_operation_t {
	opengl_operation_enum_t	type;
	//bool			activate;
	int32_t			execute;		// <0 Execute always
							// >0 Execute and decrement. Set inactive
							// when reaching 0
	int32_t			conditional;		// 0 = always execute
							// 1 = if osmesa execute
							// 2 = if glx execute
	char*			create_str;
	u_int8_t*		data;			// Private data. Needs to be freed too
	opengl_vbo_t*		pVbo;
	struct opengl_draw_operation_t*	next;

	union {
		sm_opengl_parm_t	x;
		sm_opengl_parm_t	red;
		GLfloat			redf;
#ifdef SM_OPENGL_PARM_TYPE_INT
		float			xf;	// for integer version for scale/rotate/translate
		int			light_param;
#else
		float			light_param;
#endif
		GLenum			mode;
		GLenum			sfactor;
		u_int32_t		shape_id;
		u_int32_t		texture_id;
		//u_int32_t		light_id;
		u_int32_t		vector_id;
		u_int32_t		level;
		u_int32_t		quad_id;
		u_int32_t		vbo_id;
		GLint			xi;
		GLdouble		fovy;
		GLdouble		top;
		GLdouble		left;		// ArcXZ
		GLdouble		size;		// For teapot
	} p1;
	union {
		sm_opengl_parm_t	y;
		sm_opengl_parm_t	green;
		GLfloat			greenf;
#ifdef SM_OPENGL_PARM_TYPE_INT
		float			yf;	// for integer version for scale/rotate/translate
#endif
		GLdouble		aspect;		// gluPerspective, ArcXZ
		GLdouble		radius;
		GLdouble		base;
		GLdouble		right;		//
		GLdouble		inner;		// gluDisk, gluPartialDisk
		GLenum			pname;
		GLint			yi;
		GLenum			dfactor;
		GLenum			normal;
		GLenum			draw;
		GLenum			orientation;
		GLboolean		texture;
		GLboolean		solid;		// For teapot. If not solid, then wire
	} p2;
	union {
		sm_opengl_parm_t	z;
		sm_opengl_parm_t	blue;
		GLfloat			bluef;
#ifdef SM_OPENGL_PARM_TYPE_INT
		float			zf;	// for integer version for scale/rotate/translate
#endif
		GLdouble		zNear;
		GLint			param_i;
		GLint			slices;		// gluCylinder, gluDisk, gluPartialDisk, ArcXZ
		const GLfloat*		param_f;
		GLsizei			width;
		GLenum			lighti;
		GLenum			face;
	} p3;
	union {
		sm_opengl_parm_t	w;
		sm_opengl_parm_t	alpha;
		sm_opengl_parm_t	angle;
		GLfloat			alphaf;
#ifdef SM_OPENGL_PARM_TYPE_INT
		float			anglef;	// for integer version for scale/rotate
#endif
		GLdouble		angled;		// ArcXZ
		GLdouble		zFar;
		GLint			stacks;
		GLint			loops;		// gluDisk, gluPartialDisk
		GLint			param_i;
		const GLfloat*		param_f;
		GLsizei			height;
	} p4;
	union {
		GLdouble		top;		// glOrtho, ArcXZ
		GLdouble		outer;		// gluDisk, gluPartialDisk
	} p5;
	union {
		GLdouble		height;
		GLdouble		bottom;		// glOrtho, ArcXZ
		GLdouble		start;		// gluDisk, gluPartialDisk
	} p6;
	union {
		GLdouble		sweep;		// gluDisk, gluPartialDisk
		GLdouble		aspect;		// ArcXZ
	} p7;
};

struct opengl_shape_t {
	char*		name;		// Name of shape
	u_int32_t	id;		// Id of shape
	u_int32_t	ops_count;	// Operation count
	opengl_draw_operation_t* pDraw;	// Start of draw operations
	opengl_draw_operation_t* pDrawTail;	// End of draw operations
};

// glshape place <place id> <shape id> <x> <y> <z> [ <scale x> <scale y> <scale z> [ <rotation> <rx> <ry> <rz> [ <red> <green> <blue> [ <alpha> ] ] ] ]
// glshape place alpha [ <place id> <alpha> ]
// glshape place rotation [ <place id> <rotation> <rx> <ry> <rz> ]
// glshape place scale [ <place id> <scale x> <scale y> <scale z> ]
// glshape place rgb [ <place id> <red> <green> <blue> [ <alpha> ] ]
struct opengl_placed_shape_t {
	char*		name;		// Name field for a placed shape
	u_int32_t	place_id;	// ID of placed opengl shape
	u_int32_t	shape_id;	// ID of opengl shape
	bool		push_matrix;	//

	struct {
		sm_opengl_parm_t	x;
		sm_opengl_parm_t	y;
		sm_opengl_parm_t	z;
	} translate;

	struct {
		sm_opengl_parm_t	angle;
		sm_opengl_parm_t	x;
		sm_opengl_parm_t	y;
		sm_opengl_parm_t	z;
	} rotation;

	struct {
		sm_opengl_parm_t	x;
		sm_opengl_parm_t	y;
		sm_opengl_parm_t	z;
	} scale;

	struct {
		sm_opengl_parm_t	red;
		sm_opengl_parm_t	green;
		sm_opengl_parm_t	blue;
		sm_opengl_parm_t	alpha;
	} color;
};

#ifdef HAVE_GLU
struct glu_quadric_list_t {
	u_int32_t			id;
	GLUquadric*			pQuad;
	struct glu_quadric_list_t*	next;
};
#endif // #ifdef HAVE_GLU

struct curve2d_set_t {
	sm_opengl_parm_t	x, z;
	sm_opengl_parm_t	tex;
};

class CVideoMixer;

class COpenglVideo {
  public:
	COpenglVideo(CVideoMixer *pVideomixer,
		u_int32_t max_shapes = MAX_OPENGL_SHAPES,
		u_int32_t max_shape_places = MAX_OPENGL_SHAPE_PLACES,
		u_int32_t max_textures = MAX_OPENGL_TEXTURES,
		u_int32_t max_vectors = MAX_OPENGL_VECTORS,
		u_int32_t max_vbos = MAX_OPENGL_VBOS,
		int verbose = 0);
	~COpenglVideo();

	int	ParseCommand(struct controller_type* ctr, const char* str);
	void	InvalidateCurrent() { m_current_context_is_valid = false; }
	void	SwapBuffers(u_int8_t* overlay);
	int	CopyBack(u_int8_t* dst, u_int32_t x = 0, u_int32_t y = 0, u_int32_t width = 0, u_int32_t height = 0);



	int	DestroyContext();		// Destroy context

	int	CreateContext(u_int8_t *baseframe,	// Frame use as base
			u_int32_t width = 0,		// system width will be used if 0
			u_int32_t height = 0,		// system height will be used if 0
			int depth_bits = 16);		// default is 16

	int	MakeCurrent(u_int8_t *baseframe,	// Frame use as base
			u_int32_t width = 0,		// system width will be used if 0
			u_int32_t height = 0);		// system height will be used if 0
							// default is 16

//	void 	render_image();
//	void 	render_image2(int w, int h);

  private:
	int	MakeIndexedVBO(const char* name, u_int32_t vbo_id);
	int	DestroyIndexedVBO(u_int32_t vbo_id);
	void	DestroyVBO(opengl_vbo_t* pVbo);
	struct opengl_vbo_t* MakeVBO(const char* name, u_int32_t id);
	int	ConfigVBO(struct opengl_vbo_t* pVbo, GLenum type, const char* config, GLenum usage = GL_DYNAMIC_DRAW);
	int	AddDataPointsVBO(opengl_vbo_t* pVbo, int n, sm_opengl_parm_t* pData);
	int	AddIndicesVBO(opengl_vbo_t* pVbo, int n, short* pIndices);
	int	LoadVBO(opengl_vbo_t* pVbo);
	int	ExecuteVBO(opengl_vbo_t* pVbo);

	int	ArcXZplot2(opengl_draw_operation_t* p);

	int	ArcXZplot(curve2d_set_t* curve_set, u_int32_t slices,
			double texbottom, double textop);
	curve2d_set_t*	ArcXZcalculate(opengl_operation_enum_t type, double angle, double aspect, GLint slices, double texleft, double texright);
	int	MakeArcXZ();
	
	int	ShapeOps(u_int32_t shape_id);
	int	OverlayPlacedShape(u_int32_t place_id);
	int	ExecuteShape(u_int32_t shape_id, u_int32_t level = OVERLAY_GLSHAPE_LEVELS,
			double scale_x = 1.0, double scale_y = 1.0, double scale_z = 1.0);
	int	DeleteShape(u_int32_t id);
	int	DeletePlacedShape(u_int32_t id);
	int	DeleteLight(u_int32_t id);
	int	DeleteTexture(u_int32_t id);
	int	ShapeMoveEntry(u_int32_t shape_id, u_int32_t entry_id, u_int32_t to_entry);
	int	AddPlacedShape(u_int32_t place_id, u_int32_t shape_id,
			double x, double y, double z);
	int	SetPlacedShapeRotation(u_int32_t place_id, double angle = 0,
			double x = 1, double y = 1, double z = 1);
	int	SetPlacedShapeCoor(u_int32_t place_id, double x, double y, double z);
	int	SetPlacedShapeScale(u_int32_t place_id, double x = 1, double y = 1, double z = 1);
	int	SetPlacedShapeColor(u_int32_t place_id, double red = 0, double green = 0, double blue = 0, double alpha = -1);
	int	SetPlacedShapeAlpha(u_int32_t place_id, double alpha);
	char*	DuplicateCommandString(const char* str);
	int	AddShape(u_int32_t shape_id, const char* name);
	int	AddTexture(u_int32_t shape_id, const char* name);
	int	AddVector(u_int32_t shape_id, const char* name);
	int	AddVectorValues(u_int32_t vector_id, u_int32_t count,
#ifdef SM_OPENGL_PARM_TYPE_INT
        		GLint param0, GLint param1, GLint param2 = 0, GLint param3 = 0);
#else
        		GLfloat param0, GLfloat param1, GLfloat param2 = 0, GLfloat param3 = 0);
#endif

	int	SetTextureSource(u_int32_t texture_id, texture_source_enum_t source, u_int32_t source_id);
	int	AddToShapeCommand(u_int32_t shape_id, opengl_operation_enum_t type);
	int	LoadTexture(u_int32_t texture_id, GLint min, GLint mag);
	int	ActivateShapeEntry(u_int32_t shape_id, u_int32_t line, int32_t execute);
	int	ModifyEntry(opengl_draw_operation_t* pDraw, u_int32_t param_no, const char* value);
	int	ModifyShapeEntryValues(u_int32_t shape_id, u_int32_t line, const char* numbers, const char* values) ;
	int	PrintShapeElements(struct controller_type* ctr, u_int32_t shape_id);

#ifdef HAVE_GLU
	GLUquadric* CreateGluQuadric(u_int32_t id);
	GLUquadric* FindGluQuadric(u_int32_t id);
	void	DeleteGluQuadric(glu_quadric_list_t* p, bool recursive = false);
#endif // #ifdef HAVE_GLU

	int	overlay_placed_shape(struct controller_type* ctr, const char *str);
	int	set_vbo_add(struct controller_type* ctr, const char* str);
	int	set_vbo_config(struct controller_type* ctr, const char* str);
	int	set_vbo_data(struct controller_type* ctr, const char* str);
	int	set_vbo_indices(struct controller_type* ctr, const char* str);
	int	set_shape_place(struct controller_type* ctr, const char* str);
	int	set_verbose(struct controller_type* ctr, const char* str);
	int	list_shape_info(struct controller_type* ctr, const char* str);
	int	list_glshape_help(struct controller_type* ctr, const char* str);
	int	set_copy_back(struct controller_type* ctr, const char* str);
	int	set_shape_add(struct controller_type* ctr, const char* str);
	int	set_shape_scale(struct controller_type* ctr, const char* str);
	int	set_shape_coor(struct controller_type* ctr, const char* str);
	int	set_shape_rgb(struct controller_type* ctr, const char* str, bool isrgb = true);
	int	set_shape_rotation(struct controller_type* ctr, const char* str);
	int	set_shape_alpha(struct controller_type* ctr, const char* str);
	int	set_vector_add(struct controller_type* ctr, const char* str);
	int	set_vector_value(struct controller_type* ctr, const char* str);
	int	set_texture_add(struct controller_type* ctr, const char* str);
	int	set_texture_bind(struct controller_type* ctr, const char* str);
	int	set_texture_source(struct controller_type* ctr, const char* str);
	int	set_shape_list(struct controller_type* ctr, const char* str);
	int	set_shape_moveentry(struct controller_type* ctr, const char* str);
	int	set_shape_no_parameter(struct controller_type* ctr,
			const char* str, opengl_operation_enum_t type);
	int	set_shape_enum_or_int_parameters(struct controller_type* ctr,
			const char* str, opengl_operation_enum_t type);
	int	set_shape_nummerical_parameters(struct controller_type* ctr,
			const char* str, opengl_operation_enum_t type);
	int	set_preferred_context(struct controller_type* ctr, const char* str);
	int	CheckForGlError(u_int32_t verbose = 0, const char* str = NULL);

	CVideoMixer*		m_pVideoMixer;

	OSMesaContext		m_pCtx_OSMesa;
	u_int32_t		m_width;
	u_int32_t		m_height;
	int			m_depth_bits;
	u_int32_t		m_verbose;
	u_int32_t		m_shape_count;
	u_int32_t		m_texture_count;
	u_int32_t		m_vector_count;
	u_int32_t		m_vbo_count;
	u_int32_t		m_vbo_total_count;
	u_int32_t		m_placed_shape_count;

	u_int32_t		m_max_shapes;
	u_int32_t		m_max_placed_shapes;
	u_int32_t		m_max_textures;
	u_int32_t		m_max_vectors;
	u_int32_t		m_max_vbos;
	opengl_shape_t**	m_shapes;
	opengl_placed_shape_t**	m_placed_shapes;
	opengl_texture_t**	m_textures;
	opengl_vector_t**	m_vectors;
	opengl_vbo_t**		m_vbos;
#ifdef HAVE_GLU
	glu_quadric_list_t*	m_glu_quadrics;
#endif
	bool			m_current_context_is_valid;
	bool			m_use_glx;
#ifdef WITH_GLX
	Display			*m_display;
	int			m_screen;
	int			m_fb_count;
	GLXContext		m_pCtx_GLX;
	int			m_glx_major;
	int			m_glx_minor;
	Window			m_win;
	XVisualInfo*		m_vi;
	GLuint			m_framebuffer_draw;
	GLuint			m_framebuffer_read;
#endif
};

#endif	// OPENGL_VIDEO
