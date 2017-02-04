/*
 * (c) 2015 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

//http://www.videotutorialsrock.com/opengl_tutorial/cube/video.php

#define GL_GLEXT_PROTOTYPES 1

#if defined (HAVE_OSMESA) || defined (HAVE_GLX)

//#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
//#include <limits.h>
#include <string.h>
//#include <malloc.h>
//#include <errno.h>
//#include <sys/types.h>

//#include <sys/stat.h>
//#include <fcntl.h>

//#ifdef WIN32
//#include <winsock.h>
//#include "windows_util.h"
//#else
//#include <sys/socket.h>
//#include <sys/utsname.h>
//#endif

#define WITH_OSMESA 1

#include <math.h>


#ifdef WITH_OSMESA
# include "GL/osmesa.h"
# ifdef HAVE_MACOSX
#  include <OpenGL/OpenGL.h>
#  ifdef HAVE_GLUT
#   include <GLUT/glut.h>
#  endif
# else
#  ifdef HAVE_GLUT
#   include <GL/glut.h>
#  endif
# endif
//#include <GL/gl.h>
//#include "GL/glu.h"
//#include <GL/glu_mangle.h>
//#include "gl_wrap.h"
#endif
#include <GL/glext.h>

//#include "snowmix.h"
//#include "video_text.h"
//#include "video_scaler.h"
//#include "video_image.h"
//#include "add_by.h"
//#include "command.h"
//#include "snowmix_util.h"

#include "opengl_video.h"
#include "video_mixer.h"
#include "controller.h"

#if __WORDSIZE == 32
#define BUFFER_OFFSET(i) ((void*)(((uint32_t)i)))
#endif
#if __WORDSIZE == 64
#define BUFFER_OFFSET(i) ((void*)(((uint64_t)i)))
#endif

int COpenglVideo::ArcXZplot2(opengl_draw_operation_t* pDraw) {
	if (!pDraw) return -1;

	opengl_vbo_t* pVbo = pDraw->pVbo;
	curve2d_set_t* curve_set = (curve2d_set_t*) pDraw->data;
	u_int32_t slices	= 2*pDraw->p3.slices;
	double texbottom	= pDraw->p6.bottom;
	double textop		= pDraw->p5.top;
	double aspect		= pDraw->p7.aspect;

//fprintf(stderr, "ArcXZplot2 A %s %s slices %u bot %.4f top %.4f\n",
//	pVbo ? "ptr" : "null", curve_set ? "ptr" : "null", slices, texbottom, textop);
	if (!pVbo || !curve_set || !slices || texbottom < 0.0 || aspect <= 0.0 ||
		texbottom > 1.0 || textop < 0.0 || textop > 1.0) return -1;
	
	if (!pVbo->pData) {
		sm_opengl_parm_t* pData = (sm_opengl_parm_t*)
		malloc(5*2*(slices+1)*sizeof(sm_opengl_parm_t));
		if (pData) {
			if (m_verbose) fprintf(stderr, "ArcXZ Creating data points\n");
			sm_opengl_parm_t* p = pData;
			if (pDraw->type == SM_GL_ARCXZ) for (unsigned int i = 0; i <= slices ; i++) {

				// Bottom TexCoord
				// glTexCoord2(curve_set[i].tex, texbottom)
				*p++ = curve_set[i].tex;
				*p++ = texbottom;

				// Bottom Vertex coordinate
				// glVertex3(curve_set[i].x, -0.5, curve_set[i-1].z)
				*p++ = curve_set[i].x;
				*p++ = -0.5;
				*p++ = curve_set[i].z;

				// glTexCoord2(curve_set[i].tex, texbottom)
				*p++ = curve_set[i].tex;
				*p++ = textop;

				// glVertex3(curve_set[i].x, -0.5, curve_set[i-1].z)
				*p++ = curve_set[i].x;
				*p++ = 0.5;
				*p++ = curve_set[i].z;
			}
			else if (pDraw->type == SM_GL_ARCYZ) for (unsigned int i = 0; i <= slices ; i++) {
				// Bottom TexCoord
				// glTexCoord2(curve_set[i].tex, texbottom)
				*p++ = texbottom;
				*p++ = curve_set[i].tex;

				// Bottom Vertex coordinate
				// glVertex3(curve_set[i].x, -0.5, curve_set[i-1].z)
				*p++ = -0.5*aspect;
				*p++ = curve_set[i].x;
				*p++ = curve_set[i].z;

				// glTexCoord2(curve_set[i].tex, texbottom)
				*p++ = textop;
				*p++ = curve_set[i].tex;

				// glVertex3(curve_set[i].x, -0.5, curve_set[i-1].z)
				*p++ = 0.5*aspect;
				*p++ = curve_set[i].x;
				*p++ = curve_set[i].z;
			}
			AddDataPointsVBO(pVbo, 0, NULL);		// Deletes data points if any
			if (AddDataPointsVBO(pVbo, 2*(1+slices)*5, pData))
				fprintf(stderr, "Failed to add data points to VBO\n");
		} else fprintf(stderr, "Failed to get memory for datapoints for VBO\n");
		short* pIndices = (short*) malloc(sizeof(short)*4*(slices+1));
		if (pIndices) {
			short* p = pIndices;
			for (unsigned short i = 0; i < slices ; i++) {
				// indices 0 2 3 1  2 4 5 3  4 6 7 5  6 8 9 7 etc = i*2, i*2+2, i*2+3, i*2+1
				short n = i<<1;
				*p++ = n;
				*p++ = n+2;
				*p++ = n+3;
				*p++ = n+1;
if (m_verbose > 1) fprintf(stderr, "VBO vbo_id %u Indices %hd : %hd %hd %hd %hd\n",
	pVbo->id, i, *(p-4), *(p-3), *(p-2), *(p-1));
			}
			AddIndicesVBO(pVbo, 0, NULL);	// Delete indices if any
			if (AddIndicesVBO(pVbo, 4*slices, pIndices))
				fprintf(stderr, "Failed to add indices to VBO\n");
		} else if (m_verbose) fprintf(stderr, "Failed to get memory for indices for VBO\n");
	}
	if (!pVbo->initialized) LoadVBO(pVbo);
	ExecuteVBO(pVbo);
	return 0;
}

void SetOffsetAndEnable(opengl_vbo_t* pVbo) {
	if (!pVbo || !pVbo->DataBuffer_id) return;

//if (pVbo->id) fprintf(stderr, "Set Offset and enable client state for vbo %u vao %u\n", pVbo->id, pVbo->vao_id);

	//glBindBuffer(GL_ARRAY_BUFFER, pVbo->DataBuffer_id);
	// Check we need to set color offset
	if (pVbo->n_color > 0 || pVbo->offset_color >= 0) {
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(pVbo->n_color, SM_OPENGL_PARM_GL_TYPE,
			sizeof(sm_opengl_parm_t) * pVbo->n_per_set,
			BUFFER_OFFSET(pVbo->offset_color));
		//glDisableClientState(GL_COLOR_ARRAY);
	}
	// Check if we need to set normal offset
	if (pVbo->n_normal > 0 || pVbo->offset_normal >= 0) {
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(SM_OPENGL_PARM_GL_TYPE,
			sizeof(sm_opengl_parm_t) * pVbo->n_per_set,
			BUFFER_OFFSET(pVbo->offset_normal));
		//glDisableClientState(GL_NORMAL_ARRAY);
	}
	// Check if we need to set texture offset
	if (pVbo->n_texture > 0 || pVbo->offset_texture >= 0) {
		//glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(pVbo->n_texture, SM_OPENGL_PARM_GL_TYPE,
			sizeof(sm_opengl_parm_t) * pVbo->n_per_set,
			BUFFER_OFFSET(pVbo->offset_texture));
		//glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}
	// Check if we need to set vertex offset
	if (pVbo->n_vertex > 0 || pVbo->offset_vertex >= 0) {
		void (*foo)(GLenum) = glEnableClientState;
		foo(GL_VERTEX_ARRAY);
		//glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(pVbo->n_vertex, SM_OPENGL_PARM_GL_TYPE,
			sizeof(sm_opengl_parm_t) * pVbo->n_per_set,
			BUFFER_OFFSET(pVbo->offset_vertex));
		//glDisableClientState(GL_VERTEX_ARRAY);
	}
		glClientActiveTexture(GL_TEXTURE0);
}

// Load VBO to GPU. If the VBO is not bound to buffers, buffers will be allocated and bound
int COpenglVideo::LoadVBO(opengl_vbo_t* pVbo) {
if (m_verbose) fprintf(stderr, "LoadVBO %d\n", pVbo ? pVbo->id : -1);
	if (!pVbo || !pVbo->pData || !pVbo->pIndices ||
		(pVbo->n_indices == 0 && pVbo->n_data == 0)) return -1;
if (pVbo->id && !pVbo->vao_id) {
	glGenVertexArrays(1,&pVbo->vao_id);
	if (pVbo->vao_id)
		fprintf(stderr, "Generating VAO %u for vbo %u\n", pVbo->vao_id, pVbo->id);
	else
		fprintf(stderr, "Failed generating VAO for vbo %u\n", pVbo->id);
}
	// Check if we need to generate an index buffer
	if (!pVbo->IndexBuffer_id) {
		glGenBuffers(1, &pVbo->IndexBuffer_id);
		if (m_verbose) fprintf(stderr, " - LoadVBO vbo_id %d : generating "
			"indexbuffer %d\n", pVbo ? pVbo->id : -1, pVbo->IndexBuffer_id);
		if (!pVbo->IndexBuffer_id) {
			if (m_verbose > 1) fprintf(stderr, "Failed to allocate buffer for VBO %u\n",
				pVbo->id);
				return -1;
		}
	}

	// Check if we need to generate a data buffer
	if (!pVbo->DataBuffer_id) {
		glGenBuffers(1, &pVbo->DataBuffer_id);
		if (m_verbose) fprintf(stderr, " - LoadVBO vbo_id %d : generating databuffer %d\n",
			pVbo ? pVbo->id : -1, pVbo->DataBuffer_id);
		if (!pVbo->DataBuffer_id) {
			if (m_verbose > 1) fprintf(stderr, "Failed to allocate buffer for VBO %u\n",
				pVbo->id);
			return -1;
		}
	}

	if (pVbo->vao_id) glBindVertexArray(pVbo->vao_id);

	// Bind buffer and transfer indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pVbo->IndexBuffer_id);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ushort) * pVbo->n_indices,
		pVbo->pIndices, pVbo->usage);
if (m_verbose > 0) {
	fprintf(stderr, " - LoadVBO vbo_id %d : indexbuffer %d transfered %u indices\n -- Indices = ", pVbo ? pVbo->id : -1, pVbo->IndexBuffer_id, pVbo->n_indices);
	for (unsigned int i=0 ; i < pVbo->n_indices; i++)
		fprintf(stderr, "%hu ", pVbo->pIndices[i]);
		fprintf(stderr, "\n");
	}

	// Bind buffer and transfer data
	glBindBuffer(GL_ARRAY_BUFFER, pVbo->DataBuffer_id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(sm_opengl_parm_t) * pVbo->n_data,
		pVbo->pData, pVbo->usage);
if (m_verbose > 0) {
	fprintf(stderr, " - LoadVBO vbo_id %d : databuffer %d transfered %u points\n -- Data = ", pVbo ? pVbo->id : -1, pVbo->DataBuffer_id, pVbo->n_data);
	for (unsigned int i=0 ; i < pVbo->n_data; i++)
		fprintf(stderr, "%f ", pVbo->pData[i]);
		fprintf(stderr, "\n");
	}
	if (pVbo->vao_id) {
		if (m_verbose) fprintf(stderr, " - LoadVBO vbo_id %d : vao enabled %u\n", pVbo->id, pVbo->vao_id);
		SetOffsetAndEnable(pVbo);
		glBindVertexArray(0);
	}

	pVbo->initialized = true;
if (m_verbose > 0) fprintf(stderr, " - LoadVBO %d : Initialized\n", pVbo ? pVbo->id : -1);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return 0;
}

// Execute the VBO. VBO is assumed to have been initialized
int COpenglVideo::ExecuteVBO(opengl_vbo_t* pVbo) {
	if (!pVbo) return 0;
	if (pVbo->initialized && pVbo->IndexBuffer_id) {
		if (m_verbose > 2) fprintf(stderr, "Execute vbo_id %u indexbuffer id %u "
			"indices %u datapoints %u sets %u\n", pVbo->id,
			pVbo->IndexBuffer_id, pVbo->n_indices, pVbo->n_data,
			(pVbo->n_data / pVbo->n_per_set) );
		
		if (!pVbo->vao_id) {
			glBindBuffer(GL_ARRAY_BUFFER, pVbo->DataBuffer_id);
			SetOffsetAndEnable(pVbo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pVbo->IndexBuffer_id);
			glDrawRangeElements(pVbo->type, 0, (pVbo->n_data / pVbo->n_per_set) -1,
				(GLsizei) pVbo->n_indices, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		}
		else {
//fprintf(stderr, "Binding Vertex Array %u in VBO %u\n", pVbo->vao_id, pVbo->id);
			glBindVertexArray(pVbo->vao_id);
			glDrawRangeElements(pVbo->type, 0, (pVbo->n_data / pVbo->n_per_set) -1,
				(GLsizei) pVbo->n_indices, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
			glBindVertexArray(0);
		}
	}

	return 0;
}

//curve angle aspect slices left right top bottom
//
//	p1.left   (double)
//	p2.right  (double)
//	p3.slices (int)
//	p4.angled (double)
//	p5.top    (double)
//	p6.bottom (double)
//	p7.aspect (double)
curve2d_set_t* COpenglVideo::ArcXZcalculate(opengl_operation_enum_t type, double angle, double aspect, GLint slices, double texleft, double texright) {
	bool mirror = false;
	// Make sure slices is 1 or greater and aspect is greater than zero
	if (slices < 1) slices = 1;
	if (0.5*aspect == 0.0) return NULL;

	if (angle < 0.0) {
		angle = -angle;
		mirror = true;
	}
	if (angle/slices == 0) return NULL;
	// Make sure angle is greater than 0 and less or equal to 180
	if (angle > 180.0) angle = 180.0;

	// Check texleft/texright. Must be between 0 and 1 and left < right
	if (texleft < 0.0 || texleft >= 1.0 || texright <= 0.0 ||
		texright > 1.0 || texleft > texright) return NULL;

	curve2d_set_t* curve_set = (curve2d_set_t*) malloc(sizeof(curve2d_set_t)*(2*slices+1));
	if (!curve_set) {
		fprintf(stderr, "Failed to get memory for curve_set\n");
		return NULL;
	}

	// k is a factor to ensure vi finally get a glshape from -0.5 to 0.5 being 1 unit wide
	// Convert angle to radians.
	angle = M_PI*angle/180.0;

	if (type == SM_GL_ARCXZ || type == SM_GL_ARCYZ) {
		// C = Circumfence, r = radius, a = angle in radians.
		// Circumfence = 2*PI*r. The curved length in X is 0.5*aspect.
		// r = 0.5 * aspect / angle	// XZ
		// r = 0.5 / angle		// YZ
		// x = -r * sin(angle)
		// z = -r + r * cos(angle)
		double radius;
		if (type == SM_GL_ARCXZ) radius = 0.5 * aspect / angle;
		else radius = 0.5 / angle;

		slices = 2*slices;
		double slicesd = slices;
		float tex_x = texright - texleft;
		if (m_verbose > 1)fprintf(stderr, "angle=%.4lf, radius=%.4lf, slices=%d\n", angle, radius, slices);
		for (int i = 0 ; i <= slices ; i++) {
			// anglei = angle - angle*i/
			float anglei = angle * (1.0 - 2*i / slicesd);
			curve_set[i].x = - radius * sin(anglei);
			if (mirror)
				curve_set[i].z = radius *(cos(anglei) - 1);
			else
				curve_set[i].z = radius *(1 - cos(anglei));
			curve_set[i].tex = texleft + i * tex_x / slicesd; // x, vertical
			if (m_verbose > 1) fprintf(stderr, "ArcXZ %d:%d angle %.2f x,z=%.2f,%.2f\n",
				i, slices, (float)(anglei*180/M_PI), curve_set[i].x, curve_set[i].z);
		}
	}
	return curve_set;
}


#ifdef SM_OPENGL_PARM_TYPE_INT
#error The function COpenglVideo::ArcXZplot does not work for Integer. FIXME
#endif
/* Should be changed to use Vertex Buffer Objects.
 * See https://www.opengl.org/wiki/VBO_-_just_examples
 * See http://en.wikipedia.org/wiki/Vertex_Buffer_Object
 */
struct quad_with_tex_t {
	sm_opengl_parm_t tx,ty;
	sm_opengl_parm_t x,y,z;
};

int COpenglVideo::ArcXZplot(curve2d_set_t* curve_set, u_int32_t slices, double texbottom, double textop) {
	if (!curve_set) return -1;

	// Check texbottom/textop. Must be between 0 and 1 and bottom < top
	if (texbottom < 0.0f || texbottom >= 1.0f || textop <= 0.0f || textop > 1.0f || texbottom >= textop) return -1;

	slices = 2*slices;



	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	  SM_GL_OPER_NORMAL(0.0, 0.0, 1.0);
	  for (unsigned int i = 1; i <= slices ; i++) {
		// Left Bottom Coordinate
		SM_GL_OPER_TEXCOORD2(curve_set[i-1].tex, texbottom);
		SM_GL_OPER_VERTEX3(curve_set[i-1].x, -0.5, curve_set[i-1].z);

		// Right  bottom Coordinate
		SM_GL_OPER_TEXCOORD2(curve_set[i].tex, texbottom);
		SM_GL_OPER_VERTEX3(curve_set[i].x, -0.5, curve_set[i].z);

		// Right Top Coordinate
		SM_GL_OPER_TEXCOORD2(curve_set[i].tex, textop);
		SM_GL_OPER_VERTEX3(curve_set[i].x, 0.5, curve_set[i].z);

		// Left Bottom Coordinate
		SM_GL_OPER_TEXCOORD2(curve_set[i-1].tex, textop);
		SM_GL_OPER_VERTEX3(curve_set[i-1].x, 0.5, curve_set[i-1].z);
	  }
	glEnd();
	return 0;
}

int COpenglVideo::MakeArcXZ() {
	int32_t slices = 12;
	double angle = 75.0;
	double aspect = 1.7778;
	curve2d_set_t* curve = ArcXZcalculate(SM_GL_ARCXZ, angle, aspect, slices, 0.0, 1.0);
	if (!curve) return -1;
	ArcXZplot(curve, slices, 0.0f, 1.0f);
	free(curve);
	return 0;
}

/*
struct gl_parameter_set { const char* name; size_t len; GLenum parmname; };
static struct gl_parameter_set gl_parameter_list[] = {
	{ "texture_2d ", 11, GL_TEXTURE_2D },
	{ "min_filter ", 11, GL_TEXTURE_MIN_FILTER },
	{ "mag_filter ", 11, GL_TEXTURE_MAG_FILTER },
	{ NULL, 0, 0 } };
*/


// Mode2String is used to map a mode to a string searching in gl_enum_set_t variables
#define Mode2String(res, search, list) {	\
	int i;					\
	res = NULL;				\
	for (i=0; list[i].name; i++)		\
		if (search == list[i].mode) {	\
			res = list[i].name;	\
			break;			\
		}				\
}

struct gl_enum_set_t { const char* name; size_t len; GLenum mode; };

static struct gl_enum_set_t gl_material_face_list[] = {
	{ "front ",		6, GL_FRONT			},
	{ "back ",		5, GL_FRONT			},
	{ "front_and_back ",	15,GL_FRONT_AND_BACK		},
	{ NULL,			0,  GL_NONE			}
};

// n_params is the number of elements in an array needed for the material/light mode
// in OpenGL functions lightv and materialv
struct gl_enum_parm_set_t { const char* name; size_t len; int n_params; GLenum mode; };

static struct gl_enum_parm_set_t gl_material_parm_list[] = {
	{ "ambient ",		8,  4, GL_AMBIENT		},
	{ "diffuse ",		8,  4, GL_DIFFUSE		},
	{ "specular ",		9,  4, GL_SPECULAR		},
	{ "emission ",		9,  4, GL_EMISSION		},
	{ "shininess ",		10, 1, GL_SHININESS		},
	{ "ambient_and_diffuse ",20, 4, GL_AMBIENT_AND_DIFFUSE	},
	{ "color_indexes ",	14, 3, GL_COLOR_INDEXES		},
	{ NULL,			0,  0, GL_NONE		}
};

static struct gl_enum_parm_set_t gl_light_parm_list[] = {
	{ "ambient ",		8,  4, GL_AMBIENT		},
	{ "diffuse ",		8,  4, GL_DIFFUSE		},
	{ "specular ",		9,  4, GL_SPECULAR		},
	{ "position ",		9,  4, GL_POSITION		},
	{ "spot_cutoff ",	12, 1, GL_SPOT_CUTOFF		},
	{ "spot_direction ",	15, 3, GL_SPOT_DIRECTION	},
	{ "spot_exponent ",	14, 1, GL_SPOT_EXPONENT		},
	{ "constant ",		9,  1, GL_CONSTANT_ATTENUATION	},
	{ "linear ",		7,  1, GL_LINEAR_ATTENUATION	},
	{ "quadratic ",		10, 1, GL_QUADRATIC_ATTENUATION	},
	{ NULL,			0,  0, GL_NONE		}
};

static struct gl_enum_set_t gl_modify_list[] = {
	{ "arcxz ",		6, SM_GL_ARCXZ		},
	{ "arcyz ",		6, SM_GL_ARCYZ		},
	{ "arcxy ",		6, SM_GL_ARCXY		},
	{ "clearcolor ",	11, SM_GL_CLEARCOLOR	},
	{ "color3 ",		7, SM_GL_COLOR3		},
	{ "color4 ",		7, SM_GL_COLOR4		},
#ifdef HAVE_GLU
	{ "gluperspective ",	15, SM_GLU_PERSPECTIVE	},
#endif
	{ "normal ",		7, SM_GL_NORMAL		},
	{ "ortho ",		6, SM_GL_ORTHO		},
	{ "rotate ",		7, SM_GL_ROTATE		},
	{ "scale ",		6, SM_GL_SCALE		},
	{ "scissor ",		8, SM_GL_SCISSOR	},
	{ "texcoord2 ",		10, SM_GL_TEXCOORD2	},
	{ "texcoord3 ",		10, SM_GL_TEXCOORD3	},
	{ "texcoord4 ",		10, SM_GL_TEXCOORD4	},
	{ "translate ",		10, SM_GL_TRANSLATE	},
	{ "viewport ",		9, SM_GL_VIEWPORT	},
	{ "vertex2 ",		8, SM_GL_VERTEX2	},
	{ "vertex2 ",		8, SM_GL_VERTEX3	},
	{ "vertex2 ",		8, SM_GL_VERTEX4	},
	{ NULL,			0, GL_NONE		}
};

static struct gl_enum_set_t gl_begin_list[] = {
 	{ "lines ",		6,  GL_LINES		},
 	{ "line_strip ",	11, GL_LINE_STRIP	},
 	{ "line_loop ",		10, GL_LINE_LOOP	},
	{ "points ",		7,  GL_POINTS		},
 	{ "polygon ",		8,  GL_POLYGON		},
 	{ "quad_strip ",	11, GL_QUAD_STRIP	},
 	{ "quads ",		6,  GL_QUADS		},
 	{ "triangles ",		10, GL_TRIANGLES	},
 	{ "triangle_strip ",	15, GL_TRIANGLE_STRIP	},
 	{ "triangle_fan ",	13, GL_TRIANGLE_FAN	},
	{ NULL,			0,  GL_NONE		}
};

static struct gl_enum_set_t gl_blendfunc_list[] = {
	{ "zero ",			 5, GL_ZERO			},
	{ "one ",			 4, GL_ONE			},
	{ "src_color ",			10, GL_SRC_COLOR		},
	{ "one_minus_src_color ",	20, GL_ONE_MINUS_SRC_COLOR	},
	{ "dst_color ",			10, GL_DST_COLOR		},
	{ "one_minus_dst_color ",	20, GL_ONE_MINUS_DST_COLOR	},
	{ "src_alpha ",			10, GL_SRC_ALPHA		},
	{ "one_minus_src_alpha ",	20, GL_ONE_MINUS_SRC_ALPHA	},
	{ "dst_alpha ",			10, GL_DST_ALPHA		},
	{ "one_minus_dst_alpha ",	20, GL_ONE_MINUS_DST_ALPHA	},
	{ "constant_color ",		15, GL_CONSTANT_COLOR		},
	{ "one_minus_constant_color ",	25, GL_ONE_MINUS_CONSTANT_COLOR	},
	{ "constant_alpha ",		15, GL_CONSTANT_ALPHA		},
	{ "one_minus_constant_alpha ",	25, GL_ONE_MINUS_CONSTANT_ALPHA	},
	{ "src_alpha_saturate ",	18, GL_SRC_ALPHA_SATURATE	},
	{ NULL, 0, GL_NONE							}
};

static struct gl_enum_set_t gl_enable_mode_list[] = {
	{ "depth_test ", 11, GL_DEPTH_TEST },		// Placed here for fast lookup
	{ "blend ", 6, GL_BLEND },			// Placed here for fast lookup
	{ "texture_2d ", 11, GL_TEXTURE_2D },		// Placed here for fast lookup
	{ "normalize ", 10, GL_NORMALIZE },		// Placed here for fast lookup
	{ "lighting ", 9, GL_LIGHTING },		// Placed here for fast lookup
	{ "clip_plane0 ", 12, GL_CLIP_PLANE0 },		// Placed here for fast lookup
	{ "light0 ",			7, GL_LIGHT0 },		// Placed here for fast lookup

	{ "alpha_test ",		11, GL_ALPHA_TEST },	// Rest is placed alphabetically
	{ "auto_normal ",		12, GL_AUTO_NORMAL },
	{ "clip_plane1 ",		12, GL_CLIP_PLANE1 },
	{ "clip_plane2 ",		12, GL_CLIP_PLANE2 },
	{ "clip_plane3 ",		12, GL_CLIP_PLANE3 },
	{ "clip_plane4 ",		12, GL_CLIP_PLANE4 },
	{ "clip_plane5 ",		12, GL_CLIP_PLANE5 },
	{ "color_logic_op ",		15, GL_COLOR_LOGIC_OP },
	{ "color_material ",		15, GL_COLOR_MATERIAL },
	{ "color_sum ",			10, GL_COLOR_SUM },
	{ "color_table ",		12, GL_COLOR_TABLE },
	{ "convolution_1d ",		15, GL_CONVOLUTION_1D },
	{ "convolution_2d ",		15, GL_CONVOLUTION_2D },
	{ "cull_face ",			10, GL_CULL_FACE },
	{ "dither ",			 7, GL_DITHER },
	{ "fog ",			 4, GL_FOG },
	{ "histogram ",			10, GL_HISTOGRAM },
	{ "index_logic_op ",		15, GL_INDEX_LOGIC_OP },
	{ "light1 ",			 7, GL_LIGHT1 },
	{ "light2 ",			 7, GL_LIGHT2 },
	{ "light3 ",			 7, GL_LIGHT3 },
	{ "light4 ",			 7, GL_LIGHT4 },
	{ "light5 ",			 7, GL_LIGHT5 },
	{ "light6 ",			 7, GL_LIGHT6 },
	{ "light7 ",			 7, GL_LIGHT7 },
	{ "line_smooth ",		12, GL_LINE_SMOOTH },
	{ "line_stipple ",		13, GL_LINE_STIPPLE },
	{ "map1_color_4 ",		13, GL_MAP1_COLOR_4 },
	{ "map1_index ",		11, GL_MAP1_INDEX },
	{ "map1_normal ",		12, GL_MAP1_NORMAL },
	{ "map1_texture_coord_1 ",	21, GL_MAP1_TEXTURE_COORD_1 },
	{ "map1_texture_coord_2 ",	21, GL_MAP1_TEXTURE_COORD_2 },
	{ "map1_texture_coord_3 ",	21, GL_MAP1_TEXTURE_COORD_3 },
	{ "map1_texture_coord_4 ",	21, GL_MAP1_TEXTURE_COORD_4 },
	{ "map1_vertex_3 ",		14, GL_MAP1_VERTEX_3 },
	{ "map1_vertex_4 ",		14, GL_MAP1_VERTEX_4 },
	{ "map2_color_4 ",		13, GL_MAP2_COLOR_4 },
	{ "map2_index ",		11, GL_MAP2_INDEX },
	{ "map2_normal ",		12, GL_MAP2_NORMAL },
	{ "map2_texture_coord_1 ",	21, GL_MAP2_TEXTURE_COORD_1 },
	{ "map2_texture_coord_2 ",	21, GL_MAP2_TEXTURE_COORD_2 },
	{ "map2_texture_coord_3 ",	21, GL_MAP2_TEXTURE_COORD_3 },
	{ "map2_texture_coord_4 ",	21, GL_MAP2_TEXTURE_COORD_4 },
	{ "map2_vertex_3 ",		14, GL_MAP2_VERTEX_3 },
	{ "map2_vertex_4 ",		14, GL_MAP2_VERTEX_4 },
	{ "minmax ",			 7, GL_MINMAX },
	{ "multisample ",		12, GL_MULTISAMPLE },
	{ "point_smooth ",		13, GL_POINT_SMOOTH },
	{ "point_sprite ",		13, GL_POINT_SPRITE },
	{ "polygon_offset_fill ",	20, GL_POLYGON_OFFSET_FILL },
	{ "polygon_offset_point ",	21, GL_POLYGON_OFFSET_POINT },
	{ "polygon_smooth ",		15, GL_POLYGON_SMOOTH },
	{ "polygon_stipple ",		16, GL_POLYGON_STIPPLE },
	{ "post_color_matrix_color_table ", 30, GL_POST_COLOR_MATRIX_COLOR_TABLE },
	{ "post_convolution_color_table ", 29, GL_POST_CONVOLUTION_COLOR_TABLE },
	{ "rescale_normal ",		15, GL_RESCALE_NORMAL },
	{ "sample_alpha_to_coverage ",	25, GL_SAMPLE_ALPHA_TO_COVERAGE },
	{ "sample_alpha_to_one ",	20, GL_SAMPLE_ALPHA_TO_ONE },
	{ "sample_coverage ",		16, GL_SAMPLE_COVERAGE },
	{ "separable_2d ",		13, GL_SEPARABLE_2D },
	{ "scissor_test ",		13, GL_SCISSOR_TEST },
	{ "stencil_test ",		13, GL_STENCIL_TEST },
	{ "texture_1d ",		11, GL_TEXTURE_1D },
	{ "texture_3d ",		11, GL_TEXTURE_3D },
	{ "texture_cube_map ",		17, GL_TEXTURE_CUBE_MAP },
	{ "texture_gen_q ",		14, GL_TEXTURE_GEN_Q },
	{ "texture_gen_r ",		14, GL_TEXTURE_GEN_R },
	{ "texture_gen_s ",		14, GL_TEXTURE_GEN_S },
	{ "texture_gen_t ",		14, GL_TEXTURE_GEN_T },
	{ "vertex_program_point_size ", 26, GL_VERTEX_PROGRAM_POINT_SIZE },
	{ "vertex_program_two_side ",	24, GL_VERTEX_PROGRAM_TWO_SIDE },
	{ NULL,				0, GL_NONE }
};

/*
static bool String2Mode(const char* str, size_t len, GLenum* mode) {
	int i;
	for (i=0; gl_enable_mode_list[i].name; i++)
		if (!strncasecmp(gl_enable_mode_list[i].name, str, len)) break;
	if (gl_enable_mode_list[i].name) {
		*mode = gl_enable_mode_list[i].mode;
		return false;
	}
	return true;
}
*/

static struct gl_enum_set_t gl_error_list[] = {
	{ "An unacceptable value is specified for an enumerated argument.", 62, GL_INVALID_ENUM },
	{ "A numeric argument is out of range.", 35, GL_INVALID_VALUE },
	{ "The specified operation is not allowed in the current state.", 60, GL_INVALID_OPERATION },
	{ "This command would cause a stack overflow.", 42, GL_STACK_OVERFLOW },
	{ "This command would cause a stack underflow.", 43, GL_STACK_UNDERFLOW },
	{ "There is not enough memory left to execute the command. State will undefined.", 77, GL_OUT_OF_MEMORY },
	{ "The specified table exceeds the implementation's maximum supported table size. Command is ignored.", 98, GL_TABLE_TOO_LARGE },
	{ "The currently bound framebuffer is not framebuffer complete.", 60, GL_INVALID_FRAMEBUFFER_OPERATION },
	{ NULL, 0, 0 }
};
	            

/*
static GLuint loadTexture(u_int8_t* pixels, u_int32_t width, u_int32_t height)
{
	GLuint textureId;
	glGenTextures(1, &textureId); //Make room for our texture
	glBindTexture(GL_TEXTURE_2D, textureId); //Tell OpenGL which texture to edit
	//Map the image to the texture
	glTexImage2D(
				 GL_TEXTURE_2D,                //Always GL_TEXTURE_2D
				 0,			    //0 for now
				 GL_RGBA,                       //Format OpenGL uses for image
				 width, height,  //Width and height
				 0,			    //The border of the image
				 GL_BGRA, //GL_RGB, because pixels are stored in RGB format
				 GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
						   //as unsigned numbers
				 pixels);               //The actual pixel data
	return textureId; //Returns the id of the texture
}
*/
	
COpenglVideo::COpenglVideo(CVideoMixer *pVideoMixer, u_int32_t max_shapes,
	u_int32_t max_placed_shapes, u_int32_t max_textures, u_int32_t max_vectors,
	u_int32_t max_vbos, int verbose) {

	m_pVideoMixer = pVideoMixer;
	if (!m_pVideoMixer) {
		fprintf(stderr, "ERROR. Class COpengelVideo was created with pVideoMixer NULL\n");
		exit(1);
	}
	m_pCtx_OSMesa		= NULL;
	m_width			= 0;
	m_height		= 0;
	m_depth_bits		= 0;
	m_verbose		= 1; //verbose;
	m_shape_count		= 0;
	m_texture_count		= 0;
	m_vector_count		= 0;
	m_vbo_count		= 0;
	m_vbo_total_count	= 0;
	m_placed_shape_count	= 0;
	m_max_shapes		= max_shapes;
	m_max_textures		= max_textures;
	m_max_vectors		= max_vectors;
	m_max_vbos		= max_vbos;
	m_max_placed_shapes	= max_placed_shapes;
	m_shapes		= (opengl_shape_t**) calloc(sizeof(opengl_shape_t*), m_max_shapes);
	m_placed_shapes	= (opengl_placed_shape_t**) calloc(sizeof(opengl_placed_shape_t*), m_max_placed_shapes);
	m_textures		= (opengl_texture_t**) calloc(sizeof(opengl_texture_t*), m_max_textures);
	m_vectors		= (opengl_vector_t**) calloc(sizeof(opengl_vector_t*), m_max_vectors);
	m_vbos			= (opengl_vbo_t**) calloc(sizeof(opengl_vbo_t*), m_max_vbos);
	if (!m_shapes) {
		fprintf(stderr, "Warning : no memory for shapes in COpenglVideo\n");
		m_max_shapes = 0;
	}
	if (!m_placed_shapes) {
		fprintf(stderr, "Warning : no memory for placed shapes in COpenglVideo\n");
		m_max_placed_shapes = 0;
	}
	if (!m_textures) {
		fprintf(stderr, "Warning : no memory for textures in COpenglVideo\n");
		m_max_textures = 0;
	}
	if (!m_vectors) {
		fprintf(stderr, "Warning : no memory for vectors in COpenglVideo\n");
		m_max_vectors = 0;
	}
	if (!m_vbos) {
		fprintf(stderr, "Warning : no memory for vbos in COpenglVideo\n");
		m_max_vbos = 0;
	}
#ifdef HAVE_GLU
	m_glu_quadrics		= NULL;
#endif
	m_current_context_is_valid = false;
#ifdef HAVE_GLX
	m_pCtx_GLX		= NULL;
	m_display		= NULL;
	m_use_glx		= true;
	m_screen		= -1;
	m_win			= 0;
	m_glx_major		= 0;
	m_glx_minor		= 0;
	m_vi			= NULL;
	m_framebuffer_draw	= 0;
	m_framebuffer_read	= 0;
#else
	m_use_glx		= false;
#endif
}

COpenglVideo::~COpenglVideo() {
fprintf(stderr, "Deleting Class COpenglVideo\n");
	if (m_pCtx_OSMesa
#ifdef HAVE_GLX
			|| m_pCtx_GLX
#endif
		) DestroyContext();

#ifdef HAVE_GLX
	if (m_display) XCloseDisplay(m_display);
	m_display = NULL;
#endif

	if (m_vbos) {
		for (u_int32_t id=0; id < m_max_vbos; id++)
			if (m_vbos[id]) DestroyIndexedVBO(id);
		free(m_vbos);
		m_vbos = NULL;
	}
	if (m_shapes) {
		for (u_int32_t id=0; id < m_max_shapes; id++)
			if (m_shapes[id]) DeleteShape(id);
		free(m_shapes);
		m_shapes = NULL;
	}
	if (m_placed_shapes) {
		for (u_int32_t id=0; id < m_max_placed_shapes; id++)
			if (m_placed_shapes[id]) DeletePlacedShape(id);
		free(m_placed_shapes);
		m_placed_shapes = NULL;
	}
	if (m_textures) {
		for (u_int32_t id=0; id < m_max_textures; id++)
			if (m_textures[id]) DeleteTexture(id);
		free(m_textures);
		m_textures = NULL;
	}
	if (m_vectors) {
		for (u_int32_t id=0; id < m_max_vectors; id++)
			if (m_vectors[id]) DeleteLight(id);
		free(m_vectors);
		m_vectors = NULL;
	}
#ifdef HAVE_GLU
	if (m_glu_quadrics) DeleteGluQuadric(m_glu_quadrics, true);	// Delete recursive
	m_glu_quadrics = NULL;	// not strictly necessary
#endif
}

int COpenglVideo::MakeIndexedVBO(const char* name, u_int32_t vbo_id) {
	if (!m_vbos) return 1;
	if (vbo_id >= m_max_vbos) return -1;
	if (m_vbos[vbo_id]) {
		DestroyVBO(m_vbos[vbo_id]);
		m_vbo_count--;
	}
	if (!(m_vbos[vbo_id] = MakeVBO(name, vbo_id))) return -1;
	m_vbo_count++;
	return 0;
}
int COpenglVideo::DestroyIndexedVBO(u_int32_t vbo_id) {
	if (!m_vbos) return 1;
	if (vbo_id >= m_max_vbos) return -1;
	if (m_vbos[vbo_id]) {
		DestroyVBO(m_vbos[vbo_id]);
		m_vbo_count--;
	} else return -1;
	return 0;
}


void COpenglVideo::DestroyVBO(opengl_vbo_t* pVbo) {
	if (!pVbo) return;
fprintf(stderr, "Destroy VBO %u\n", pVbo->id);
	if (m_verbose > 1) fprintf(stderr, "Destroying VBO %u name %s (%u remain)\n",
		pVbo->id, pVbo->name ? pVbo->name : "no name", --m_vbo_total_count);
	if (pVbo->config) free(pVbo->config);
	if (pVbo->DataBuffer_id)
		glDeleteBuffers(1, &pVbo->DataBuffer_id); //pVbo->DataBuffer_id = 0;
	if (pVbo->IndexBuffer_id)
		glDeleteBuffers(1, &pVbo->IndexBuffer_id); //pVbo->IndexBuffer_id = 0;
	if (pVbo->pData) free(pVbo->pData);
	if (pVbo->pIndices) free(pVbo->pIndices);
	if (pVbo->vao_id) {
		fprintf(stderr, "Deleting Vertex Array %u for VBO %u\n", pVbo->vao_id, pVbo->id);
		glDeleteVertexArrays(1, &pVbo->vao_id);
	}
	free(pVbo);
}

struct opengl_vbo_t* COpenglVideo::MakeVBO(const char* name, u_int32_t id) {
	if (!name || !(*name)) return NULL;
	opengl_vbo_t* pVbo = (opengl_vbo_t*) calloc(1,sizeof(opengl_vbo_t)+strlen(name)+1);
	if (!pVbo) {
		if (m_verbose) fprintf(stderr, "Failed to get memory for VBO %u name %s\n",
			id, name);
		return NULL;
	}
	pVbo->name = ((char*)pVbo)+sizeof(opengl_vbo_t);
	strcpy(pVbo->name, name);
	trim_string(pVbo->name);
	pVbo->id = id;
	pVbo->type = GL_NONE;
	pVbo->usage = GL_NONE;
	pVbo->offset_color = -1;
	pVbo->offset_normal = -1;
	pVbo->offset_texture = -1;
	pVbo->offset_vertex = -1;
	pVbo->n_color = -1;
	pVbo->n_normal = -1;
	pVbo->n_texture = -1;
	pVbo->n_vertex = -1;
	m_vbo_total_count++;
	if (m_verbose) fprintf(stderr, "VBO id %u Created <%s>\n", pVbo->id, pVbo->name);
	return pVbo;
}

int COpenglVideo::ConfigVBO(struct opengl_vbo_t* pVbo, GLenum type, const char* config, GLenum usage) {
	u_int32_t n_data;
	int i;
	const char* config_str = config;
	if (!pVbo || !config || !(*config)) return -1;

	// Check that type is valid
	for (i=0; gl_begin_list[i].name; i++) {
		if (type == gl_begin_list[i].mode) break;
	}
	if (gl_begin_list[i].mode == GL_NONE) return -1;

	if (usage != GL_STATIC_DRAW && usage != GL_DYNAMIC_DRAW && usage != GL_STREAM_DRAW) return -1;
	int offset = 0;
	int offset_color = -1, offset_normal = -1, offset_texture = -1, offset_vertex = -1;
	int n_color = -1, n_normal = -1, n_texture = -1, n_vertex = -1;
	while (*config) {
		while (isspace(*config)) config++;
		if (*config == '#' || !(*config)) break;
		if (sscanf(config+1, "%u", &n_data) != 1 || n_data < 2 || n_data > 4) return -1;
		if (*config == 'c' || *config == 'C') {
			if (n_color != -1 || (n_data != 3 && n_data != 4)) return -1;
			offset_color = offset;
			n_color = n_data;
		}
		else if (*config == 'n' || *config == 'N') {
			if (n_normal != -1 || n_data != 3) return -1;
			offset_normal = offset;
			n_normal = n_data;
		}
		else if (*config == 't' || *config == 'T') {
			if (n_texture != -1 || n_data < 1 || n_data > 4) return -1;
			offset_texture = offset;
			n_texture = n_data;
		}
		else if (*config == 'v' || *config == 'V') {
			if (n_vertex != -1 || n_data < 2 || n_data > 4) return -1;
			offset_vertex = offset;
			n_vertex = n_data;
		} else if (*config == '#') break;
		else return -1;
		offset += (sizeof(sm_opengl_parm_t) * n_data);
		config += 2;
	}
	if (n_color < 0 && n_normal < 0 && n_texture < 0 && n_vertex  < 0) return -1;
	pVbo->offset_color = offset_color;
	pVbo->offset_normal = offset_normal;
	pVbo->offset_texture = offset_texture;
	pVbo->offset_vertex = offset_vertex;
	pVbo->n_color = n_color;
	pVbo->n_normal = n_normal;
	pVbo->n_texture = n_texture;
	pVbo->n_vertex = n_vertex;
	pVbo->n_per_set = (n_color > 0 ? n_color : 0) + (n_normal > 0 ? n_normal : 0) +
		(n_texture > 0 ? n_texture : 0) + (n_vertex > 0 ? n_vertex : 0);
	pVbo->usage = usage;
	pVbo->type = type;
	pVbo->config = strdup(config_str);;
	if (m_verbose) fprintf(stderr, "VBO Configured color(%d,%d) normal(%d,%d) "
		"texture(%d,%d) vertex(%d,%d)\n", pVbo->n_color, pVbo->offset_color,
		pVbo->n_normal, pVbo->offset_normal, pVbo->n_texture, pVbo->offset_texture,
		pVbo->n_vertex, pVbo->offset_vertex);
	return 0;
}

// Add data points to VBO. n is number of data, n < 1 deletes data
// pData must point to an array of sm_opengl_parm_t with n elements
int COpenglVideo::AddDataPointsVBO(opengl_vbo_t* pVbo, int n, sm_opengl_parm_t* pData) {
	if (!pVbo) return -1;
	if (n <= 0) {
if (m_verbose) fprintf(stderr, "VBO vbo_id %u name <%s> data deleted\n", pVbo->id, pVbo->name ? pVbo->name : "");
		pVbo->n_data = 0;
		pVbo->initialized = false;
		// Delete data points if any
		if (!pVbo->pData) return -1;
		free (pVbo->pData);
		pVbo->pData = NULL;
		return 0;
	}
	if (!pData) return -1;
	// Just in case.
	if (!pVbo->pData) pVbo->n_data = 0;		// should not be necessary.

	// Allocate data
	sm_opengl_parm_t* pBlock = (sm_opengl_parm_t*) malloc(sizeof(sm_opengl_parm_t)*(n+pVbo->n_data));
	if (!pBlock) return -1;

	if (pVbo->pData) {
		// Move old data to new block
		memcpy(pBlock, pVbo->pData, sizeof(sm_opengl_parm_t)*pVbo->n_data);
		free(pVbo->pData);				// Free old block
		pVbo->pData = pBlock;				// Save new block
		pBlock = pVbo->pData + pVbo->n_data;		// point to free space in new block
	} else pVbo->pData = pBlock;
	memcpy(pBlock, pData, sizeof(sm_opengl_parm_t)*n);
	pVbo->n_data += n;
	if (m_verbose) fprintf(stderr, "VBO vbo_id %u adding %d data points. Total %u. Sets %d\n", pVbo->id, n, pVbo->n_data,
		pVbo->n_per_set ? pVbo->n_data/pVbo->n_per_set : -1);
	pVbo->initialized = false;
	return 0;
}

// Add Indices to VBO. n is number of indices, n < 1 deletes indices
// pIndices must point to an array of shorts with n elements
int COpenglVideo::AddIndicesVBO(opengl_vbo_t* pVbo, int n, short* pIndices) {
	if (!pVbo || !pVbo->pData) return -1;
	if (n <= 0) {
if (m_verbose) fprintf(stderr, "VBO vbo_id %u indices deleted\n", pVbo->id);
		if (!pVbo->pIndices) return -1;
		free (pVbo->pIndices);
		pVbo->pIndices = NULL;
		pVbo->n_indices = 0;
		pVbo->initialized = false;
		return 0;
	}
	if (!pIndices) return -1;

	// We need to run through the indices to check we are not indexing something we
	// dont have data for.
	if (!pVbo->n_per_set) {
		fprintf(stderr, "Can not add indices to VBO. n_per_set == 0\n");
		return -1;
	}
	short max_index = (pVbo->n_data / pVbo->n_per_set) -1;
	if (m_verbose) fprintf(stderr, "VBO vbo_id %u : Max index allowed is %hd\n", pVbo->id, max_index);
	for (int i=0 ; i < n; i++)
		if (pIndices[i] > max_index) {
			fprintf(stderr, "VBO : index %d is %hd. Max data sets is %hd\n",
				i, pIndices[i], max_index);
			return -1;
	}

	// Just in case.
	if (!pVbo->pIndices) pVbo->n_indices = 0;		// should not be necessary.

	// Allocate data
	short* pBlock = (short*) malloc(sizeof(short)*(n+pVbo->n_indices));
	if (!pBlock) return -1;

	if (pVbo->pIndices) {
		// Move old data to new block
		memcpy(pBlock, pVbo->pIndices, sizeof(short)*pVbo->n_indices);
		free(pVbo->pIndices);				// Free old block
		pVbo->pIndices = pBlock;			// Save new block
		pBlock = pVbo->pIndices + pVbo->n_indices;	// point to free space in new block
	} else pVbo->pIndices = pBlock;
	memcpy(pBlock, pIndices, sizeof(short)*n);
	pVbo->n_indices += n;
	
	if (m_verbose > 1) {
		fprintf(stderr, "VBO vbo_id %u adding %d indices. Total %u.\n - VBO Indices = ",
			pVbo->id, n, pVbo->n_indices);
		for (unsigned int i = 0; i < pVbo->n_indices ; i++)
			fprintf(stderr, " %hd", pVbo->pIndices[i]);
		fprintf(stderr, "\n");
	}
	pVbo->initialized = false;
	return 0;
}


#ifdef HAVE_GLU
GLUquadric* COpenglVideo::CreateGluQuadric(u_int32_t id) {
	GLUquadric* p = FindGluQuadric(id);
	if (p) return p;

	// None found. We will create one
	glu_quadric_list_t* item = (glu_quadric_list_t*) calloc(sizeof(glu_quadric_list_t),1);
	if (!item) return NULL;
	item->id = id;
	if (!(item->pQuad = gluNewQuadric())) {
		fprintf(stderr, "Failed to create new GLU_Quadric\n");
		free (item);
		return NULL;
	}
	if (!m_glu_quadrics) {
		m_glu_quadrics = item;
		return item->pQuad;
	}
	// We need to insert it in list sorted
	glu_quadric_list_t* list = m_glu_quadrics;
	if (list->id > item->id) {
		item->next = m_glu_quadrics;
		m_glu_quadrics = item;
	} else while (list) {
		if (!list->next) {
			list->next = item;
			return item->pQuad;
		}
		if (list->next->id > item->id) {
			item->next = list->next;
			list->next = item;
		}
	}
	return item->pQuad;
}

GLUquadric* COpenglVideo::FindGluQuadric(u_int32_t id) {
	if (!m_glu_quadrics) return NULL;
	glu_quadric_list_t* p = m_glu_quadrics;
	while (p) {
		if (id == p->id) break;
		if (id < p->id) { p = NULL; break; }
		p = p->next;
	}
	return p ? p->pQuad : NULL;
}

void COpenglVideo::DeleteGluQuadric(glu_quadric_list_t* p, bool recursive) {
	if (!p) return;
	if (recursive) DeleteGluQuadric(p->next, true);
	if (p->pQuad) gluDeleteQuadric(p->pQuad);
	free(p);
}
#endif // #ifdef HAVE_GLU

int COpenglVideo::DeleteShape(u_int32_t id) {
	if (!m_shapes || id >= m_max_shapes || !m_shapes[id]) return -1;

	// Name is allocated as part of the shape, but the draw operations
	// are allocated individually and must be freed as such
	opengl_draw_operation_t* p = m_shapes[id]->pDraw;
	while (p) {
		if (p->create_str) {
			free(p->create_str);
			p->create_str = NULL;	// Not strictly needed
		}
		if (p->data) {
			free(p->data);
			p->data = NULL;		// Not strictly needed
		}
		opengl_draw_operation_t* next = p->next;
		free(p);
		p = next;
	}
	m_shapes[id]->pDraw = NULL;	// Not strictly necessary
	free(m_shapes[id]);
	m_shapes[id] = NULL;
	m_shape_count--;
	return 0;
}

int COpenglVideo::DeleteLight(u_int32_t id) {
	if (!m_vectors || id >= m_max_vectors || !m_vectors[id]) return -1;

	// Name is allocated as part of the shape
	free(m_vectors[id]);
	m_vectors[id] = NULL;
	m_vector_count--;
	return 0;
}

int COpenglVideo::DeleteTexture(u_int32_t id) {
	if (!m_textures || id >= m_max_textures || !m_textures[id]) return -1;

	// Name is allocated as part of the shape
	if (m_textures[id]->real_texture_id)
		glDeleteTextures(1, &(m_textures[id]->real_texture_id));
	free(m_textures[id]);
	m_textures[id] = NULL;
	m_texture_count--;
	return 0;
}

int COpenglVideo::DeletePlacedShape(u_int32_t id) {
	if (!m_placed_shapes || id >= m_max_placed_shapes || !m_placed_shapes[id]) return -1;
	if (m_placed_shapes[id]->name) free(m_placed_shapes[id]->name);
	m_placed_shapes[id]->name = NULL;
	free(m_placed_shapes[id]);
	m_placed_shapes[id] = NULL;
	m_placed_shape_count--;
	return 0;
}

// glshape
// Returns: 0=OK, 1=system error -1=parameter/syntax error
int COpenglVideo::ParseCommand(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;

	// Commands used regularly or not expected for setup
	if (!strncasecmp (str, "overlay ", 8)) {
		return overlay_placed_shape(ctr, str+8);
	}
	else if (!strncasecmp (str, "modify ", 7)) {
		return set_shape_enum_or_int_parameters(ctr, str+7, SM_GL_MODIFY);
	}
	else if (!strncasecmp (str, "entry ", 6)) {
		return set_shape_enum_or_int_parameters(ctr, str+6, SM_GL_ENTRY);
	} else if (!strncasecmp (str, "copyback ", 9)) {
		return set_copy_back(ctr, str+9);
	} else if (!strncasecmp (str, "place ", 6)) {
		str += 6;
		while (isspace(*str)) str++;

		// glshape place alpha [<place id> [<alpha>]]
		// glshape place rotation [<place id> [<rotation> <rx> <ry> <rz>]]
		// glshape place rgb [<place id> [<red> <green> <blue> [<alpha>]]]
		// glshape place scale [<place id> [<scale x> <scale y> <scale z>]]
		// glshape place [<place id> [<shape id> <x> <y> <z> [<scale x> <scale y> <scale z> [<rotation> <rx> <ry> <rz> [<red> <green> <blue> [<alpha>]]]]]]

		if (!strncasecmp (str, "alpha ", 6))
			return set_shape_rgb(ctr, str+6, false);
		else if (!strncasecmp (str, "coor ", 5))
			return set_shape_coor(ctr, str+5);
		else if (!strncasecmp (str, "rotation ", 9))
			return set_shape_rotation(ctr, str+9);
		else if (!strncasecmp (str, "rgb ", 4))
			return set_shape_rgb(ctr, str+4);
		else if (!strncasecmp (str, "scale ", 6))
			return set_shape_scale(ctr, str+6);
		else if (!strncasecmp (str, "help ", 5))
			return list_glshape_help(ctr, "place ");
		else return set_shape_place(ctr, str);
	} else if (!strncasecmp (str, "moveentry ", 10)) {
		return set_shape_moveentry(ctr, str+10);
	} else if (!strncasecmp (str, "list ", 5)) {
		return set_shape_list(ctr, str+5);
	} else if (!strncasecmp (str, "help ", 5)) {
		return list_glshape_help(ctr, str+5);

	// Commands used for setup although they can also be used later to change setup
	// glshape add [<shape id> [<name>]]
	} else if (!strncasecmp (str, "add ", 4)) {
		return set_shape_add(ctr, str+4);
	} else if (!strncasecmp (str, "begin ", 6)) {
		return set_shape_enum_or_int_parameters(ctr, str+6, SM_GL_BEGIN);
	} else if (!strncasecmp (str, "inshape ", 8)) {
		return set_shape_enum_or_int_parameters(ctr, str+8, SM_GL_GLSHAPE);
	} else if (!strncasecmp (str, "enable ", 7)) {
		return set_shape_enum_or_int_parameters(ctr, str+7, SM_GL_ENABLE);
	} else if (!strncasecmp (str, "disable ", 8)) {
		return set_shape_enum_or_int_parameters(ctr, str+8, SM_GL_DISABLE);
	} else if (!strncasecmp (str, "translate ", 10)) {
		return set_shape_nummerical_parameters(ctr, str+10, SM_GL_TRANSLATE);
	} else if (!strncasecmp (str, "scale ", 6)) {
		return set_shape_nummerical_parameters(ctr, str+6, SM_GL_SCALE);
	} else if (!strncasecmp (str, "clearcolor ", 11)) {
		return set_shape_enum_or_int_parameters(ctr, str+11, SM_GL_CLEARCOLOR);
	} else if (!strncasecmp (str, "color", 5)) {
		str += 5;
		if (*str == ' ' || *str == '3' || *str == '4')
			return set_shape_nummerical_parameters(ctr, str+1, SM_GL_COLOR);
		else return -1;
	} else if (!strncasecmp (str, "rotate ", 7)) {
		return set_shape_nummerical_parameters(ctr, str+7, SM_GL_ROTATE);
	} else if (!strncasecmp (str, "vertex ", 7)) {
		return set_shape_nummerical_parameters(ctr, str+7, SM_GL_VERTEX);
	} else if (!strncasecmp (str, "normal ", 7)) {
		return set_shape_nummerical_parameters(ctr, str+7, SM_GL_NORMAL);

#ifdef HAVE_GLU
	} else if (!strncasecmp (str, "glu", 3)) {
		str += 3;
		if (!strncasecmp (str, "cylinder ", 9)) {
			return set_shape_enum_or_int_parameters(ctr, str+9, SM_GLU_CYLINDER);
		} else if (!strncasecmp (str, "disk ", 5)) {
			return set_shape_enum_or_int_parameters(ctr, str+5, SM_GLU_DISK);
		} else if (!strncasecmp (str, "drawstyle ", 10)) {
			return set_shape_enum_or_int_parameters(ctr, str+10, SM_GLU_DRAWSTYLE);
		} else if (!strncasecmp (str, "normals ", 8)) {
			return set_shape_enum_or_int_parameters(ctr, str+8, SM_GLU_NORMALS);
		} else if (!strncasecmp (str, "partialdisk ", 12)) {
			return set_shape_enum_or_int_parameters(ctr, str+12, SM_GLU_PARTIALDISK);
		} else if (!strncasecmp (str, "perspective ", 12)) {
			return set_shape_enum_or_int_parameters(ctr, str+12, SM_GLU_PERSPECTIVE);
		} else if (!strncasecmp (str, "orientation ", 12)) {
			return set_shape_enum_or_int_parameters(ctr, str+12, SM_GLU_ORIENTATION);
		} else if (!strncasecmp (str, "sphere ", 7)) {
			return set_shape_enum_or_int_parameters(ctr, str+7, SM_GLU_SPHERE);
		} else if (!strncasecmp (str, "texture ", 8)) {
			return set_shape_enum_or_int_parameters(ctr, str+8, SM_GLU_TEXTURE);
#ifdef HAVE_GLUT
		} else if (!strncasecmp (str, "tteapot ", 8)) {
			return set_shape_enum_or_int_parameters(ctr, str+8, SM_GLUT_TEAPOT);
#endif
		} else return -1;
#endif // #ifdef HAVE_GLU
	} else if (!strncasecmp (str, "texcoord ", 9)) {
		return set_shape_nummerical_parameters(ctr, str+9, SM_GL_TEXCOORD);
	} else if (!strncasecmp (str, "recursion ", 10)) {
		return set_shape_enum_or_int_parameters(ctr, str+10, SM_GL_RECURSION);
	} else if (!strncasecmp (str, "matrixmode ", 11)) {
		return set_shape_enum_or_int_parameters(ctr, str+11, SM_GL_MATRIXMODE);
	} else if (!strncasecmp (str, "clear ", 6)) {
		return set_shape_enum_or_int_parameters(ctr, str+6, SM_GL_CLEAR);
	} else if (!strncasecmp (str, "end ", 4)) {
		return set_shape_no_parameter(ctr, str+4, SM_GL_END);
	} else if (!strncasecmp (str, "finish ", 6)) {
		return set_shape_no_parameter(ctr, str+6, SM_GL_FINISH);
	} else if (!strncasecmp (str, "flush ", 6)) {
		return set_shape_no_parameter(ctr, str+6, SM_GL_FLUSH);
	} else if (!strncasecmp (str, "loadidentity ", 13)) {
		return set_shape_no_parameter(ctr, str+13, SM_GL_LOADIDENTITY);
	} else if (!strncasecmp (str, "pushmatrix ", 11)) {
		return set_shape_no_parameter(ctr, str+11, SM_GL_PUSHMATRIX);
	} else if (!strncasecmp (str, "popmatrix ", 10)) {
		return set_shape_no_parameter(ctr, str+10, SM_GL_POPMATRIX);
//	} else if (!strncasecmp (str, "matrixmode ", 11)) {
//		return set_shape_enum_or_int_parameters(ctr, str+11, SM_GL_MATRIXMODE);
	} else if (!strncasecmp (str, "texfilter2d ", 12)) {
		return set_shape_enum_or_int_parameters(ctr, str+12, SM_GL_TEXFILTER2D);
	} else if (!strncasecmp (str, "blendfunc ", 10)) {
		return set_shape_enum_or_int_parameters(ctr, str+10, SM_GL_BLENDFUNC);
	} else if (!strncasecmp (str, "viewport ", 9)) {
		return set_shape_enum_or_int_parameters(ctr, str+9, SM_GL_VIEWPORT);
	} else if (!strncasecmp (str, "scissor ", 8)) {
		return set_shape_enum_or_int_parameters(ctr, str+8, SM_GL_SCISSOR);
	} else if (!strncasecmp (str, "snowmix ", 8)) {
		return set_shape_enum_or_int_parameters(ctr, str+8, SM_GL_SNOWMIX);

	// glshabe vbo add <vbo id> <name>
	// glshabe vbo config <vbo id> [(static|dynamic|stream) (quads|triangles)
	//		(c3 | c4 | n3 | t1 | t2 | t3 | t4 | v2 | v3 | v4) ...]
	// glshabe vbo data <vbo id> [<data 0> <data 1> ...]
	// glshabe vbo indices <vbo id> [<index 0> <index 1> ...]
	// glshabe vbo <glshape id> <vbo id>
	} else if (!strncasecmp (str, "vbo ", 4)) {
			str += 4;
			while (isspace(*str)) str++;
			if (!strncasecmp (str, "add ", 4))
				return set_vbo_add(ctr, str+4);
			else if (!strncasecmp (str, "config ", 7))
				return set_vbo_config(ctr, str+7);
			else if (!strncasecmp (str, "data ", 5))
				return set_vbo_data(ctr, str+5);
			else if (!strncasecmp (str, "indices ", 8))
				return set_vbo_indices(ctr, str+8);
			else if (isdigit(*str))
				return set_shape_enum_or_int_parameters(ctr, str, SM_GL_VBO);

			else return -1;
	} else if (!strncasecmp (str, "if ", 3)) {
		str += 3;
		while (isspace(*str)) str++;
		const char* str2 = str;
#ifdef HAVE_GLX
		if (!strncasecmp (str, "glx ", 4)) {
			str += 4; while (isspace(*str)) str++;
			if (isdigit(*str))
				return set_shape_enum_or_int_parameters(ctr, str, SM_GL_IFGLX);
		} else
#endif	// HAVE_GLX
#ifdef HAVE_OSMESA
			if (!strncasecmp (str, "osmesa ", 7)) {
			str += 7; while (isspace(*str)) str++;
			if (isdigit(*str))
				return set_shape_enum_or_int_parameters(ctr, str, SM_GL_IFOSMESA);
		} else
#endif // HAVE_OSMESA
		return -1;

		if (!strncasecmp (str2, "osmesa ", 7)) {
			if (!m_pCtx_OSMesa || m_use_glx) return 0;
		} else if (!strncasecmp (str2, "glx ", 4)) {
#ifdef HAVE_GLX
			if (!m_pCtx_GLX || !m_use_glx)
#endif
				return 0;
		} else return -1;
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		char* command = strdup(str);
		if (!command) return 1;
		m_pVideoMixer->m_pController->controller_parse_command(
			m_pVideoMixer, ctr, command);
		free(command);
		return 0;
/*
	} else if (!strncasecmp (str, "glTexParameteri ", 16)) {
		return set_shape_enum_or_int_parameters(ctr, str+16, SM_GL_TEXPARAMETERI);
	} else if (!strncasecmp (str, "glTexParameterf ", 16)) {
		return set_shape_enum_or_int_parameters(ctr, str+16, SM_GL_TEXPARAMETERF);
*/
	} else if (!strncasecmp (str, "ortho ", 6)) {
		return set_shape_enum_or_int_parameters(ctr, str+6, SM_GL_ORTHO);
	} else if (!strncasecmp (str, "materialv ", 10)) {
		return set_shape_enum_or_int_parameters(ctr, str+10, SM_GL_MATERIALV);
	} else if (!strncasecmp (str, "shademodel ", 11)) {
		return set_shape_enum_or_int_parameters(ctr, str+11, SM_GL_SHADEMODEL);
	} else if (!strncasecmp (str, "arc", 3)) {
		str += 3;
		if (!strncasecmp (str, "xz ", 3))
			return set_shape_enum_or_int_parameters(ctr, str+3, SM_GL_ARCXZ);
		else if (!strncasecmp (str, "yz ", 3))
			return set_shape_enum_or_int_parameters(ctr, str+3, SM_GL_ARCYZ);
		else if (!strncasecmp (str, "xy ", 3))
			return set_shape_enum_or_int_parameters(ctr, str+3, SM_GL_ARCXY);
		else return -1;

	// glshape vector add [<vector id> [<light name>]]
	// glshape vector value [<vector id> <a> <b> <c> <d>]
	} else if (!strncasecmp (str, "vector ", 7)) {
		str += 7;
		while (isspace(*str)) str++;
		if (!strncasecmp (str, "add ", 4)) {
			return set_vector_add(ctr, str+4);
		}
		else if (!strncasecmp (str, "value ", 6)) {
			return set_vector_value(ctr, str+6);
		} else return -1;

	// glshape light <shape id> light<gllight i> <pname> <value>
	// glshape lightv <shape id> light<gllight i> <pname> <vector id>
	} else if (!strncasecmp (str, "light", 5)) {
		str += 5;
		if (!(strncasecmp(str, "v ", 2)))
			return set_shape_enum_or_int_parameters(ctr, str+2, SM_GL_LIGHTV);
		else if (isspace(*str))
			return set_shape_enum_or_int_parameters(ctr, str+1, SM_GL_LIGHT);
		else return -1;

	// glshape texture add [<texture id> [<texture name>]]
	// glshape texture source [<texture id> (feed <feed id>|image <image id>)]
	// glshape texture bind <glshape> <texture id> [<min> <mag>] [2d | cube]
	} else if (!strncasecmp (str, "texture ", 8)) {
		str += 8;
		while (isspace(*str)) str++;
		if (!strncasecmp (str, "bind ", 5)) {
			return set_shape_enum_or_int_parameters(ctr, str+5, SM_GL_BINDTEXTURE);
			//return set_texture_bind(ctr, str+5);
		}
		else if (!strncasecmp (str, "source ", 6)) {
			return set_texture_source(ctr, str+6);
		}
		else if (!strncasecmp (str, "add ", 4)) {
			return set_texture_add(ctr, str+4);
		} else return -1;
	} else if (!strncasecmp (str, "context ", 8)) {
		return set_preferred_context(ctr, str+8);
	} else if (!strncasecmp (str, "info ", 5)) {
		return list_shape_info(ctr, str+5);
	} else if (!strncasecmp (str, "verbose ", 8)) {
		return set_verbose(ctr, str+8);
	} else if (!strncasecmp (str, "noop ", 5)) {
		return set_shape_no_parameter(ctr, str+5, SM_GL_NOOP);
	}
	return -1;
}

// glshape verbose [<level>] // set control channel to write out
int COpenglVideo::set_verbose(struct controller_type* ctr, const char* str)
{
	int n;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_verbose) m_verbose = 0;
		else m_verbose = 1;
		if (m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"glshape verbose %s\n", m_verbose ? "on" : "off");
		return 0;
	}
	while (isspace(*str)) str++;
	n = sscanf(str, "%u", &m_verbose);
	if (n != 1) return -1;
	if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape verbose level set to %u for class COpenglVideo\n",
			m_verbose);
	return 0;
}

int COpenglVideo::list_shape_info(struct controller_type* ctr, const char* str)
{
	GLuint vao_id;
	if (!ctr || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	const char* vendor = (const char*) glGetString(GL_VENDOR);
	const char* render = (const char*) glGetString(GL_RENDERER);
	const char* version = (const char*) glGetString(GL_VERSION);
	const char* shader = (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
	glGenVertexArrays(1,&vao_id);
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS" glshape info\n"
		STATUS" verbose level   : %u\n"
		STATUS" glshapes        : %u of %u\n"
		STATUS" placed glshapes : %u of %u\n"
		STATUS" textures        : %u of %u\n"
		STATUS" vectors         : %u of %u\n"
		STATUS" vbos            : %u of %u and %u non indexed\n"
		STATUS" vao support     : %s\n"
		STATUS" context         : %s (%s preferred)\n"
		STATUS" geometry        : %ux%u depth %u\n"
		STATUS" OpenGL vendor   : %s\n"
		STATUS" OpenGL renderer : %s\n"
		STATUS" OpenGL Version  : %s\n"
		STATUS" OpenGL Shading  : %s\n"
		STATUS" Supporting      :"
#ifdef HAVE_OSMESA
					  " OSMesa"
#endif
#ifdef HAVE_GLX
					  " GLX"
#endif
#ifdef HAVE_GLU
					  " GLU"
#endif
#ifdef HAVE_GLU
					  " GLUT"
#endif
					  "\n"
		STATUS" glshapes : ops_count shape_name\n",
		m_verbose,
		m_shape_count, m_max_shapes,
		m_placed_shape_count, m_max_placed_shapes,
		m_texture_count, m_max_textures,
		m_vector_count, m_max_vectors,
		m_vbo_count, m_max_vbos, m_vbo_total_count - m_vbo_count,
		vao_id ? "yes" : "no",
		m_pCtx_OSMesa && m_pCtx_GLX ? "osmesa and glx" :
		  m_pCtx_OSMesa ? "osmesa" :
		    m_pCtx_GLX ? "glx" : "none",
		m_use_glx ? "glx" : "osmesa",
		m_width, m_height, m_depth_bits,
		vendor ? vendor : "none",
		render ? render : "none",
		version ? version : "none",
		shader ? shader : "none"
		);
	if (m_shapes) for (u_int32_t id=0 ; id < m_max_shapes; id++) if (m_shapes[id])
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS" - shape %u : %u %s\n",
			id, m_shapes[id]->ops_count, m_shapes[id]->name);
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	if (vao_id) glDeleteVertexArrays(1, &vao_id);
	return 0;
}


// glshabe vbo add <vbo id> <name>
int COpenglVideo::set_vbo_add(struct controller_type* ctr, const char* str) {
	if (!str || !m_vbos) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		for (unsigned int id = 0; id < m_max_vbos; id++)
			if (m_vbos[id]) m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS" vbo %u %s name %s\n",
					id, m_vbos[id]->initialized ? "ready" : "setup", m_vbos[id]->name);
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"\n");
		return 0;
	}
	u_int32_t vbo_id;
	if (sscanf(str, "%u", &vbo_id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return DestroyIndexedVBO(vbo_id);
	int n = MakeIndexedVBO(str, vbo_id);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"glshape vbo %u %s added\n", vbo_id, str);
	return 0;
}


// glshabe vbo indices <vbo id> [<index 0> <index 1> ...]
int COpenglVideo::set_vbo_indices(struct controller_type* ctr, const char* str) {
	if (!str || !m_vbos) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		for (unsigned int id = 0; id < m_max_vbos; id++)
			if (m_vbos[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"vbo %u indices :", id);
				short* pIndices = m_vbos[id]->pIndices;
				if (m_vbos[id]->n_indices)
					for (unsigned int i=0 ; i < m_vbos[id]->n_indices; i++) {
						m_pVideoMixer->m_pController->controller_write_msg (ctr,
							" %hd", *pIndices++);
				}
				m_pVideoMixer->m_pController->controller_write_msg(ctr, "\n");
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get VBO id
	u_int32_t vbo_id;
	short pIndices[32];
	if (sscanf(str, "%u", &vbo_id) != 1 || vbo_id >= m_max_vbos || !m_vbos[vbo_id]) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str) || *str == '#') return -1;
	int indices = 0;
	while (*str) {
		for (indices = 0; indices < 32; indices++) {
			if (sscanf(str, "%hd", &pIndices[indices]) != 1 || pIndices[indices] < 0) return -1;
			while (*str && !isspace(*str)) str++;
			while (isspace(*str)) str++;
			if (!(*str) || *str == '#') break;
		}
		if (AddIndicesVBO(m_vbos[vbo_id], indices+1, pIndices)) return -1;
		indices = 0;
		if (*str == '#') break;
	}
	return 0;
}

// glshabe vbo data <vbo id> [<data 0> <data 1> ...]
int COpenglVideo::set_vbo_data(struct controller_type* ctr, const char* str) {
	if (!str || !m_vbos) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		for (unsigned int id = 0; id < m_max_vbos; id++)
			if (m_vbos[id]) {
				sm_opengl_parm_t* pData = m_vbos[id]->pData;
				if (!m_vbos[id]->n_data)
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS"vbo %u data :\n", id);
				else for (unsigned int i=0 ;
					i < m_vbos[id]->n_data/m_vbos[id]->n_per_set; i++) {
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS"vbo %u data : ", id);
					for (int j=0 ; j < m_vbos[id]->n_per_set; j++)
						m_pVideoMixer->m_pController->controller_write_msg(ctr,
							SM_OPENGL_PARM_PRINT" ", *pData++);
					m_pVideoMixer->m_pController->controller_write_msg(ctr, "\n");
				}
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	// Get VBO id
	u_int32_t vbo_id;
	if (sscanf(str, "%u", &vbo_id) != 1 || vbo_id >= m_max_vbos || !m_vbos[vbo_id]) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str) || *str == '#') return -1;
	int j = 10;
	sm_opengl_parm_t* p, *pData = (sm_opengl_parm_t*)
		malloc(j*sizeof(sm_opengl_parm_t)*m_vbos[vbo_id]->n_per_set);
	if (!pData) return 1;
	int datapoints = 0;
	p = pData;
	while (*str) {
		for (int i = 0; i < m_vbos[vbo_id]->n_per_set; i++) {
			if (sscanf(str, SM_OPENGL_PARM, p++) != 1) {
				free(pData);
				return -1;
			}
			while (*str && !isspace(*str)) str++;
			while (isspace(*str)) str++;
		}
		datapoints += m_vbos[vbo_id]->n_per_set;
		if (!(*str) || *str == '#' || datapoints == j*m_vbos[vbo_id]->n_per_set) {
			if (AddDataPointsVBO(m_vbos[vbo_id], datapoints, pData)) {
				free(pData);
				return -1;
			}
			datapoints = 0;
			p = pData;
		}
		if (*str == '#') break;
	}
	if (pData) free(pData);
	return 0;
}

// glshabe vbo config <vbo id> [(static|dynamic|stream) (quads|triangles)
//		(c3 | c4 | n3 | t1 | t2 | t3 | t4 | v2 | v3 | v4) ...]
int COpenglVideo::set_vbo_config(struct controller_type* ctr, const char* str) {
	if (!str || !m_vbos) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		for (unsigned int id = 0; id < m_max_vbos; id++)
			if (m_vbos[id]) m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS" vbo %u %s %s data %u indices %u per set %u config %s\n",
					id,
					m_vbos[id]->type == GL_TRIANGLES ? "triangles" :
					  m_vbos[id]->type == GL_QUADS ? "quads" : "none",
					m_vbos[id]->usage == GL_STATIC_DRAW ? "static" :
					  m_vbos[id]->usage == GL_DYNAMIC_DRAW ? "dynamic" :
					    m_vbos[id]->usage == GL_STREAM_DRAW ? "stream" :
					    m_vbos[id]->usage == GL_NONE ? "uninitalized" :
					      "unknown",
					m_vbos[id]->n_data, m_vbos[id]->n_indices, 
					m_vbos[id]->n_per_set, 
					m_vbos[id]->config ? m_vbos[id]->config : "none") ;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"\n");
		return 0;
	}
	int i;
	u_int32_t vbo_id;
	const char* create_str;
	GLenum usage = GL_NONE, type = GL_NONE;

	// Get VBO id
	if (sscanf(str, "%u", &vbo_id) != 1 || vbo_id >= m_max_vbos || !m_vbos[vbo_id]) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	create_str = str;

	// Get usage
	if (!(strncasecmp(str, "dynamic ",8))) {
		usage = GL_DYNAMIC_DRAW;
		str += 8;
	} else if (!(strncasecmp(str, "stream ",7))) {
		usage = GL_STREAM_DRAW;
		str += 7;
	} else if (!(strncasecmp(str, "static ",7))) {
		usage = GL_STATIC_DRAW;
		str += 7;
	} else return -1;

	// Get type
	for (i=0; gl_begin_list[i].name; i++) {
		if (!(strncasecmp(str, gl_begin_list[i].name, gl_begin_list[i].len))) break;
	}
	if ((type = gl_begin_list[i].mode) == GL_NONE) return -1;
	while (*str && !isspace(*str)) str++;
	while (isspace(*str)) str++;

	if (ConfigVBO(m_vbos[vbo_id], type, str, usage)) return -1;

	if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE" glshape vbo config %u %s\n", vbo_id, create_str);
	return 0;
}

// glshabe vbo data <vbo id> [<data 0> <data 1> ...]
// glshabe vbo indices <vbo id> [<index 0> <index 1> ...]
// glshabe vbo <vbo id>

// glshape copyback [frame | image <image_id> <x> <y> <width> <height>]
int COpenglVideo::set_copy_back(struct controller_type* ctr, const char* str)
{
	if (!str || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	if (!m_pVideoMixer->m_overlay) return -1;
	while (isspace(*str)) str++;

	// Now we check if we copy from OpenGL to frame
	if (!(*str) || *str == '#' || !strncasecmp(str, "frame ", 6)) {
		return CopyBack(m_pVideoMixer->m_overlay);
	}

	// Now we check if we copy to an image
	u_int32_t image_id, x, y, width, height;
	u_int8_t* pImage;
	int n;
	if (strncasecmp(str, "image ", 6)) return -1;
	if (!m_pVideoMixer->m_pVideoImage) return -1;
	str += 6;
	if ((sscanf(str, "%u %u %u %u %u", &image_id, &x, &y, &width, &height) != 5 ||
		!width || !height ||
		image_id >= m_pVideoMixer->m_pVideoImage->MaxImages())) return -1;
	if (!(pImage = (u_int8_t*) calloc(4*width, height))) {
		if (m_verbose) fprintf(stderr, "Failed to allocate memory for copyback to image\n");
		return 1;
	}
	n = m_pVideoMixer->m_pVideoImage->CreateImage(image_id, pImage, width, height, "");
	if (n) {
		if (m_verbose) fprintf(stderr, "Failed to create image %u\n", image_id);
		free(pImage);
		return n;
	}
	return CopyBack(pImage, x, y, width, height);
}

/*
void InsertBalanced(int start, int end) {
	if (start > end) return;
	if (start == end) {
		Insert(start);
		return;
	}
	int half = (start+end)/2;
	if (half > start) {
		Insert(half);
		InsertBalanced(start,half-1);
	} else insert(start);
	half++;
	if (half < end) InsertBalanced(half,end);
	else Insert(end);
}
*/

struct glshape_command_list_t { const char* name; size_t len; const char* options; };

/*
//curve angle aspect slices left right top bottom
static struct glshape_command_list_t glshape_command_list[] = {
	{ "add ",	 4, "[<glshape id> [<shape name>]]"			},
	{ "arcxz ",	 6, "<glshape id> <angle> <aspect> <slices> <texleft> <texright> <textop> <texbottom>\n"			},
	{ "begin ",	 6, "<glshape id> <form>"				},
	{ "blendfunc ",	10, "<glshape id> <s factor> <d factor>"		},
	{ "clear ",	 6, "<glshape id> (depth | color | color+depth)"	},
	{ "clearcolor ",11, "<glshape id> <red> <green> <blue> [<alpha>]"	},
	{ "color ",	 6, "<glshape id> <red> <green> <blue> [<alpha>]"	},
	{ "context ",	 8, "(osmesa|glx|auto)"					},
	{ "disable ",	 8, "<glshape id> <mode>"				},
	{ "enable ",	 7, "<glshape id> <mode>"				},
	{ "end ",	 4, "<glshape id>"					},
	{ "entry ",	 0, "<glshape id> (active|inactivate|number)"		},
	{ "finish ",	 0, "<glshape id>"					},
	{ "flush ",	 0, "<glshape id>"						},
#ifdef HAVE_GLU
	{ "glucylinder ", 0, "<glshape id> <quad id> <base> <top> <height> <slices> <stacks>"	},
	{ "gludisk ",	 0, "<glshape id> <quad id> <inner> <outer> <slices> <loops>"	},
	{ "gludrawstyle ", 0, "<glshape id> <quad id> (fill|line|point|silhouette)"	},
	{ "glunormals ", 0, "<glshape id> <quad id> (none|flat|smooth)"			},
	{ "gluorientation ",	 0, "<glshape id> <quad id> (outside|inside)"		},
	{ "glupartialdisk ",	 0, "<glshape id> <quad id> <inner> <outer> <slices> <loops> <start> <sweep>"	},
	{ "gluperspective ",	 0, "<glshape id> <fovy> <aspect> <znear> <zfar>"	},
	{ "glusphere ",	 0, "<glshape id> <quad id> <radius> <slices> <stacks>"		},
	{ "glutexture ", 0, "<glshape id> <quad id> (0|1)"				},
#endif // #ifdef HAVE_GLU
	{ "info ",	 0, ""								},
	{ "if glx ",	 0, "<glshape id> <glshape command>"				},
	{ "if osmesa ",	 0, "<glshape id> <glshape command>"				},
	{ "inshape ",	 0, "<glshape id> <inglshape id>"				},
	{ "light ",	 0, "<shape id> <gllight i> <pname> <value>"			},
	{ "lightv ",	 0, "<shape id> <gllight i> <pname> <vector id>"		},
	{ "loadidentity ", 0, "<glshape id>"						},
	{ "materialv ",	 0, "<glshape id> <face> <pname> <vector id>"			},
	{ "matrixmode ", 0, "<glshape id> (projection | modelview | texture | color)"	},
	{ "modify ",	 0, "<glshape id> <line> (<no>[,<no>...] (<value> | values>)"	},
	{ "moveentry ",	 0, "<shape id> <entry id> [<to entry>]"			},
	{ "normal ",	 0, "<glshape id> <x> <y> <z>"					},
	{ "noop ",	 0, "<glshape id>"						},
	{ "ortho ",	 0, "<glshape id> <left> <right> <bottom> <top> <near> <far>"	},
	{ "popmatrix ",	 0, "<glshape id>"						},
	{ "pushmatrix ", 0, "<glshape id>"						},
	{ "recursion ",	 0, "<glshape id> <level>"					},
	{ "rotate ",	 0, "<glshape id> <angle> <x> <y> <z>"				},
	{ "scale ",	 0, "<glshape id> <scale x> <scale y> <scale z>"		},
	{ "scissor ",	 0, "<glshape id> <x> <y> <width> <height>"			},
	{ "shademodel ", 0, "<glshape id> (flat|smooth)"				},
	{ "snowmix ",	 0, "<glshape id> <snowmix command>"				},
	{ "translate ",	 0, "<glshape id> <x> <y> <z>"					},
	{ "texcoord ",	 0, "<glshape id> <s> [<r> [<t> [<q>]]]"			},
	{ "texfilter2d ", 0, "<glshape id> <near filter> <mag filter>"			},
	{ "texture add ", 0, "[<texture id> [<texture name>]]"				},
	{ "texture bind ", 0, "<glshape id> <texture id> [<min> <mag>] [2d | cube]"	},
	{ "texture source ", 0, "[<texture id> (feed <feed id>|frame <frame id>|image <image id>)|none [<id>]]"			},
	{ "vbo add",	 0, "add <vbo id> <name>"					},
	{ "vbo config ", 0, "config <vbo id> [(static|dynamic|stream) <form> (c3 | c4 | n3 | t1 | t2 | t3 | t4 | v2 | v3 | v4) ...]"},
	{ "vbo data ",	 0, "<vbo id> [<data 0> <data 1> ...]"				},
	{ "vbo indices ", 0, "<vbo id> [<index 0> <index 1> ...]"			},
	{ "vbo ",	 0, "<glshape id> <vbo id>"					},
	{ "vector add ", 0, "[<vector id> [<vector name>]]"				},
	{ "vector value ", 0, "[<vector id> <a> <b> [<c> [<d>]]]"			},
	{ "verbose ",	 0, "[<level>]"							},
	{ "vertex ",	 0, "<glshape id> <x> [<y> [<z> [<w>]]]"			},
	{ "help ",	 0, "[form | mode | filter | factor | light | modify]"		},
	{ "place help ", 0, ""								},
	{ NULL,		 0, NULL							}
};
*/

int COpenglVideo::list_glshape_help(struct controller_type* ctr, const char* str)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	if (str) while (isspace(*str)) str++;
	if (!str || !(*str)) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"Commands:\n"
			MESSAGE"glshape add [<glshape id> [<shape name>]]\n"
			MESSAGE"glshape arcxz <glshape id> <angle> <aspect> <slices> <texleft> <texright> <textop> <texbottom>\n"
			MESSAGE"glshape arcyz <glshape id> <angle> <aspect> <slices> <texleft> <texright> <textop> <texbottom>\n"
			MESSAGE"glshape begin <glshape id> <form>\n"
			MESSAGE"glshape blendfunc <glshape id> <s factor> <d factor>\n"
			MESSAGE"glshape clear <glshape id> (depth | color | color+depth)\n"
			MESSAGE"glshape clearcolor <glshape id> <red> <green> <blue> [<alpha>]\n"
			MESSAGE"glshape color <glshape id> <red> <green> <blue> [<alpha>]\n"
			MESSAGE"glshape context [(osmesa|glx|auto)]\n"
			MESSAGE"glshape disable <glshape id> <mode>\n"
			MESSAGE"glshape enable <glshape id> <mode>\n"
			MESSAGE"glshape end <glshape id>\n"
			MESSAGE"glshape entry <glshape id> (active|inactivate|number)\n"
			MESSAGE"glshape finish <glshape id>\n"
			MESSAGE"glshape flush <glshape id>\n"
#ifdef HAVE_GLU
			MESSAGE"glshape glucylinder <glshape id> <quad id> <base> <top> <height> <slices> <stacks>\n"
			MESSAGE"glshape gludisk <quad id> <inner> <outer> <slices> <loops>\n"
			MESSAGE"glshape gludrawstyle <glshape id> <quad id> (fill|line|point|silhouette)\n"
			MESSAGE"glshape glunormals <glshape id> <quad id> (none|flat|smooth)\n"
			MESSAGE"glshape gluorientation <glshape id> <quad id> (outside|inside)\n"
			MESSAGE"glshape glupartialdisk <quad id> <inner> <outer> <slices> <loops> <start> <sweep>\n"
			MESSAGE"glshape gluperspective <glshape id> <fovy> <aspect> <znear> <zfar>\n"
			MESSAGE"glshape glusphere <glshape id> <quad id> <radius> <slices> <stacks>\n"
			MESSAGE"glshape glutexture <glshape id> <quad id> (0|1)\n"
#endif // #ifdef HAVE_GLU
			MESSAGE"glshape if (osmesa|glx) <glshape id> <shape command>\n"
			MESSAGE"glshape if (osmesa|glx) <glshape command>\n"
			MESSAGE"glshape info\n"
			MESSAGE"glshape inshape <glshape id> <inglshape id>\n");
#define GLSHAPE "glshape"
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape light <shape id> <gllight i> <pname> <value>\n"
			MESSAGE"glshape lightv <shape id> <gllight i> <pname> <vector id>\n"
			MESSAGE"glshape loadidentity <glshape id>\n"
			MESSAGE"glshape materialv <glshape id> <face> <pname> <vector id>\n"
			MESSAGE"glshape matrixmode <glshape id> (projection | modelview | texture | color)\n"
			MESSAGE"glshape modify <glshape id> <line> (<no>[,<no>...] (<value> | values>)\n"
			MESSAGE""GLSHAPE" moveentry <shape id> <entry id> [<to entry>]\n"
			MESSAGE"glshape normal <glshape id> <x> <y> <z>\n"
			MESSAGE"glshape noop <glshape id>\n"
			MESSAGE"glshape ortho <glshape id> <left> <right> <bottom> <top> <near> <far>\n"
			MESSAGE"glshape popmatrix <glshape id>\n"
			MESSAGE"glshape pushmatrix <glshape id>\n"
			MESSAGE"glshape recursion <glshape id> <level>\n"
			MESSAGE"glshape rotate <glshape id> <angle> <x> <y> <z>\n"
			MESSAGE"glshape scale <glshape id> <scale x> <scale y> <scale z>\n"
			MESSAGE"glshape scissor <glshape id> <x> <y> <width> <height>\n"
			MESSAGE"glshape shademodel <glshape id> (flat|smooth)\n"
			MESSAGE"glshape snowmix <glshape id> <snowmix command>\n"
			MESSAGE"glshape translate <glshape id> <x> <y> <z>\n"
			MESSAGE"glshape texcoord <glshape id> <s> [<r> [<t> [<q>]]]\n"
			MESSAGE"glshape texfilter2d <glshape id> <near filter> <mag filter>\n"
			MESSAGE"glshape texture add [<texture id> [<texture name>]]\n"
			MESSAGE"glshape texture bind <glshape id> <texture id> [<min> <mag>] [(2d | cube)]\n"
			MESSAGE"glshape texture source [<texture id> (feed <feed id>|frame "
				"<frame id>|image <image id>)|none [<id>]]\n"
			MESSAGE"glshabe vbo add <vbo id> <name>\n"
			MESSAGE"glshabe vbo config <vbo id> [(static|dynamic|stream) <form> (c3 | c4 | n3 | t1 | t2 | t3 | t4 | v2 | v3 | v4) ...]\n"
			MESSAGE"glshabe vbo data <vbo id> [<data 0> <data 1> ...]\n"
			MESSAGE"glshabe vbo indices <vbo id> [<index 0> <index 1> ...]\n"
			MESSAGE"glshabe vbo <glshape id> <vbo id>\n"
	
			MESSAGE"glshape vector add [<vector id> [<vector name>]]\n"
			MESSAGE"glshape vector value [<vector id> <a> <b> [<c> [<d>]]]\n"
			MESSAGE"glshape verbose [<level>]\n"
			MESSAGE"glshape vertex <glshape id> <x> [<y> [<z> [<w>]]]\n"
			MESSAGE"glshape help [form | mode | filter | factor | light | modify]\n"
			MESSAGE"glshape place help\n"
			MESSAGE"\n");
	} else if (!strncasecmp(str, "place ", 6)) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"Commands:\n"
			MESSAGE"glshape copyback [frame | image <image_id> <x> <y> <width> <height>]\n"
			MESSAGE"glshape [ place ] overlay (<id> | <id>..<id> | <id>..end | all) [(<id> | <id>..<id> | <id>..end | all)] ....\n"
			MESSAGE"glshape place [<place id> [ <shape id> <x> <y> <z> "
				"[<scale x> <scale y> <scale z> [<rotation> <rx> <ry> <rz> "
				"[<red> <green> <blue> [<alpha>]]]]]\n"
			MESSAGE"glshape place alpha [<place id> [<alpha>]]\n"
			MESSAGE"glshape place coor [<place id> [<x> <y> <z>]]\n"
			MESSAGE"glshape place rgb [<place id> [<red> <green> <blue> [<alpha>]]]\n"
			MESSAGE"glshape place rotation [<place id> [<rotation> <rx> <ry> <rz>]]\n"
			MESSAGE"glshape place scale [<place id> [<scale x> <scale y> <scale z>]]\n"
			MESSAGE"glshape place help	// this help\n"
			MESSAGE"\n");
	}
	else if (!strncasecmp(str, "filter ", 7))
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"<near filter> = nearest | linear\n"
			MESSAGE"<mag filter> = nearest | linear\n"MESSAGE"\n");
	else if (!strncasecmp(str, "form ", 5)) {
		int i;
		for (i=0; gl_begin_list[i].name; i++) {
			if (!gl_begin_list[i].name) break;
			if (i % 3 == 0)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s"MESSAGE"<form> = ", i ? "\n" : "");
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"%s%s",
				gl_begin_list[i].name,
				(i+1)%3 && gl_begin_list[i+1].name ? " | " : "");
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "\n"MESSAGE"\n");
	}
	else if (!strncasecmp(str, "light ", 6)) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"Light parameters\n"
			MESSAGE"<gllight i> : light0..light%d (0..7 supported)\n", GL_MAX_LIGHTS);
		int i;
		for (i=0; gl_light_parm_list[i].name; i++) {
			//if (i && !strcmp(gl_light_parm_list[i].name,gl_light_parm_list[i-1].name)) continue;
			if (!gl_light_parm_list[i].name) break;
			if (i % 5 == 0)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s"MESSAGE" <pname> : ", i ? "\n" : "");
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"%s%s",
				gl_light_parm_list[i].name,
				(i+1)%5 && gl_light_parm_list[i+1].name ? "| " : "");
		 }
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "\n"MESSAGE"\n");
	}
	else if (!strncasecmp(str, "modify ", 5)) {
		int i;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"List of glshape elements where its parameters can be changed with the modify command:\n");
		for (i=0; gl_modify_list[i].name; i++) {
			//if (i && !strcmp(gl_modify_list[i].name,gl_modify_list[i-1].name)) continue;
			if (!gl_modify_list[i].name) break;
			if (i % 5 == 0)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s"MESSAGE" : ", i ? "\n" : "");
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"%s%s",
				gl_modify_list[i].name,
				(i+1)%5 && gl_modify_list[i+1].name ? ", " : "");
		 }
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "\n"MESSAGE"\n");
	}
	else if (!strncasecmp(str, "mode ", 5)) {
		int i;
		for (i=0; gl_enable_mode_list[i].name; i++) {
			if (!gl_enable_mode_list[i].name) break;
			if (i % 3 == 0)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s"MESSAGE"<mode> = ", i ? "\n" : "");
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"%s%s",
				gl_enable_mode_list[i].name,
				(i+1)%3 && gl_enable_mode_list[i+1].name ? " | " : "");
		 }
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "\n"MESSAGE"\n");
	}
	else if (!strncasecmp(str, "factor ", 5)) {
		int i;
		for (i=0; gl_blendfunc_list[i].name; i++) {
			if (!gl_blendfunc_list[i].name) break;
			if (i % 3 == 0)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s"MESSAGE"<factor> = ", i ? "\n" : "");
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"%s%s",
				gl_blendfunc_list[i].name,
				(i+1)%3 && gl_blendfunc_list[i+1].name ? " | " : "");
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "\n"MESSAGE"\n");
	} else return -1;
	return 0;
}

/* CheckForGlError will check and empty pending GL errors
 * If verbose is 0, the check will be silent
 * If verbose is 1, only the first error message will be reported on stderr.
 * If verbose os 2 or greater, all error messages will be reported on stderr.
 */
int COpenglVideo::CheckForGlError(u_int32_t verbose, const char* str)
{
	const char* errorstr = NULL;
	GLenum glError;
	int error_count = 0;
	while ((glError = glGetError()) != GL_NO_ERROR) {
		error_count++;
		if (!verbose) continue;
		if (verbose > 1 || error_count == 1)
		Mode2String(errorstr, glError, gl_error_list);
		fprintf(stderr, "GL Error %s: %s\n", str ? str : "",
			errorstr ? errorstr : "unknown error type");
	}
	return error_count;
}

int COpenglVideo::OverlayPlacedShape(u_int32_t place_id) {
	bool matrix_pushed = false;
	int n;

	if (!m_placed_shapes || !m_pVideoMixer) return 1;
	if (place_id >= m_max_placed_shapes || !m_placed_shapes[place_id] ||
		!m_pVideoMixer->m_overlay) return -1;

	if (m_placed_shapes[place_id]->color.alpha == 0) return 0;
	// Create a Context if one does not exists
	//if (!m_pCtx_OSMesa && CreateContext(m_pVideoMixer->m_overlay)) return 1;

	if (!m_current_context_is_valid && MakeCurrent(m_pVideoMixer->m_overlay)) return 1;
	// Context exists. Save matrix if needed (default)
	if (m_placed_shapes[place_id]->push_matrix) {
		glPushMatrix();
		matrix_pushed = true;
	}
#ifdef SM_OPENGL_PARM_TYPE_INT
	// Translate coordinates
	SM_GL_OPER_TRANSLATE(
		(float)m_placed_shapes[place_id]->translate.x,
		(float)m_placed_shapes[place_id]->translate.y,
		(float)m_placed_shapes[place_id]->translate.z);
	// Do scaling
	SM_GL_OPER_SCALE(
		(float)m_placed_shapes[place_id]->scale.x,
		(float)m_placed_shapes[place_id]->scale.y,
		(float)m_placed_shapes[place_id]->scale.z);
	// Do vector rotation
	SM_GL_OPER_ROTATE(
		(float)m_placed_shapes[place_id]->rotation.angle,
		(float)m_placed_shapes[place_id]->rotation.x,
		(float)m_placed_shapes[place_id]->rotation.y,
		(float)m_placed_shapes[place_id]->rotation.z);
	SM_GL_OPER_COLOR4(
		(float)m_placed_shapes[place_id]->color.red,
		(float)m_placed_shapes[place_id]->color.green,
		(float)m_placed_shapes[place_id]->color.blue,
		(float)m_placed_shapes[place_id]->color.alpha);
#else
	// Translate coordinates
	SM_GL_OPER_TRANSLATE(
		(sm_opengl_parm_t) m_placed_shapes[place_id]->translate.x,
		(sm_opengl_parm_t) m_placed_shapes[place_id]->translate.y,
		(sm_opengl_parm_t) m_placed_shapes[place_id]->translate.z);
	// Do vector rotation
	SM_GL_OPER_ROTATE(
		(sm_opengl_parm_t)m_placed_shapes[place_id]->rotation.angle,
		(sm_opengl_parm_t)m_placed_shapes[place_id]->rotation.x,
		(sm_opengl_parm_t)m_placed_shapes[place_id]->rotation.y,
		(sm_opengl_parm_t)m_placed_shapes[place_id]->rotation.z);
	// Do scaling
	SM_GL_OPER_SCALE(
		(sm_opengl_parm_t)m_placed_shapes[place_id]->scale.x,
		(sm_opengl_parm_t)m_placed_shapes[place_id]->scale.y,
		(sm_opengl_parm_t)m_placed_shapes[place_id]->scale.z);
	SM_GL_OPER_COLOR4(
		(sm_opengl_parm_t)m_placed_shapes[place_id]->color.red,
		(sm_opengl_parm_t)m_placed_shapes[place_id]->color.green,
		(sm_opengl_parm_t)m_placed_shapes[place_id]->color.blue,
		(sm_opengl_parm_t)m_placed_shapes[place_id]->color.alpha);
		
/*
fprintf(stderr, "Setting shape place %u shape %u color %f,%f,%f,%f\n", place_id,
		m_placed_shapes[place_id]->shape_id,
		(float)m_placed_shapes[place_id]->color.red,
		(float)m_placed_shapes[place_id]->color.green,
		(float)m_placed_shapes[place_id]->color.blue,
		(float)m_placed_shapes[place_id]->color.alpha);
*/
#endif
	n = ExecuteShape(m_placed_shapes[place_id]->shape_id, OVERLAY_GLSHAPE_LEVELS,
		m_placed_shapes[place_id]->scale.x,
		m_placed_shapes[place_id]->scale.y,
		m_placed_shapes[place_id]->scale.z);
	if (matrix_pushed) glPopMatrix();

	// Check and empty GLerrors
	char str[35];
	sprintf(str, "in OverlayPlacedShape(%u)", place_id);
	CheckForGlError(m_verbose, str);

	return n;
}

// glshape [ place ] overlay (<id> | <id>..<id> | <id>..end | all) [(<id> | <id>..<id> | <id>..end | all)] ....
int COpenglVideo::overlay_placed_shape(struct controller_type* ctr, const char *str)
{
	// Check for system error
	if (!str || !m_pVideoMixer) return 1;

	// Check that we are ready to overlay
	if (!m_pVideoMixer->m_overlay) return -1;

	// We need to signal to Cairo overlays that we will draw in the overlay frame
	// FIXME

	// Check that there are anything to overlay
	while (isspace(*str)) str++;
	if (!(*str) || !m_placed_shape_count) return -1;
	u_int32_t from = 0, to = 0;
	const char* nextstr;
	while (*str) {
		if (!(nextstr = GetIdRange(str, &from, &to, 0,
			m_max_placed_shapes ? m_max_placed_shapes - 1 : 0))) break;
		while (from <= to) {
			if (m_placed_shapes[from]) OverlayPlacedShape(from);
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	return 0;
}


int COpenglVideo::ShapeOps(u_int32_t shape_id)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return -1;
	return m_shapes[shape_id]->ops_count;
}

// glshape context (osmesa|glx)
int COpenglVideo::set_preferred_context(struct controller_type* ctr, const char* str)
{
	if (!str) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"GL Context preferred is %s\n",
			m_use_glx ? "GLX" : "OSMesa");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE" - OSMesa : %s%s\n",
			m_pCtx_OSMesa ? "Created" : "Not Created",
			m_use_glx ? "" : ", preferred");
#ifdef HAVE_GLX
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE" - GLX    : %s%s\n",
			m_pCtx_GLX ? "Created" : "Not Created",
			m_use_glx ? ", preferred" : "");
#endif
		m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"\n");
		return 0;
	}
	if (!(strncasecmp(str, "osmesa ", 7))) {
		m_use_glx = false;
	} else if (!(strncasecmp(str, "glx ", 4))) {
#ifdef HAVE_GLX
		m_use_glx = true;
#else
		fprintf(stderr, "WARNING. GLX not enabled when compiling Snowmix. Setting preferred context to glx is not possible.\n");
		m_use_glx = false;
		return -1;
#endif
	} else return -1;
	if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE" Preferred OpenGL Context is set to %s\n",
			m_use_glx ? "GLX" : "OSMesa");
	return 0;
}

int COpenglVideo::set_shape_moveentry(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;

	u_int32_t shape_id, entry_id, to_entry = 0;

	int n = sscanf(str, "%u %u %u", &shape_id, &entry_id, &to_entry);
	if (n < 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	if (n == 2) {
		while (isdigit(*str)) str++;
		while (isspace(*str)) str++;
	}
	if (n == 1) {
		if (!strncmp(str, "first ", 6)) {
			entry_id = 1;
			n++;
		} else if (!strncmp(str, "last ", 5) || !strcmp(str,"last")) {
			int last = ShapeOps(shape_id);
			if (last < 1) return -1;
			entry_id = last;
			n++;
		}
		if (n > 1) {
			while (*str && !isspace(*str)) str++;
			while (isspace(*str)) str++;
		}
	}
	if (n == 2) {
		if (!strncmp(str, "first ", 6)) {
			to_entry = 1;
			n++;
		} else if (!strncmp(str, "last ", 5)) {
			int last = ShapeOps(shape_id);
			if (last < 1) return -1;
			to_entry = last;
			n++;
		} else {
		  if (*str && sscanf(str, "%u", &to_entry) != 1) return -1;
		  if (*str) n++;
		}
	}
	
	if (n == 2 || n == 3) {
		int i = ShapeMoveEntry(shape_id, entry_id, n == 3 ? to_entry : 0);
		if (i) return -1;
		if (m_verbose && !i && m_pVideoMixer && m_pVideoMixer->m_pController) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"glshape %u moventry %u%s", shape_id, entry_id,
				n == 2 ? " (deleted)\n" : "");
			if (n == 3) m_pVideoMixer->m_pController->controller_write_msg (ctr,
				" to %u\n", to_entry);
		}
		return 0;
	}
	return -1;
}

int COpenglVideo::ShapeMoveEntry(u_int32_t shape_id, u_int32_t entry_id, u_int32_t to_entry)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id] ||
		!m_shapes[shape_id]->pDraw || entry_id < 1) return 1;
	opengl_draw_operation_t* p = m_shapes[shape_id]->pDraw;
	opengl_draw_operation_t* pInsert = NULL;
	u_int32_t n = 1;

	// Search for the entry id and remove it
	// Check if entry is 1. Check also Tail if there is only one entry in the list
	if (entry_id == 1) {
		// Check if there is only one entry, then set tail to NULL
		if (m_shapes[shape_id]->pDrawTail == m_shapes[shape_id]->pDraw)
			m_shapes[shape_id]->pDrawTail = NULL;
		// Remove entry if entry id is 1
		pInsert = m_shapes[shape_id]->pDraw;
		m_shapes[shape_id]->pDraw = m_shapes[shape_id]->pDraw->next;
	} else while (p) {
		// Else check if entry id is the next one
		if (n+1 == entry_id) {
			pInsert = p->next;
			// If next entry is not NULL, remove entry in list
			// and point current next to entry after next entry
			if (p->next) p->next = p->next->next;
			// Check to see if we need to update Tail
			if (m_shapes[shape_id]->pDrawTail == pInsert)
				m_shapes[shape_id]->pDrawTail = p;
			break;
		}
		n++;
		p = p->next;
	}
	// Check that we found entry
	if (!pInsert) return -1;

	// If no toentry, we free the entry and return succesfully
	if (to_entry == 0) {
		if (pInsert->create_str) free(pInsert->create_str);
		free(pInsert);
		m_shapes[shape_id]->ops_count--;
		return 0;
	}
	p = m_shapes[shape_id]->pDraw;
	n = 1;
	if (to_entry == 1) {
		pInsert->next = m_shapes[shape_id]->pDraw;
		m_shapes[shape_id]->pDraw = pInsert;
		if (pInsert->next == NULL)  m_shapes[shape_id]->pDrawTail = pInsert;
	} else {
		while (p) {
			if (n+1 == to_entry || !p->next) {
				pInsert->next = p->next;
				p->next = pInsert;
				if (pInsert->next == NULL)
					m_shapes[shape_id]->pDrawTail = pInsert;
				break;
			}
			n++;
			p = p->next;
		}
	}
	return 0;
}

// glshape place scale [<place id> [<scale x> <scale y> <scale z>]]
int COpenglVideo::set_shape_scale(struct controller_type* ctr, const char* str) {
	if (!str || !m_placed_shapes) return 1;
	while (isspace(*str)) str++;
	u_int32_t place_id = m_max_placed_shapes;
	if (sscanf(str, "%u", &place_id) == 1) {
		if (place_id >= m_max_placed_shapes ||
			!m_placed_shapes[place_id]) return -1;
		while (isdigit(*str)) str++;
		while (isspace(*str)) str++;
	}
	// Should we list placed shapes ?
	if (!(*str)) {
		if (!m_pVideoMixer || ! m_pVideoMixer->m_pController || !m_placed_shapes)
			return 1;
		for (u_int32_t id = 0; id < m_max_placed_shapes; id++)
			if (m_placed_shapes[id] && (place_id == m_max_placed_shapes ||
				id == place_id)) {
				opengl_placed_shape_t* p = m_placed_shapes[id];
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" glshape place %u scale %.4lf,%4lf,%4lf\n",
					id,
					p->scale.x, p->scale.y, p->scale.z);
			}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		
		return 0;
	}

	// We are changing scale
	int n;
	double scale_x, scale_y, scale_z;
	if (sscanf(str, "%lf %lf %lf", &scale_x, &scale_y, &scale_z) != 3) return -1;
	n = SetPlacedShapeScale(place_id, scale_x, scale_y, scale_z);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape place %u scale %.4lf,%.4lf,%.4lf\n",
			place_id, scale_x, scale_y, scale_z);
	return n;
}

// glshape place rgb [<place id> [<red> <green> <blue> [<alpha>]]]
// glshape place alpha [<place id> [<alpha>]]
int COpenglVideo::set_shape_rgb(struct controller_type* ctr, const char* str, bool isrgb) {
	if (!str || !m_placed_shapes) return 1;
	while (isspace(*str)) str++;
	u_int32_t place_id = m_max_placed_shapes;
	if (sscanf(str, "%u", &place_id) == 1) {
		if (place_id >= m_max_placed_shapes ||
			!m_placed_shapes[place_id]) return -1;
		while (isdigit(*str)) str++;
		while (isspace(*str)) str++;
	}
	// Should we list placed shapes ?
	if (!(*str)) {
		if (!m_pVideoMixer || ! m_pVideoMixer->m_pController || !m_placed_shapes)
			return 1;
		for (u_int32_t id = 0; id < m_max_placed_shapes; id++)
			if (m_placed_shapes[id] && (place_id == m_max_placed_shapes ||
				id == place_id)) {
				opengl_placed_shape_t* p = m_placed_shapes[id];
				if (isrgb) m_pVideoMixer->m_pController->controller_write_msg(ctr,
					STATUS" glshape place %u rgb %.4lf,%4lf,%4lf "
					"alpha %.4lf\n", id,
					p->color.red, p->color.green, p->color.blue, p->color.alpha);
				else m_pVideoMixer->m_pController->controller_write_msg(ctr,
					STATUS" glshape place %u alpha %.4lf\n", id,
					p->color.alpha);
			}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		
		return 0;
	}

	// We are changing rgb or alpha or all
	int n;
	double red, green, blue, alpha = -1;
	if (isrgb) {
		if (sscanf(str, "%lf %lf %lf %lf", &red, &green, &blue, &alpha) < 3) return -1;
		n = SetPlacedShapeColor(place_id, red, green, blue, alpha);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController) {
			if (alpha >= 0) m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"glshape place %u rgb %.4lf,%.4lf,%.4lf alpha %.4lf\n",
				place_id, red, green, blue, alpha);
			else m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"glshape place %u rgb %.4lf,%.4lf,%.4lf\n",
				place_id, red, green, blue);
		}
	} else {
		if (sscanf(str, "%lf", &alpha) != 1) return -1;
		n = SetPlacedShapeAlpha(place_id, alpha);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"glshape place %u alpha %.4lf\n", place_id, alpha);
	}
	return n;
}

// glshape place coor [<place id> [<x> <y> <z>]]
int COpenglVideo::set_shape_coor(struct controller_type* ctr, const char* str) {
	if (!str || !m_placed_shapes) return 1;
	while (isspace(*str)) str++;
	u_int32_t place_id = m_max_placed_shapes;
	if (sscanf(str, "%u", &place_id) == 1) {
		if (place_id >= m_max_placed_shapes ||
			!m_placed_shapes[place_id]) return -1;
		while (isdigit(*str)) str++;
		while (isspace(*str)) str++;
	}
	// Should we list placed shapes ?
	if (!(*str)) {
		if (!m_pVideoMixer || ! m_pVideoMixer->m_pController || !m_placed_shapes)
			return 1;
		for (u_int32_t id = 0; id < m_max_placed_shapes; id++)
			if (m_placed_shapes[id] && (place_id == m_max_placed_shapes ||
				id == place_id)) {
				opengl_placed_shape_t* p = m_placed_shapes[id];
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" glshape place %u coor %.4lf,%4lf,%4lf\n",
					id,
					p->translate.x, p->translate.y, p->translate.z);
			}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		
		return 0;
	}

	// We are changing coor
	int n;
	double x, y, z;
	if (sscanf(str, "%lf %lf %lf", &x, &y, &z) != 3) return -1;
	n = SetPlacedShapeCoor(place_id, x, y, z);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape place %u coor %.4lf,%.4lf,%.4lf\n",
			place_id, x, y, z);
	return n;
}

// glshape place rotation [<place id> [rotation <x> <y> <z>]]
int COpenglVideo::set_shape_rotation(struct controller_type* ctr, const char* str) {
	if (!str || !m_placed_shapes) return 1;
	while (isspace(*str)) str++;
	u_int32_t place_id = m_max_placed_shapes;
	if (sscanf(str, "%u", &place_id) == 1) {
		if (place_id >= m_max_placed_shapes ||
			!m_placed_shapes[place_id]) return -1;
		while (isdigit(*str)) str++;
		while (isspace(*str)) str++;
	}
	// Should we list placed shapes ?
	if (!(*str)) {
		if (!m_pVideoMixer || ! m_pVideoMixer->m_pController || !m_placed_shapes)
			return 1;
		for (u_int32_t id = 0; id < m_max_placed_shapes; id++)
			if (m_placed_shapes[id] && (place_id == m_max_placed_shapes ||
				id == place_id)) {
				opengl_placed_shape_t* p = m_placed_shapes[id];
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" glshape place %u rotation %.4lf vector "
					"%.4lf,%4lf,%4lf\n", id,
					p->rotation.angle, p->rotation.x,
					p->rotation.y, p->rotation.z);
			}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		
		return 0;
	}

	// We are changing rotation
	int n;
	double rotation, rx, ry, rz;
	if (sscanf(str, "%lf %lf %lf %lf", &rotation, &rx, &ry, &rz) != 4) return -1;
	n = SetPlacedShapeRotation(place_id, rotation, rx, ry, rz);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape place %u rotation %.4lf vector %.4lf,%.4lf,%.4lf\n",
			place_id, rotation, rx, ry, rz);
	return n;
}

// glshape place <place id> [ <shape id> <x> <y> <z> [ <scale x> <scale y> <scale z> [ <rotation> <rx> <ry> <rz> [ <red> <green> <blue> [ <alpha> ] ] ] ] ]
int COpenglVideo::set_shape_place(struct controller_type* ctr, const char* str) {
	if (!str) return 1;
	while (isspace(*str)) str++;

	// Should we list placed shapes ?
	if (!(*str)) {
		if (!m_pVideoMixer || ! m_pVideoMixer->m_pController || !m_placed_shapes)
			return 1;
		for (u_int32_t id = 0; id < m_max_placed_shapes; id++)
			if (m_placed_shapes[id]) {
				opengl_placed_shape_t* p = m_placed_shapes[id];
				 m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" glshape place %u shape %u at %.4lf,%.4lf,%.4lf "
					"scale %.4lf,%4lf,%4lf "
					"rotation %.4lf vector %.4lf,%.4lf,%.4lf "
					"color %.3lf,%.3lf,%.3lf alpha %.3lf%s\n",
					id, p->shape_id,
					p->translate.x, p->translate.y, p->translate.z,
					p->scale.x, p->scale.y, p->scale.z,
					p->rotation.angle, p->rotation.x, p->rotation.y, p->rotation.z,
					p->color.red, p->color.green, p->color.blue,
					p->color.alpha, p->push_matrix ? " pushmatrix" : "");
			}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		
		return 0;
	}

	// We are creating or deleting a shape
	int n;
	u_int32_t place_id, shape_id;
	double x, y, z, scale_x, scale_y, scale_z, rotation, rx, ry, rz, red, green, blue, alpha;
	n = sscanf(str, "%u %u %lf %lf %lf   %lf %lf %lf   %lf %lf %lf %lf  %lf %lf %lf %lf",
		&place_id, &shape_id, &x, &y, &z, &scale_x, &scale_y, &scale_z, &rotation, &rx, &ry, &rz, &red, &green, &blue, &alpha);

	// n =  1 : delete place_id
	// n =  5 : Setting place, shape, x,y,z
	// n =  8 : Setting place, shape, x,y,z, scalex,y,z
	// n = 12 : Setting place, shape, x,y,z, scalex,y,z rot,x,y,z
	// n = 15 : Setting place, shape, x,y,z, scalex,y,z rot,x,y,z r,g,b
	// n = 16 : Setting place, shape, x,y,z, scalex,y,z rot,x,y,z r,g,b,a
	if (n == 1) {
		n = DeletePlacedShape(place_id);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"glshape place %u deleted\n", place_id);
		return n;
	}
	int i;
	if (n == 5 || n == 8 || n == 12 || n == 15 || n == 16) {
		if ((i = AddPlacedShape(place_id, shape_id, x, y, z))) return -1;
	} else return -1;
	if (n > 5) {
		if ((i = SetPlacedShapeScale(place_id, scale_x,
			scale_y, scale_z))) return i;
	}
	if (n > 8) {
		if ((i = SetPlacedShapeRotation(place_id, rotation,
			rx, ry, rz))) return i;
	}
	if (n > 12)
		if ((i = SetPlacedShapeRotation(place_id, red, green,
		blue, n == 15 ? -1 : alpha))) return i;
	if (!i && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape place %u shape %u added%s%s%s%s.\n",
			place_id, shape_id,
			i > 5 ? " with scale" : "",
			i > 8 ? " with rotation" : "",
			i > 12 ? " with color" : "",
			i > 15 ? " with alpha" : "");
	
	return i;
}

int COpenglVideo::AddPlacedShape(u_int32_t place_id, u_int32_t shape_id, double x, double y, double z) {
	if (!m_placed_shapes || !m_shapes) return 1;
	if (place_id >= m_max_placed_shapes || shape_id >= m_max_shapes ||
		!m_shapes[shape_id]) return -1;
	opengl_placed_shape_t* p = m_placed_shapes[place_id];
	if (!p) {
		p = (opengl_placed_shape_t*) calloc(sizeof(opengl_placed_shape_t),1);
		if (p) m_placed_shape_count++; else return 1;
	}
	p->shape_id = shape_id;
	if (!m_placed_shapes[place_id]) {
		p->push_matrix = true;
		m_placed_shapes[place_id] = p;
		p->rotation.angle = 0;
		p->rotation.x = 0;
		p->rotation.y = 0;
		p->rotation.z = 0;
		p->scale.x = 1;
		p->scale.y = 1;
		p->scale.z = 1;
		p->color.red = 1;
		p->color.green = 1;
		p->color.blue = 1;
		p->color.alpha = 1;
	}
	p->translate.x = x;
	p->translate.y = y;
	p->translate.z = z;
	return 0;
}

int COpenglVideo::SetPlacedShapeRotation(u_int32_t place_id, double angle, double x, double y, double z) {
	if (!m_placed_shapes) return 1;
	if (place_id >= m_max_placed_shapes || !m_placed_shapes[place_id]) return -1;
	while (angle > 360) angle -=360;
	while (angle < -360) angle +=360;
	if (m_verbose && x == 0 && y == 0 && z == 0) fprintf(stderr, "Warning. Setting rotation for placed glshape %u with a null vector has no effect.\n", place_id);
	m_placed_shapes[place_id]->rotation.angle = angle;
	m_placed_shapes[place_id]->rotation.x = x;
	m_placed_shapes[place_id]->rotation.y = y;
	m_placed_shapes[place_id]->rotation.z = z;
	return 0;
}
	
int COpenglVideo::SetPlacedShapeCoor(u_int32_t place_id, double x, double y, double z) {
	if (!m_placed_shapes) return 1;
	if (place_id >= m_max_placed_shapes || !m_placed_shapes[place_id]) return -1;
	m_placed_shapes[place_id]->translate.x = x;
	m_placed_shapes[place_id]->translate.y = y;
	m_placed_shapes[place_id]->translate.z = z;
	return 0;
}
	
int COpenglVideo::SetPlacedShapeScale(u_int32_t place_id, double x, double y, double z) {
	if (!m_placed_shapes) return 1;
	if (place_id >= m_max_placed_shapes || !m_placed_shapes[place_id] ||
		x == 0 || y == 0 || z == 0) return -1;
	if (m_verbose && x == 1 && y == 1 && z == 1) fprintf(stderr, "Warning. Setting scale for placed glshape %u to the unity vector has no effect.\n", place_id);
	m_placed_shapes[place_id]->scale.x = x;
	m_placed_shapes[place_id]->scale.y = y;
	m_placed_shapes[place_id]->scale.z = z;
	return 0;
}
	
int COpenglVideo::SetPlacedShapeColor(u_int32_t place_id, double red, double green, double blue, double alpha) {
	if (!m_placed_shapes) return 1;
	if (place_id >= m_max_placed_shapes || !m_placed_shapes[place_id]) return -1;
	if (red < 0) red = 0; else if (red > 1) red = 1;
	if (green < 0) green = 0; else if (green > 1) green = 1;
	if (blue < 0) blue = 0; else if (blue > 1) blue = 1;
	if (alpha > 1) alpha = 1;
	m_placed_shapes[place_id]->color.red = red;
	m_placed_shapes[place_id]->color.green = green;
	m_placed_shapes[place_id]->color.blue = blue;
	if (alpha >= 0) m_placed_shapes[place_id]->color.alpha = alpha;
	return 0;
}

int COpenglVideo::SetPlacedShapeAlpha(u_int32_t place_id, double alpha) {
	if (!m_placed_shapes) return 1;
	if (place_id >= m_max_placed_shapes || !m_placed_shapes[place_id]) return -1;
	if (alpha > 1) alpha = 1; else if (alpha < 0) alpha = 0;
	m_placed_shapes[place_id]->color.alpha = alpha;
	return 0;
}
	
	

int COpenglVideo::LoadTexture(u_int32_t texture_id, GLint min, GLint mag)
{
	if (!m_textures || texture_id >= m_max_textures ||
		!m_textures[texture_id] || !m_pVideoMixer) return -1;

	u_int32_t width = 0, height = 0, seq_no = 0;
	u_int8_t* pixels = NULL;
	bool load_image = true;
	u_int32_t source_id = m_textures[texture_id]->source_id;

	//fprintf(stderr, "Load texture %u\n", texture_id);

	if (m_textures[texture_id]->source == TEXTURE_SOURCE_FEED) {
		if (!m_pVideoMixer->m_pVideoFeed) return -1;
		m_pVideoMixer->m_pVideoFeed->FeedGeometry(source_id, &width, &height);
		pixels = m_pVideoMixer->m_pVideoFeed->GetFeedFrame(source_id);

		// Get feed seq_no. Gets incremented if changed
		seq_no = m_pVideoMixer->m_pVideoFeed->FeedSeqNo(source_id);
	} else if (m_textures[texture_id]->source == TEXTURE_SOURCE_IMAGE) {
		if (!m_pVideoMixer->m_pVideoImage) return -1;
		image_item_t* pImage = m_pVideoMixer->m_pVideoImage->GetImageItem(source_id);
		if (!pImage) return -1;
		width = pImage->width;
		height = pImage->height;
		pixels = pImage->pImageData;

		// Get image seq_no. Gets incremented if changed
		seq_no = m_pVideoMixer->m_pVideoImage->ImageItemSeqNo(source_id);

	} else if (m_textures[texture_id]->source == TEXTURE_SOURCE_FRAME) {
		m_pVideoMixer->SystemGeometry(&width, &height);
		pixels = m_pVideoMixer->m_overlay;
	}
	if (!pixels || !width || !height) return -1;

	load_image = !m_textures[texture_id]->source_seq_no ||
		seq_no != m_textures[texture_id]->source_seq_no;
	m_textures[texture_id]->source_seq_no = seq_no;

	// Generate a texture holder if needed
	if (!m_textures[texture_id]->real_texture_id) {
//fprintf(stderr, "Generating texture for texture %u\n", texture_id);
		glGenTextures(1, &(m_textures[texture_id]->real_texture_id));
	}
	//else fprintf(stderr, "Skipping generating texture for texture %u\n", texture_id);

	if (!m_textures[texture_id]->real_texture_id) {
		fprintf(stderr, "WARNING. Texture generation failure. ID %u Source %s id %u\n",
			texture_id,
			m_textures[texture_id]->source == TEXTURE_SOURCE_FEED ? "feed" :
			m_textures[texture_id]->source == TEXTURE_SOURCE_IMAGE ? "image" :
			m_textures[texture_id]->source == TEXTURE_SOURCE_FRAME ? "frame" :
			"none", m_textures[texture_id]->source_id);

		
		// Check and empty GLerrors
		char str[27];
		sprintf(str, "in Loadtexture(%u)", texture_id);
		CheckForGlError(m_verbose, str);
	}

	//Tell OpenGL which texture to edit
	glBindTexture(GL_TEXTURE_2D, m_textures[texture_id]->real_texture_id);

	if (load_image) {
		//fprintf(stderr, "Loading image for texture %u\n", texture_id);
		//Map the image to the texture
		if (min == GL_LINEAR || min == GL_NEAREST) 
			glTexImage2D(
				 GL_TEXTURE_2D,			//Always GL_TEXTURE_2D
				 0,				//0 for now
				 GL_RGBA,                       //Format OpenGL uses for image
				 width, height,			//Width and height
				 0,				//The border of the image
				 GL_BGRA, //GL_RGB, because pixels are stored in RGB format
				 GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
						   //as unsigned numbers
				 pixels);			//The actual pixel data
		else {
#ifdef HAVE_GLU
fprintf(stderr, "Building mipmap for texture %u\n", texture_id );
			gluBuild2DMipmaps(GL_TEXTURE_2D,
				GL_RGBA,
				width, height,			//Width and height
				GL_BGRA, //GL_RGB, because pixels are stored in RGB format
				GL_UNSIGNED_BYTE, //GL_UNSIGNED_BYTE, because pixels are stored
						   //as unsigned numbers
				pixels);			//The actual pixel data
#else
			fprintf(stderr, "Can not build mipmaps. GLU was not included in compilation.\n");
#endif //#ifdef HAVE_GLU
		}
				
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag);

	}
	// else fprintf(stderr, "Skipping loading image for texture %u\n", texture_id);
	// Check and empty GLerrors
	char str[43];
	sprintf(str, "binding texture in Loadtexture(%u)", texture_id);
	CheckForGlError(m_verbose, str);

	return 0;
}

int COpenglVideo::ExecuteShape(u_int32_t shape_id, u_int32_t level, double scale_x, double scale_y, double scale_z)
{
	int line = 1;
	int begins = 0;
	sm_opengl_parm_t color4[4];
	if (!level || !m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return -1;
	color4[0] = 0.5f;
	SM_GL_OPER_GETV(GL_CURRENT_COLOR, color4);
	//CheckForGlError(2, "Getting color");
	//fprintf(stderr, "shape=%u red=%f, green=%f, blue=%f, alpha=%f\n",
		//shape_id, color4[0], color4[1], color4[2], color4[3]);
	opengl_draw_operation_t* p = m_shapes[shape_id]->pDraw;
	while (p) {

		if (!p->execute) {
			p = p->next;
			line++;
			continue;
		}

		// conditional = 1 : if osmesa
		// conditional = 2 : if glx

		if (p->conditional) {

			if (!(0
#ifdef HAVE_OSMESA
			   || (p->conditional == 1 && m_pCtx_OSMesa && m_use_glx == 0)
#endif
#ifdef HAVE_GLX
			   || (p->conditional == 2 && m_pCtx_GLX && m_use_glx)
#endif
			)) {
				p = p->next;
				line++;
				continue;
			}
		}
		if (p->execute > 0) p->execute--;
		switch (p->type) {

			case SM_GL_ARCXZ:
			case SM_GL_ARCYZ:
			case SM_GL_ARCXY:
				if (p->data && p->p3.slices && p->p7.aspect > 0.0) {

					ArcXZplot2(p);
				}
				break;
			case SM_GL_BEGIN:
				glBegin(p->p1.mode);
				begins++;
				break;
			case SM_GL_MATERIALV:
			case SM_GL_LIGHTV:
				{
				if (!m_vectors || p->p1.vector_id > m_max_vectors ||
					!m_vectors[p->p1.vector_id]) break;
				if (p->type == SM_GL_LIGHTV)
					SM_GL_OPER_LIGHTV(p->p3.lighti, p->p2.pname,
						m_vectors[p->p1.vector_id]->param);
				else if (p->type == SM_GL_MATERIALV)
#ifdef SM_OPENGL_PARM_TYPE_INT
				{
				// the vector might double, int or float and we need an array of float
				GLfloat color_index[4];
					for (int i=0 ; i < 4 ; i++)
						color_index[i] = (GLfloat) m_vectors[p->p1.vector_id]->param[i];
					glMaterialfv(p->p3.face, p->p2.pname, color_index);
						m_vectors[p->p1.vector_id]->param);
				}
#else
					glMaterialfv(p->p3.face, p->p2.pname,
						m_vectors[p->p1.vector_id]->param);
#endif
				else return -1;
				}
				break;
			case SM_GL_LIGHT:
				SM_GL_OPER_LIGHT(p->p3.lighti, p->p2.pname, p->p1.light_param);
				break;
			case SM_GL_BINDTEXTURE:
				if (!m_textures || p->p1.texture_id > m_max_textures ||
					!m_textures[p->p1.texture_id]) break;

				// If source is none, we delete the texture, otherwise
				// we load it and bind. The LoadTexture will determine
				// if an image actually needs to be loaded
				if (m_textures[p->p1.texture_id]->source == TEXTURE_SOURCE_NONE) {
					if (!m_textures[p->p1.texture_id]->real_texture_id) {
						if (m_verbose) fprintf(stderr, "Deleting texture %u "
							"glTexture %u\n", p->p1.texture_id,
							m_textures[p->p1.texture_id]->real_texture_id);
						glDeleteTextures(1,
							&(m_textures[p->p1.texture_id]->real_texture_id));
						m_textures[p->p1.texture_id]->real_texture_id = 0;
					} else break; //do nothing. glTexture already deleted
				} else {
					//if (!m_textures[p->p1.texture_id]->real_texture_id)
						if (LoadTexture(p->p1.texture_id,
							p->p3.param_i, p->p4.param_i)) break;
					if (!m_textures[p->p1.texture_id]->real_texture_id) break;
					//Tell OpenGL which texture to edit
					//glBindTexture(GL_TEXTURE_2D,
					glBindTexture(p->p2.pname,
						m_textures[p->p1.texture_id]->real_texture_id);
				}
				break;
			case SM_GL_BLENDFUNC:
				glBlendFunc(p->p1.sfactor, p->p2.dfactor);
				break;
			case SM_GL_CLEAR:
				glClear(p->p1.mode);
				break;
			case SM_GL_CLEARCOLOR:
				glClearColor(p->p1.red, p->p2.green, p->p3.blue, p->p4.alpha);
				break;
			case SM_GL_COLOR3:
				SM_GL_OPER_COLOR3(p->p1.red, p->p2.green, p->p3.blue);
				break;
			case SM_GL_COLOR4:
				SM_GL_OPER_COLOR4(p->p1.red, p->p2.green, p->p3.blue, color4[3]*p->p4.alpha);
				break;
			case SM_GL_DISABLE:
				glDisable(p->p1.mode);
				break;
			case SM_GL_ENABLE:
				glEnable(p->p1.mode);
				break;
			case SM_GL_END:
				glEnd();
				if (begins) begins--;
				break;
			case SM_GL_FINISH:
				glFinish();
				break;
			case SM_GL_FLUSH:
				glFlush();
				break;
			case SM_GL_GLSHAPE:
				ExecuteShape(p->p1.shape_id, --level, scale_x, scale_y, scale_z);
				break;
#ifdef HAVE_GLU
			case SM_GLU_PERSPECTIVE:
				gluPerspective(p->p1.fovy, p->p2.aspect, p->p3.zNear, p->p4.zFar);
				break;
#endif // #ifdef HAVE_GLU
			case SM_GL_LOADIDENTITY:
				glLoadIdentity();
				break;
			case SM_GL_MATRIXMODE:
				glMatrixMode(p->p1.mode);
				break;
			case SM_GL_NORMAL:
				SM_GL_OPER_NORMAL(p->p1.x, p->p2.y, p->p3.z);
				break;
#ifdef HAVE_GLU
			case SM_GLU_CYLINDER:
			case SM_GLU_DISK:
			case SM_GLU_DRAWSTYLE:
			case SM_GLU_NORMALS:
			case SM_GLU_ORIENTATION:
			case SM_GLU_PARTIALDISK:
			case SM_GLU_SPHERE:
			case SM_GLU_TEXTURE: {
				  GLUquadric* pQuad = FindGluQuadric(p->p1.quad_id);
				  if (!pQuad) pQuad = CreateGluQuadric(p->p1.quad_id);
				  if (!pQuad) break;		// We fail silently. FIXMEE
				  if (p->type == SM_GLU_NORMALS)
					gluQuadricNormals(pQuad, p->p2.normal);
				  else if (p->type == SM_GLU_TEXTURE)
					gluQuadricTexture(pQuad, p->p2.texture);
				  else if (p->type == SM_GLU_ORIENTATION)
					gluQuadricOrientation(pQuad, p->p2.orientation);
				  else if (p->type == SM_GLU_DRAWSTYLE)
					gluQuadricDrawStyle(pQuad, p->p2.draw);
				  else if (p->type == SM_GLU_SPHERE)
					gluSphere(pQuad, p->p2.radius, p->p3.slices, p->p4.stacks);
				  else if (p->type == SM_GLU_CYLINDER)
					gluCylinder(pQuad, p->p2.base, p->p5.top, p->p6.height,
						p->p3.slices, p->p4.stacks);
				  else if (p->type == SM_GLU_DISK)
					gluDisk(pQuad, p->p2.inner, p->p5.outer,
						p->p3.slices, p->p4.loops);
				  else if (p->type == SM_GLU_PARTIALDISK)
					gluPartialDisk(pQuad, p->p2.inner, p->p5.outer,
						p->p3.slices, p->p4.loops,
						p->p6.start, p->p7.sweep);
				}
				break;
#endif // #ifdef HAVE_GLU
			case SM_GL_ORTHO:
				glOrtho(p->p1.left, p->p2.right, p->p6.bottom,
					p->p5.top, p->p3.zNear, p->p4.zFar);
				break;
			case SM_GL_POPMATRIX:
				glPopMatrix();
				break;
			case SM_GL_PUSHMATRIX:
				glPushMatrix();
				break;
			case SM_GL_RECURSION:
				level = p->p1.level;
				break;
			case SM_GL_ROTATE:
#ifdef SM_OPENGL_PARM_TYPE_INT
				if (p->p4.anglef > 360) p->p4.anglef -= 360;
				else if (p->p4.anglef < -360) p->p4.anglef += 360;
				SM_GL_OPER_ROTATE(p->p4.anglef, p->p1.xf, p->p2.yf, p->p3.zf);
#else // float and double
				if (p->p4.angle > 360) p->p4.angle -= 360;
				else if (p->p4.angle < -360) p->p4.angle += 360;
				SM_GL_OPER_ROTATE(p->p4.angle, p->p1.x, p->p2.y, p->p3.z);
#endif
				break;
			case SM_GL_SCALE:
#ifdef SM_OPENGL_PARM_TYPE_INT
				SM_GL_OPER_SCALE(p->p1.xf, p->p2.yf, p->p3.zf);
				scale_x *= p->p1.xf; scale_y *= p->p2.yf; scale_z *= p->p3.zf;
#else // float and double
				SM_GL_OPER_SCALE(p->p1.x, p->p2.y, p->p3.z);
				scale_x *= p->p1.x; scale_y *= p->p2.y; scale_z *= p->p3.z;
#endif
				break;
			case SM_GL_SCISSOR:
				glScissor(p->p1.xi, p->p2.yi, p->p3.width, p->p4.height);
				break;
			case SM_GL_SHADEMODEL:
				glShadeModel(p->p1.mode);
				break;
			case SM_GL_TEXCOORD1:
				SM_GL_OPER_TEXCOORD1(p->p1.x);
				break;
			case SM_GL_TEXCOORD2:
				SM_GL_OPER_TEXCOORD2(p->p1.x, p->p2.y);
				break;
			case SM_GL_TEXCOORD3:
				SM_GL_OPER_TEXCOORD3(p->p1.x, p->p2.y, p->p3.z);
				break;
			case SM_GL_TEXCOORD4:
				SM_GL_OPER_TEXCOORD4(p->p1.x, p->p2.y, p->p3.z, p->p4.w);
				break;
			case SM_GL_TEXFILTER2D: {
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(p->p1.mode, GL_TEXTURE_MIN_FILTER, p->p3.param_i);
				glTexParameteri(p->p1.mode, GL_TEXTURE_MAG_FILTER, p->p4.param_i);
}
				break;
			case SM_GL_TRANSLATE:
#ifdef SM_OPENGL_PARM_TYPE_INT
				SM_GL_OPER_TRANSLATE(p->p1.xf, p->p2.yf, p->p3.zf);
#else // float and double
				SM_GL_OPER_TRANSLATE(p->p1.x, p->p2.y, p->p3.z);
#endif
				break;
			case SM_GL_VERTEX2:
				SM_GL_OPER_VERTEX2(p->p1.x, p->p2.y);
				break;
			case SM_GL_VERTEX3:
				SM_GL_OPER_VERTEX3(p->p1.x, p->p2.y, p->p3.z);
				break;
			case SM_GL_VERTEX4:
				SM_GL_OPER_VERTEX4(p->p1.x, p->p2.y, p->p3.z, p->p4.w);
				break;
			case SM_GL_VIEWPORT:
				glViewport(p->p1.xi, p->p2.yi, p->p3.width, p->p4.height);
				break;
			case SM_GL_VBO:
				if (m_vbos && p->p1.vbo_id < m_max_vbos && m_vbos[p->p1.vbo_id]) {
					if (!m_vbos[p->p1.vbo_id]->initialized)
						LoadVBO(m_vbos[p->p1.vbo_id]);
					ExecuteVBO(m_vbos[p->p1.vbo_id]);
				}
				break;
			case SM_GL_SNOWMIX:
				if (m_pVideoMixer && m_pVideoMixer->m_pController)
					m_pVideoMixer->m_pController->controller_parse_command(
						m_pVideoMixer, NULL, p->create_str);
				break;
#ifdef HAVE_GLUT
			case SM_GLUT_TEAPOT:
				glFrontFace(GL_CW);
				if (p->p2.solid) glutSolidTeapot(p->p1.size);
				else glutWireTeapot(p->p1.size);
				glFrontFace(GL_CCW);
				break;
#endif
			case SM_GL_NOOP:
				MakeArcXZ();
/*
				fprintf(stderr, "glshape %u "SM_OPENGL_PARM_PRINT
					SM_OPENGL_PARM_PRINT" "SM_OPENGL_PARM_PRINT"\n",
					shape_id, p->p1.x, p->p2.y, p->p3.z);
*/
				break;
			default:
				fprintf(stderr, "COpenglVideo::ExecuteShape unknown "
					"type for glshape %u line %d\n",shape_id, line);
		}
		// Its illegal to ask for glGetError with a begind-end
		// More that one error can be queued. Take them until no error is reached.
		// Print max one error per shape unless verbose > 1

		// Check and empty GLerrors. Line will be wrong as we have executed all lines
		if (!begins) {
			char str[44];
			sprintf(str, "in ExecuteShape(%u)", shape_id);
			CheckForGlError(m_verbose, str);
		}
		line++;
		p = p->next;
	}
	return 0;
}

int COpenglVideo::AddShape(u_int32_t shape_id, const char *name)
{
	if (!m_shapes) return 1;
	if (shape_id >= m_max_shapes || !name) return -1;
	while (isspace(*name)) name++;
	if (!(*name)) return -1;
	if (m_shapes[shape_id]) DeleteShape(shape_id);
	opengl_shape_t* pShape = m_shapes[shape_id] =
		(opengl_shape_t*) calloc(sizeof(opengl_shape_t)+strlen(name)+1,1);
	if (!pShape) return 1;
	pShape->name = ((char*)pShape)+sizeof(opengl_shape_t);
	strcpy(pShape->name,name);
	trim_string(pShape->name);
	pShape->id = shape_id;
	m_shape_count++;
	return 0;
}

int COpenglVideo::AddVector(u_int32_t vector_id, const char *name)
{
	if (!m_vectors) return 1;
	if (vector_id >= m_max_vectors || !name) return -1;
	while (isspace(*name)) name++;
	if (!(*name)) return -1;
	if (m_vectors[vector_id]) DeleteLight(vector_id);
	opengl_vector_t* pVector = m_vectors[vector_id] =
		(opengl_vector_t*) calloc(sizeof(opengl_vector_t)+strlen(name)+1,1);
	if (!pVector) return 1;
	pVector->name = ((char*)pVector)+sizeof(opengl_vector_t);
	strcpy(pVector->name,name);
	trim_string(pVector->name);
	pVector->id = vector_id;
	// pVector->n_params = 0;	// Implicit by calloc
	// pVector->param[0] = 0;	// Implicit by calloc
	// pVector->param[1] = 0;	// Implicit by calloc
	// pVector->param[2] = 0;	// Implicit by calloc
	// pVector->param[3] = 0;	// Implicit by calloc
	m_vector_count++;
	return 0;
}

int COpenglVideo::AddVectorValues(u_int32_t vector_id, u_int32_t count,
#ifdef SM_OPENGL_PARM_TYPE_INT
	GLint param0, GLint param1, GLint param2, GLint param3)
#else
	GLfloat param0, GLfloat param1, GLfloat param2, GLfloat param3)
#endif
{
	if (!m_vectors) return 1;
	if (vector_id >= m_max_vectors || !m_vectors[vector_id] ||
		count < 2 || count >4) return -1;
	m_vectors[vector_id]->n_params = count;
	// It is safe to add all 4 vectors as these are set to zero if they are not initialized
	// by the calling function
	m_vectors[vector_id]->param[0] = param0;
	m_vectors[vector_id]->param[1] = param1;
	m_vectors[vector_id]->param[2] = param2;
	m_vectors[vector_id]->param[3] = param3;
	return 0;
}

int COpenglVideo::AddTexture(u_int32_t texture_id, const char *name)
{
	if (!m_textures) return 1;
	if (texture_id >= m_max_textures || !name) return -1;
	while (isspace(*name)) name++;
	if (!(*name)) return -1;
	if (m_textures[texture_id]) DeleteTexture(texture_id);
	opengl_texture_t* pTexture = m_textures[texture_id] =
		(opengl_texture_t*) calloc(sizeof(opengl_texture_t)+strlen(name)+1,1);
	if (!pTexture) return 1;
	pTexture->name = ((char*)pTexture)+sizeof(opengl_texture_t);
	strcpy(pTexture->name,name);
	trim_string(pTexture->name);
	pTexture->id = texture_id;
	pTexture->source = TEXTURE_SOURCE_NONE;
	//pTexture->source_id = 0;	// implicit in calloc
	//pTexture->source_seq_no = 0;	// implicit in calloc
	m_texture_count++;
	return 0;
}

int COpenglVideo::SetTextureSource(u_int32_t texture_id, texture_source_enum_t source, u_int32_t source_id)
{
	if (!m_textures || !m_textures[texture_id]) return 1;
	if (texture_id >= m_max_textures) return -1;
	if (source != TEXTURE_SOURCE_NONE && source != TEXTURE_SOURCE_FEED &&
		source != TEXTURE_SOURCE_IMAGE && source != TEXTURE_SOURCE_FRAME) return -1;
	m_textures[texture_id]->source = source;
	m_textures[texture_id]->source_id = source_id;
	m_textures[texture_id]->source_seq_no = 0;
	return 0;
}

int COpenglVideo::set_shape_add(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_shapes || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		for (u_int32_t id=0 ; id < m_max_shapes; id++)
			if (m_shapes[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" glshape %u ops %u name %s\n",
					id, m_shapes[id]->ops_count, m_shapes[id]->name);
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"\n");
		return 0;
	}
	u_int32_t shape_id;
	int n;
	if (sscanf(str, "%u", &shape_id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_shapes) return 1;
		n = DeleteShape(shape_id);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"glshape %u deleted\n", shape_id);
		return n;
	}
	n = AddShape(shape_id, str);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape %u add %s\n", shape_id, str);
	return n;
}

int COpenglVideo::set_vector_add(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_vectors || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		for (u_int32_t id=0 ; id < m_max_vectors; id++)
			if (m_vectors[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" vector %u name %s\n",
					id, m_vectors[id]->name);
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"\n");
		return 0;
	}
	u_int32_t vector_id;
	int n;
	if (sscanf(str, "%u", &vector_id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_vectors) return 1;
		n = DeleteLight(vector_id);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"glshape vector %u deleted\n", vector_id);
		return n;
	}
	n = AddVector(vector_id, str);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape vector %u add %s\n", vector_id, str);
	return n;
}

int COpenglVideo::set_vector_value(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_vectors || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		for (u_int32_t id=0 ; id < m_max_vectors; id++)
			if (m_vectors[id]) {
				if (m_vectors[id]->n_params == 2)
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS" vector %u values "
#ifdef SM_OPENGL_PARM_TYPE_INT
						"%d %d"
#else
						"%f %f"
#endif
						" name %s\n", id,
						m_vectors[id]->param[0], m_vectors[id]->param[1],
						m_vectors[id]->name);
				else if (m_vectors[id]->n_params == 3)
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS" vector %u values "
#ifdef SM_OPENGL_PARM_TYPE_INT
						"%d %d %d"
#else
						"%f %f %f"
#endif
						" name %s\n", id,
						m_vectors[id]->param[0], m_vectors[id]->param[1],
						m_vectors[id]->param[2], m_vectors[id]->name);
				else if (m_vectors[id]->n_params == 4)
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS" vector %u values "
#ifdef SM_OPENGL_PARM_TYPE_INT
						"%d %d %d %d"
#else
						"%f %f %f %f"
#endif
						" name %s\n", id,
						m_vectors[id]->param[0], m_vectors[id]->param[1],
						m_vectors[id]->param[2], m_vectors[id]->param[3],
						m_vectors[id]->name);
				else m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" vector %u values none name %s\n", id,
					m_vectors[id]->name);
			}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"\n");
		return 0;
	}
	u_int32_t vector_id, count;
	int n;
#ifdef SM_OPENGL_PARM_TYPE_INT
	int param0, param1, param2, param3;
	if ((count = sscanf(str, "%u %d %d %d %d", &vector_id, &param0, &param1, &param2, &param3)) < 3) return -1;
#else
	GLfloat param0, param1, param2, param3;
	if ((count = sscanf(str, "%u %f %f %f %f", &vector_id, &param0, &param1, &param2, &param3)) < 3) return -1;
#endif
	count--;
	n = AddVectorValues(vector_id, count, param0, param1, param2, param3);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController) {
		if (count == 2) m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape vector %u values "
#ifdef SM_OPENGL_PARM_TYPE_INT
			"%d %d"
#else
			"%f %f"
#endif
			"\n", vector_id, param0, param1);
		else if (count == 3) m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape vector %u values "
#ifdef SM_OPENGL_PARM_TYPE_INT
			"%d %d %d"
#else
			"%f %f %f"
#endif
			"\n", vector_id, param0, param1, param2);
		else m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape vector %u values "
#ifdef SM_OPENGL_PARM_TYPE_INT
			"%d %d %d %d"
#else
			"%f %f %f %f"
#endif
			"\n", vector_id, param0, param1, param2, param3);
	}
	return n;
}

int COpenglVideo::set_texture_add(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_textures || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		for (u_int32_t id=0 ; id < m_max_textures; id++)
			if (m_textures[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" texture %u name %s\n",
					id, m_textures[id]->name);
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"\n");
		return 0;
	}
	u_int32_t texture_id;
	int n;
	if (sscanf(str, "%u", &texture_id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_textures) return 1;
		n = DeleteTexture(texture_id);
		if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"glshape texture %u deleted\n", texture_id);
		return n;
	}
	n = AddTexture(texture_id, str);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape texture %u add %s\n", texture_id, str);
	return n;
}

// glshape texture source [<texture id> (feed <feed id>|image <image id>)]
int COpenglVideo::set_texture_source(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_textures || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		for (u_int32_t id=0 ; id < m_max_textures; id++)
			if (m_textures[id]) {
				u_int32_t width = 0, height = 0;
				u_int32_t source_id = m_textures[id]->source_id;
				if (m_textures[id]->source == TEXTURE_SOURCE_FEED) {
					if (!m_pVideoMixer->m_pVideoFeed) return -1;
					m_pVideoMixer->m_pVideoFeed->FeedGeometry(source_id,
						&width, &height);
				} else if (m_textures[id]->source == TEXTURE_SOURCE_IMAGE) {
					if (!m_pVideoMixer->m_pVideoImage) return -1;
					image_item_t* pImage =
						m_pVideoMixer->m_pVideoImage->GetImageItem(source_id);
					if (!pImage) return -1;
					width = pImage->width;
					height = pImage->height;
				} else if (m_textures[id]->source == TEXTURE_SOURCE_FRAME) {
					m_pVideoMixer->SystemGeometry(&width, &height);
				} // else if (m_textures[id]->source == TEXTURE_SOURCE_NONE)
				  // implicit width = height = 0

				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" texture %u source %s %u Geometry %ux%u\n",
					id,
					m_textures[id]->source == TEXTURE_SOURCE_FEED ? "feed" :
					  m_textures[id]->source == TEXTURE_SOURCE_IMAGE ? "image" :
					    m_textures[id]->source == TEXTURE_SOURCE_FRAME ? "frame" :
					      m_textures[id]->source == TEXTURE_SOURCE_NONE ? "none" :
					        "unknown", m_textures[id]->source_id, width, height);
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS"\n");
		return 0;
	}
	u_int32_t texture_id, source_id;
	int n;
	texture_source_enum_t source = TEXTURE_SOURCE_NONE;
	if (sscanf(str, "%u", &texture_id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	if (!(strncasecmp(str, "feed ", 5))) source = TEXTURE_SOURCE_FEED;
	else if (!(strncasecmp(str, "image ", 6))) source = TEXTURE_SOURCE_IMAGE;
	else if (!(strncasecmp(str, "frame ", 6))) source = TEXTURE_SOURCE_FRAME;
	else if (!(strncasecmp(str, "none ", 5))) source = TEXTURE_SOURCE_NONE;
	else return -1;
	while (*str && !isspace(*str)) str++;
	while (isspace(*str)) str++;
	if (source != TEXTURE_SOURCE_NONE) {
		if ((sscanf(str, "%u", &source_id) != 1)) return -1;
	} else source_id = 0;
	n = SetTextureSource(texture_id, source, source_id);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape texture %u source %s %u\n", texture_id,
				m_textures[texture_id]->source == TEXTURE_SOURCE_FEED ? "feed" :
				  m_textures[texture_id]->source == TEXTURE_SOURCE_IMAGE ? "image" :
				    m_textures[texture_id]->source == TEXTURE_SOURCE_FRAME ? "frame" :
				      m_textures[texture_id]->source == TEXTURE_SOURCE_NONE ? "none" :
				        "unknown", m_textures[texture_id]->source_id);
	return n;
}

// Will return a copy of the command string. str is expected to point somewhere
// in the command string that begins with "glshape ......" meaning we search backwards.
// We then omit the the 'glshape ' part, as it is implicit for all.
char* COpenglVideo::DuplicateCommandString(const char* str)
{
	if (!str) return NULL;
	// We should search for "glshape " or "gl "
	// initially we subtract 8 which is the length of "gl "
	str -= 3;
	while (strncasecmp(str, "glshape ", 8) && strncasecmp(str, "gl ", 3)) str--;
	str += (strncasecmp(str, "gl ", 3)) ? 8 : 3;
	//while (isspace(*str)) str++;
	return strdup(str);
}


int COpenglVideo::AddToShapeCommand(u_int32_t shape_id, opengl_operation_enum_t type)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	opengl_draw_operation_t* p = (opengl_draw_operation_t*) calloc(sizeof(opengl_draw_operation_t),1);
	if (!p) return 1;
	p->type = type;
	p->execute = -1;
	//p->conditional = 0;	// implicit calloc
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;	// No memory leak. Allocated memory is freed when shape is
			// deleted. Deconstructor deletes all shapes
}

int COpenglVideo::set_shape_nummerical_parameters(struct controller_type* ctr, const char* str, opengl_operation_enum_t type)
{
	int n;
	u_int32_t shape_id;
	sm_opengl_parm_t parm1, parm2, parm3, parm4;
#ifdef SM_OPENGL_PARM_TYPE_INT
	float parmf1, parmf2, parmf3, parmf4;
#endif
	const char* create_str = str;
	if (!m_shapes) return 1;
	if (!str) return -1;
	if (type != SM_GL_TRANSLATE && type != SM_GL_SCALE && type != SM_GL_NORMAL &&
		type != SM_GL_COLOR && type != SM_GL_VERTEX && type != SM_GL_ROTATE &&
		type != SM_GL_TEXCOORD
		) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
#ifdef SM_OPENGL_PARM_TYPE_INT
	if (type == SM_GL_SCALE || type == SM_GL_ROTATE || type == SM_GL_TRANSLATE) // values are floats for sc
		n = sscanf(str, "%u %f %f %f %f", &shape_id, &parmf1, &parmf2, &parmf3, &parmf4);
	else n = sscanf(str, "%u "SM_OPENGL_PARM" "SM_OPENGL_PARM" "SM_OPENGL_PARM" "
			SM_OPENGL_PARM, &shape_id, &parm1, &parm2, &parm3, &parm4);
#else	// for GL float and double
	n = sscanf(str, "%u "SM_OPENGL_PARM" "SM_OPENGL_PARM" "SM_OPENGL_PARM" "
		SM_OPENGL_PARM, &shape_id, &parm1, &parm2, &parm3, &parm4);
#endif

	if (n < 2 || (n == 2 && type != SM_GL_VERTEX && type != SM_GL_TEXCOORD) ||
		(n == 3 && type != SM_GL_VERTEX && type != SM_GL_TEXCOORD) ||
		(n == 4 && type != SM_GL_VERTEX && type != SM_GL_TEXCOORD &&
		 type != SM_GL_TRANSLATE && type != SM_GL_SCALE &&
			type != SM_GL_NORMAL && type != SM_GL_COLOR) ||
		(n == 5 && type != SM_GL_COLOR && type != SM_GL_VERTEX &&
			type != SM_GL_ROTATE && type != SM_GL_TEXCOORD))
		return -1;
	if (type == SM_GL_COLOR) {
		if (n == 5) type = SM_GL_COLOR4;
		else if (n == 4) type = SM_GL_COLOR3;
		else return -1;
	} else if (type == SM_GL_VERTEX) {
		if (n == 5) type = SM_GL_VERTEX4;
		else if (n == 4) type = SM_GL_VERTEX3;
		else if (n == 3) type = SM_GL_VERTEX2;
		else return -1;
	} else if (type == SM_GL_TEXCOORD) {
		if (n == 5) type = SM_GL_TEXCOORD4;
		else if (n == 4) type = SM_GL_TEXCOORD3;
		else if (n == 3) type = SM_GL_TEXCOORD2;
		else if (n == 2) type = SM_GL_TEXCOORD1;
		else return -1;
	}


	if (shape_id >= m_max_shapes || !m_shapes[shape_id]) return -1;
	if ((n = AddToShapeCommand(shape_id, type))) return n;
	switch (type) {
		case SM_GL_VERTEX2:
		case SM_GL_VERTEX3:
		case SM_GL_VERTEX4:
		case SM_GL_TEXCOORD1:
		case SM_GL_TEXCOORD2:
		case SM_GL_TEXCOORD3:
		case SM_GL_TEXCOORD4:
		case SM_GL_NORMAL:
			m_shapes[shape_id]->pDrawTail->p1.x = parm1;
			if (type == SM_GL_TEXCOORD1) break;
			m_shapes[shape_id]->pDrawTail->p2.y = parm2;
			if (type == SM_GL_TEXCOORD2 || type == SM_GL_VERTEX2) break;
			m_shapes[shape_id]->pDrawTail->p3.z = parm3;
			if (type != SM_GL_VERTEX4 && type != SM_GL_TEXCOORD4) break;
			m_shapes[shape_id]->pDrawTail->p4.w = parm4;
			break;
		case SM_GL_COLOR3:
		case SM_GL_COLOR4:
			m_shapes[shape_id]->pDrawTail->p1.red = parm1;
			m_shapes[shape_id]->pDrawTail->p2.green = parm2;
			m_shapes[shape_id]->pDrawTail->p3.blue = parm3;
			if (type == SM_GL_COLOR4)
				m_shapes[shape_id]->pDrawTail->p4.alpha = parm4;
			break;
		case SM_GL_TRANSLATE:
#ifdef SM_OPENGL_PARM_TYPE_INT
			m_shapes[shape_id]->pDrawTail->p1.xf = parmf1;
			m_shapes[shape_id]->pDrawTail->p2.yf = parmf2;
			m_shapes[shape_id]->pDrawTail->p3.zf = parmf3;
#else // for GL functions double or float
			m_shapes[shape_id]->pDrawTail->p1.x = parm1;
			m_shapes[shape_id]->pDrawTail->p2.y = parm2;
			m_shapes[shape_id]->pDrawTail->p3.z = parm3;
#endif
			break;
		case SM_GL_SCALE:
#ifdef SM_OPENGL_PARM_TYPE_INT
			m_shapes[shape_id]->pDrawTail->p1.xf = parmf1;
			m_shapes[shape_id]->pDrawTail->p2.yf = parmf2;
			m_shapes[shape_id]->pDrawTail->p3.zf = parmf3;
#else // for GL functions double or float
			m_shapes[shape_id]->pDrawTail->p1.x = parm1;
			m_shapes[shape_id]->pDrawTail->p2.y = parm2;
			m_shapes[shape_id]->pDrawTail->p3.z = parm3;
#endif
			break;
		case SM_GL_ROTATE:
#ifdef SM_OPENGL_PARM_TYPE_INT
			m_shapes[shape_id]->pDrawTail->p4.anglef = parmf1;
			m_shapes[shape_id]->pDrawTail->p1.xf = parmf2;
			m_shapes[shape_id]->pDrawTail->p2.yf = parmf3;
			m_shapes[shape_id]->pDrawTail->p3.zf = parmf4;
#else // for GL functions double or float
			m_shapes[shape_id]->pDrawTail->p4.angle = parm1;
			m_shapes[shape_id]->pDrawTail->p1.x = parm2;
			m_shapes[shape_id]->pDrawTail->p2.y = parm3;
			m_shapes[shape_id]->pDrawTail->p3.z = parm4;
#endif
			break;
		default:
			fprintf(stderr, "Unexpected missses in COpenglVideo::"
				"set_shape_nummerical_parameters <%s>\n", create_str);
			return -1;
			break;
	}
	return 0;
}

#define SetVar(a,no,var1,var2,var3,var4,val)	\
	if (no == 1) a->var1 = val;		\
	else if (no == 2) a->var2 = val;	\
	else if (no == 3) a->var3 = val;	\
	else if (no == 4) a->var4 = val;	\
	else return -1;


int COpenglVideo::ModifyEntry(opengl_draw_operation_t* pDraw, u_int32_t param_no, const char* value) {
	if (!pDraw || param_no < 1 || !value) return -1;
	sm_opengl_parm_t param;
	opengl_draw_operation_t Draw;		// Make a copy for ARCXZ, ARCYZ, ARCXY
	bool fail = false;
	if (pDraw->type == SM_GL_ARCXZ || pDraw->type == SM_GL_ARCYZ || pDraw->type == SM_GL_ARCXY)
		memcpy(&Draw, pDraw, sizeof (Draw));
	switch (pDraw->type) {
		case SM_GL_SCALE:
		case SM_GL_TRANSLATE: {
			if (param_no > 3) return -1;
#ifdef SM_OPENGL_PARM_TYPE_INT
			float paramf;
			if (sscanf(value, SM_OPENGL_PARM_ROTATE_SCALE, &paramf) != 1) return -1;
			SetVar(pDraw, param_no, p1.xf, p2.yf, p3.zf, p4.anglef, paramf);
#else
			if (sscanf(value, SM_OPENGL_PARM, &param) != 1) return -1;
			SetVar(pDraw, param_no, p1.x, p2.y, p3.z, p4.angle, param);
#endif
			}
			break;
		case SM_GL_ROTATE: {
			if (param_no > 4) return -1;
			// 1->4 2->1 3->2 4->3
			param_no = (param_no + 2) % 4 + 1;
#ifdef SM_OPENGL_PARM_TYPE_INT
			float paramf;
			if (sscanf(value, SM_OPENGL_PARM_ROTATE_SCALE, &paramf) != 1) return -1;
			SetVar(pDraw, param_no, p1.xf, p2.yf, p3.zf, p4.anglef, paramf);
#else
			if (sscanf(value, SM_OPENGL_PARM, &param) != 1) return -1;
			SetVar(pDraw, param_no, p1.x, p2.y, p3.z, p4.angle, param);
#endif
			}
			break;
		case SM_GL_COLOR3:
			if (param_no > 3) return -1;
		case SM_GL_COLOR4:
			if (param_no > 4) return -1;
			if (sscanf(value, SM_OPENGL_PARM, &param) != 1) return -1;
			SetVar(pDraw, param_no, p1.red, p2.green, p3.blue, p4.alpha, param);
			break;
		case SM_GL_TEXCOORD1:
			if (param_no > 1) return -1;
		case SM_GL_TEXCOORD2:
		case SM_GL_VERTEX2:
			if (param_no > 2) return -1;
		case SM_GL_NORMAL:
		case SM_GL_TEXCOORD3:
		case SM_GL_VERTEX3:
			if (param_no > 3) return -1;
		case SM_GL_TEXCOORD4:
		case SM_GL_VERTEX4:
			if (param_no > 4) return -1;
			if (sscanf(value, SM_OPENGL_PARM, &param) != 1) return -1;
			SetVar(pDraw, param_no, p1.x, p2.y, p3.z, p4.w, param);
			break;
		case SM_GLU_PERSPECTIVE:
			if (param_no > 4) return -1;
			{
				double *pDouble = param_no == 1 ? &(pDraw->p1.fovy) :
					param_no == 2 ? &(pDraw->p2.aspect) :
					  param_no == 3 ? &(pDraw->p3.zNear) :
					    &(pDraw->p4.zFar);
				if (sscanf(value, "%lf", pDouble) != 1) return -1;
			}
			break;
		case SM_GL_ARCXZ:
		case SM_GL_ARCYZ:
		case SM_GL_ARCXY:
			// glshape arcxz <glshape id> <angle> <aspect> <slices>
			//	<texleft> <texright> <textop> <texbottom>\n"
			if (param_no > 7) return -1;
			if (param_no == 3) {
				u_int32_t slices;
				if (sscanf(value, "%u", &slices) != 1 || !slices) {
					fail = true;
					break;
				}
				Draw.p3.slices = slices;
			} else {
				double *pDouble = param_no == 1 ? &(Draw.p4.angled) :
					param_no == 2 ? &(Draw.p7.aspect) :
					  param_no == 4 ? &(Draw.p1.left) :
					    param_no == 5 ? &(Draw.p2.right) :
					      param_no == 6 ? &(Draw.p5.top) :
					        param_no == 7 ? &(Draw.p6.bottom) : NULL;
				if (!pDouble || sscanf(value, "%lf", pDouble) != 1) {
					fail = true;
					break;
				}
			}
			break;
		case SM_GL_ORTHO:
			if (param_no > 6) return -1;
			{
				double *pDouble = param_no == 1 ? &(pDraw->p1.left) :
					param_no == 2 ? &(pDraw->p2.right) :
					  param_no == 3 ? &(pDraw->p6.bottom) :
					    param_no == 4 ? &(pDraw->p5.top) :
					      param_no == 5 ? &(pDraw->p3.zNear) :
					        &(pDraw->p4.zFar);
				if (sscanf(value, "%lf", pDouble) != 1) return -1;
			}
			break;
		case SM_GL_SCISSOR:
		case SM_GL_VIEWPORT:
			if (param_no > 4) return -1;
			{
				int* pInt = param_no == 1 ? &(pDraw->p1.xi):
					param_no == 2 ? &(pDraw->p2.yi):
					  param_no == 3 ? &(pDraw->p3.width):
					    &(pDraw->p4.height);
				if (sscanf(value, "%d", pInt) != 1) return -1;
			}
			break;
		default:
			fprintf(stderr, "ModifyShape unsupported entry param_no %u value <%s>\n", param_no, value);
			return -1;
			break;
	}
	if (fail) return -1;
	if (pDraw->type == SM_GL_ARCXZ || pDraw->type == SM_GL_ARCYZ || pDraw->type == SM_GL_ARCXY) {
		// Check angle
		if (Draw.p7.aspect == 0.0 || Draw.p4.angled == 0.0 ||
			Draw.p4.angled / Draw.p3.slices == 0.0 ||
			Draw.p4.angled > 180.0 || Draw.p4.angled < -180.0) return -1;
		// Check tex
		if (Draw.p1.left < 0.0 || Draw.p1.left > 1.0 ||
			Draw.p2.right < 0.0 || Draw.p2.right > 1.0) return -1;
		if (Draw.p5.top < 0.0 || Draw.p5.top > 1.0 ||
			Draw.p6.bottom < 0.0 || Draw.p6.bottom > 1.0) return -1;
		curve2d_set_t* data = ArcXZcalculate(pDraw->type, Draw.p4.angled, Draw.p7.aspect, Draw.p3.slices,
			Draw.p1.left, Draw.p2.right);
		if (!data) return -1;
if (m_verbose) fprintf(stderr, "%s data updated\n",
	pDraw->type == SM_GL_ARCXZ ? "ArcXZ" :
	  pDraw->type == SM_GL_ARCYZ ? "ArcYZ" :
	  pDraw->type == SM_GL_ARCXY ? "ArcXY" : "ArcUnknown");
		if (pDraw->data) free(pDraw->data);
		memcpy(pDraw, &Draw, sizeof(Draw));
		pDraw->data = (u_int8_t*) data;
		if (pDraw->pVbo) {
			AddDataPointsVBO(pDraw->pVbo, 0, NULL);		// Deletes data points if any
			AddIndicesVBO(pDraw->pVbo, 0, NULL);	// Delete indices if any
		}
	}
	return 0;
}

int COpenglVideo::ActivateShapeEntry(u_int32_t shape_id, u_int32_t line, int32_t execute) {
	if (!m_shapes) return 1;
	if (shape_id >= m_max_shapes || !m_shapes[shape_id] ||
		line > m_shapes[shape_id]->ops_count) return -1;

	// Find entry in shape and return if not found
	opengl_draw_operation_t* p = m_shapes[shape_id]->pDraw;
	for (u_int32_t i = 1; i < line ; i++) p = p->next;
	if (!p) return -1;
	p->execute = execute;
	return 0;
}

int COpenglVideo::ModifyShapeEntryValues(u_int32_t shape_id, u_int32_t line, const char* numbers, const char* values) {
	u_int32_t number;
	if (!m_shapes) return 1;
	if (shape_id >= m_max_shapes || !m_shapes[shape_id] || !numbers || !values ||
		line > m_shapes[shape_id]->ops_count) return -1;

	// Find entry in shape and return if not found
	opengl_draw_operation_t* p = m_shapes[shape_id]->pDraw;
	for (u_int32_t i = 1; i < line ; i++) p = p->next;
	if (!p) return -1;

//fprintf(stderr, "Running through number list <%s> <%s>\n", numbers, values);
	// Now we scan through number list and call to modify
	while (sscanf(numbers, "%u", &number) == 1) {
//fprintf(stderr, "Got number %u\n", number);
		int n = ModifyEntry(p, number, values);
		if (n) return -1;
		while (*values && !isspace(*values)) values++;
		while (isspace(*values)) values++;
		while (isdigit(*numbers)) numbers++;
		if (*numbers == ' ') break;
		if (*numbers == ',' && isdigit(*(numbers+1))) { numbers++; }
		else return -1;
	}
	return 0;
}

// begin %u (points | lines | line_strip | line_loop | triangles | triangle_strip | triangle_fan | quads | quad_strip | polygon)
// enable %u (blend | ...)
// disable %u (blend | ...)
// matrixmode %u (projection | modelview)
// clear %u (color | depth | color+depth)
int COpenglVideo::set_shape_enum_or_int_parameters(struct controller_type* ctr, const char* str, opengl_operation_enum_t type)
{
	int n = 0;
	u_int32_t shape_id;
	GLenum mode;
	const char* create_str = str;
	if (!m_shapes) return 1;
	if (!str) return -1;
/*
	if (type != SM_GL_BEGIN && type != SM_GL_ENABLE && type != SM_GL_DISABLE &&
		type != SM_GL_MATRIXMODE && type != SM_GL_CLEAR && type != SM_GL_GLSHAPE &&
		type != SM_GL_TEXFILTER2D && type != SM_GL_BINDTEXTURE &&
		type != SM_GL_BLENDFUNC && type != SM_GLU_PERSPECTIVE &&
		type != SM_GL_VIEWPORT && type != SM_GL_MODIFY && type != SM_GL_RECURSION &&
		type != SM_GLU_NORMALS && type != SM_GLU_TEXTURE &&
		type != SM_GLU_ORIENTATION && type != SM_GLU_DRAWSTYLE &&
		type != SM_GLU_SPHERE && type != SM_GL_SNOWMIX && type != SM_GLU_SPHERE &&
		type != SM_GLU_CYLINDER && type != SM_GL_LIGHTV && type != SM_GL_MATERIALV &&
		type != SM_GL_SCISSOR || SM_GL_LIGHT || SM_GLUT_TEAPOT || GL_SHADE_MODEL ||
		SM_GLU_DISK || SM_GLU_PARTIALDISK
*/
/*
		&& type != SM_GL_TEXPARAMETERI && type != SM_GL_TEXPARAMETERF
		) return -1;
*/
	while (isspace(*str)) str++;
	if (sscanf(str, "%u", &shape_id) != 1 || shape_id >= m_max_shapes ||
		!m_shapes[shape_id]) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	switch (type) {
	  case SM_GL_GLSHAPE:
		{
			u_int32_t id;
			if (sscanf(str, "%u", &id) != 1 || id >= m_max_shapes ||
				!m_shapes[id]) return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.shape_id = id;
		}
		break;
	  case SM_GL_BEGIN:
		{
			int i;
			for (i=0; gl_begin_list[i].name; i++) {
				if (!strncmp(str, gl_begin_list[i].name,
					gl_begin_list[i].len)) break;
			}
			if (!gl_begin_list[i].name) return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			if (!m_shapes || shape_id >= m_max_shapes ||
				!m_shapes[shape_id] || !m_shapes[shape_id]->pDrawTail) {
				fprintf(stderr, "Unexpected missses in COpenglVideo::"
					"set_shape_enum_or_int_parameters <%s>\n", create_str);
				return -1;
			}
			m_shapes[shape_id]->pDrawTail->p1.mode =  gl_begin_list[i].mode;
		}
		break;
	  case SM_GL_ENABLE:
	  case SM_GL_DISABLE:
		{
			int i;
			for (i=0; gl_enable_mode_list[i].name; i++) {
				if (!strncmp(str, gl_enable_mode_list[i].name,
					gl_enable_mode_list[i].len)) break;
			}
			if (!gl_enable_mode_list[i].name) return -1;
			mode = gl_enable_mode_list[i].mode;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			if (!m_shapes || shape_id >= m_max_shapes ||
				!m_shapes[shape_id] || !m_shapes[shape_id]->pDrawTail) {
				fprintf(stderr, "Unexpected missses in COpenglVideo::"
					"set_shape_enum_or_int_parameters <%s>\n", create_str);
				return -1;
			}
			m_shapes[shape_id]->pDrawTail->p1.mode = mode;
		}
		break;
	  case SM_GL_ENTRY: {
			int n;
			int32_t execute;
			u_int32_t line;
			const char* create_str = str;;
			if ((n = sscanf(str, "%u %d", &line, &execute)) < 1) return -1;
			if (n == 1) {
			  while (isdigit(*str)) str++;
			  while (isspace(*str)) str++;
			  if (!strncasecmp(str, "act", 3)) execute = -1;
			  else if (!strncasecmp(str, "inact", 5)) execute = 0;
			  else return -1;
			}
			n = ActivateShapeEntry(shape_id, line, execute);
			if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"glshape %u %s\n", shape_id, create_str);
			return n;
		}
		break;
	  case SM_GL_MODIFY:
		{
			u_int32_t line;
			if (sscanf(str, "%u", &line) != 1) return -1;
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
			const char* values = str;
			while (*values && !isspace(*values)) values++;
			while (isspace(*values)) values++;
			if (values == str) return -1;
			return ModifyShapeEntryValues(shape_id, line, str, values);
		}
		break;
	  case SM_GL_RECURSION:
		{
			u_int32_t level;
			if (sscanf(str, "%u", &level) != 1) return 1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			if (!m_shapes || shape_id >= m_max_shapes ||
				!m_shapes[shape_id] || !m_shapes[shape_id]->pDrawTail) {
				fprintf(stderr, "Unexpected missses in COpenglVideo::"
					"set_shape_enum_or_int_parameters <%s>\n", create_str);
			return -1;
			}
			m_shapes[shape_id]->pDrawTail->p1.level = level;
		}
		break;
	  case SM_GL_MATRIXMODE:
		if (!strncmp(str,"pro",3)) mode = GL_PROJECTION;
		else if (!strncmp(str,"mod",3)) mode = GL_MODELVIEW;
		else if (!strncmp(str,"tex",3)) mode = GL_TEXTURE;
		else if (!strncmp(str,"col",5)) mode = GL_COLOR;
		else return -1;
		if ((n = AddToShapeCommand(shape_id, type))) return n;
		if (!m_shapes || shape_id >= m_max_shapes ||
			!m_shapes[shape_id] || !m_shapes[shape_id]->pDrawTail) {
			fprintf(stderr, "Unexpected missses in COpenglVideo::"
				"set_shape_enum_or_int_parameters <%s>\n", create_str);
			return -1;
		}
		m_shapes[shape_id]->pDrawTail->p1.mode = mode;
		break;
	  case SM_GL_CLEAR:
		if (strcasestr(str,"depth") && strcasestr(str,"color"))
			mode = GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT;
		else if (!strncmp(str,"color",5)) mode = GL_COLOR_BUFFER_BIT;
		else if (!strncmp(str,"depth",5)) mode = GL_DEPTH_BUFFER_BIT;
		else return -1;
		if ((n = AddToShapeCommand(shape_id, type))) return n;
		if (!m_shapes || shape_id >= m_max_shapes ||
			!m_shapes[shape_id] || !m_shapes[shape_id]->pDrawTail) {
			fprintf(stderr, "Unexpected missses in COpenglVideo::"
				"set_shape_enum_or_int_parameters <%s>\n", create_str);
			return -1;
		}
		m_shapes[shape_id]->pDrawTail->p1.mode = mode;
		break;
	  case SM_GL_LIGHT:
	  case SM_GL_LIGHTV:
		{
			int i;
			u_int32_t vector_id, gllight_id, n_params;
			//char gllight[8];  // "lighti"
			GLenum lighti;

			// We are looking for the string : "light" followed by a int between 0-7
			if (strncasecmp(str, "light", 5)) return -1;
			if (sscanf(str, "light%u", &gllight_id) != 1 || gllight_id > 7) return -1;
			//sprintf(gllight,"light%u ",gllight_id);
			// Searching for light0, light1..light7
			for (i=0 ; gl_enable_mode_list[i].name; i++) {
				if (!strncasecmp(str, gl_enable_mode_list[i].name,
					gl_enable_mode_list[i].len)) break;
			}
			if (!gl_enable_mode_list[i].name) return -1;
			lighti = gl_enable_mode_list[i].mode;
			str += 6;
			if (!isspace(*str)) return -1;
			while (isspace(*str)) str++;

			// Searching for ambient, diffuse ...
			for (i=0 ; gl_light_parm_list[i].name; i++) {
				if (!strncasecmp(str, gl_light_parm_list[i].name,
					gl_light_parm_list[i].len)) break;
			}
			if (!gl_light_parm_list[i].name) return -1;
			mode = gl_light_parm_list[i].mode;
			str += gl_light_parm_list[i].len;
			n_params = gl_light_parm_list[i].n_params;
			//while (*str && !isspace(*str)) str++;

			while (isspace(*str)) str++;

			if (type == SM_GL_LIGHT) {
#ifdef SM_OPENGL_PARM_TYPE_INT
				int param;
				if (sscanf(str, "%d", &param) != 1) return -1;
#else
				float param;
				if (sscanf(str, "%f", &param) != 1) return -1;
#endif
				if ((n = AddToShapeCommand(shape_id, type))) return n;
				m_shapes[shape_id]->pDrawTail->p1.light_param = param;
			} else if (type == SM_GL_LIGHTV) {
				// Getting the defined vector id
				if (sscanf(str, "%u", &vector_id) != 1 ||
					vector_id >= m_max_vectors ||
					!m_vectors[vector_id]) return -1;

				// We need to check if the vector has the necessary number
				// of parameters
				if (n_params > m_vectors[vector_id]->n_params) {
					const char* parm = NULL;
					Mode2String(parm, mode, gl_light_parm_list);
					if (m_verbose) fprintf(stderr, "Warning: lightv "
						"with parameter %s require a vector wih %d "
						"components but vector %d only has %d\n",
						parm ? parm : "unknown", n_params, vector_id,
						m_vectors[vector_id]->n_params);
					return -1;
				}
				if ((n = AddToShapeCommand(shape_id, type))) return n;
				m_shapes[shape_id]->pDrawTail->p1.vector_id = vector_id;
			} else return -1;
			m_shapes[shape_id]->pDrawTail->p3.lighti = lighti;
			m_shapes[shape_id]->pDrawTail->p2.pname = mode;
		}
		break;
	  case SM_GL_MATERIALV:
		{
			int i;
			u_int32_t vector_id, n_params;
			GLenum face;
			// Searching for back, front ...
			for (i=0 ; gl_material_face_list[i].name; i++) {
				if (!strncmp(str, gl_material_face_list[i].name,
					gl_material_face_list[i].len)) break;
			}
			if (!gl_material_face_list[i].name) return -1;
			face = gl_material_face_list[i].mode;
			str += gl_material_face_list[i].len;
			//while (*str && !isspace(*str)) str++;
			while (isspace(*str)) str++;

			// Searching for ambient, diffuse ...
			for (i=0 ; gl_material_parm_list[i].name; i++) {
				if (!strncmp(str, gl_material_parm_list[i].name,
					gl_material_parm_list[i].len)) break;
			}
			if (!gl_material_parm_list[i].name) return -1;
			mode = gl_material_parm_list[i].mode;
			str += gl_material_parm_list[i].len;
			n_params = gl_material_parm_list[i].n_params;
			//while (*str && !isspace(*str)) str++;

			// Getting the defined vector id
			while (isspace(*str)) str++;
			if (sscanf(str, "%u", &vector_id) != 1 ||
				vector_id >= m_max_vectors ||
				!m_vectors[vector_id]) return -1;
			// We need to check if the vector has the necessary number
			// of parameters
			if (n_params > m_vectors[vector_id]->n_params) {
				const char* parm = NULL;
				Mode2String(parm, mode, gl_material_parm_list);
				if (m_verbose) fprintf(stderr, "Warning: materialv "
					"with parameter %s require a vector wih %d "
					"components but vector %d only has %d\n",
					parm ? parm : "unknown", n_params, vector_id,
					m_vectors[vector_id]->n_params);
				return -1;
			}
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.vector_id = vector_id;
			m_shapes[shape_id]->pDrawTail->p2.pname = mode;
			m_shapes[shape_id]->pDrawTail->p3.face = face;
		}
		break;
	  case SM_GL_BINDTEXTURE:
		{
			// glshape texture bind <glshape> <texture id> [<min> <mag>] [2d | cube]
			// min = nearest | linear | nearest_mipmap_nearest | linear_mipmap_nearest 
			//       nearest_mipmap_linear | linear_mipmap_linear
			//
			u_int32_t texture_id;
			GLenum texture = GL_TEXTURE_2D, min = GL_NEAREST, mag = GL_NEAREST; 

			if (sscanf(str, "%u", &texture_id) != 1 || texture_id >= m_max_textures ||
				!m_textures[texture_id]) return -1;
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
			while (*str && *str != '#') {
				while (isspace(*str)) str++;
				if (!(*str) || *str == '#') break;
				if (!strncmp(str, "nearest_mipmap_nearest", 22))
					min = (GLint) GL_NEAREST_MIPMAP_NEAREST;
				else if (!strncmp(str, "linear_mipmap_nearest", 21))
					min = (GLint) GL_LINEAR_MIPMAP_NEAREST;
				else if (!strncmp(str, "nearest_mipmap_linear", 21))
					min = (GLint) GL_NEAREST_MIPMAP_LINEAR;
				else if (!strncmp(str, "linear_mipmap_linear", 20))
					min = (GLint) GL_LINEAR_MIPMAP_LINEAR;
				else if (!strncmp(str, "near", 4)) min = (GLint) GL_NEAREST;
				else if (!strncmp(str, "lin", 3)) min = (GLint) GL_LINEAR;
				else if (!(strncasecmp(str, "cube ",5))) {
					texture = GL_TEXTURE_CUBE_MAP;
					break;
				} else if (!(strncasecmp(str, "2d ",5))) {
					texture = GL_TEXTURE_2D;
					break;
				} else return -1;
				while (*str && !isspace(*str)) str++;
				while (isspace(*str)) str++;
				if (!strncmp(str, "near", 4)) mag = (GLint) GL_NEAREST;
				else if (!strncmp(str, "lin", 3)) mag = (GLint) GL_LINEAR;
				else return -1;
				while (*str && !isspace(*str)) str++;
			}
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.texture_id = texture_id;
			m_shapes[shape_id]->pDrawTail->p2.pname = texture;
			m_shapes[shape_id]->pDrawTail->p3.param_i = min;
			m_shapes[shape_id]->pDrawTail->p4.param_i = mag;
		}
		break;
#ifdef HAVE_GLU
	  case SM_GLU_PERSPECTIVE:
		{
			double fovy, aspect, zNear, zFar;
			if ((sscanf(str, "%lf %lf %lf %lf", &fovy, &aspect, &zNear, &zFar) != 4))
				return -1;
			if (aspect <= 0) {
				aspect = (double) m_pVideoMixer->GetSystemWidth()/
					m_pVideoMixer->GetSystemHeight();
				fprintf(stderr, "Setting aspect for gluperspective to"
					" %u/%u = %.5lf\n", m_pVideoMixer->GetSystemWidth(),
				       	m_pVideoMixer->GetSystemHeight(), aspect);
			}
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.fovy = fovy;
			m_shapes[shape_id]->pDrawTail->p2.aspect = aspect;
			m_shapes[shape_id]->pDrawTail->p3.zNear = zNear;
			m_shapes[shape_id]->pDrawTail->p4.zFar = zFar;
		}
		break;
#endif // #ifdef HAVE_GLU

				// glshape if glx <glshape command>
	  case SM_GL_IFGLX: 
	  case SM_GL_IFOSMESA: {
			if (shape_id >= m_max_shapes || !m_shapes[shape_id]) return -1;
			u_int32_t count = m_shapes[shape_id]->ops_count;
			if ((n = ParseCommand(ctr,str)) || count == m_shapes[shape_id]->ops_count)
				return -1;
			m_shapes[shape_id]->pDrawTail->conditional =
				(type == SM_GL_IFOSMESA ? 1 : 2);
		}
		break;
	  case SM_GL_VBO: {
			u_int32_t vbo_id;
			if (sscanf(str, "%u", &vbo_id) != 1) return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.vbo_id = vbo_id;
		}
		break;
	  case SM_GL_SNOWMIX: {
			//while (*str && !isspace(*str)) str++;
			//while (isspace(*str)) str++;
			if (!(*str)) return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			int len = strlen(str);
			m_shapes[shape_id]->pDrawTail->create_str = (char*) malloc(len+2);
			if (!(m_shapes[shape_id]->pDrawTail->create_str)) return -1;
			strcpy(m_shapes[shape_id]->pDrawTail->create_str, str);
			m_shapes[shape_id]->pDrawTail->create_str[len] =' ';
			m_shapes[shape_id]->pDrawTail->create_str[len+1] ='\0';
			return 0; // We need to return because we don't want the DuplicateString at the end of this
		}
		break;
	  case SM_GL_ARCXZ: 
	  case SM_GL_ARCYZ: 
	  case SM_GL_ARCXY: {
			// glshape arcxz <glshape id> <angle> <aspect> <slices>
			//	<texleft> <texright> <textop> <texbottom>\n"
			double angle, aspect, left, right, top, bottom;
			u_int32_t slices;
			if (sscanf(str, "%lf %lf %u %lf %lf %lf %lf", &angle, &aspect, &slices, &left, &right, &top, &bottom) != 7) return -1;
			// Check slices, aspect and angl
			if (!slices || aspect == 0.0 || angle == 0.0 ||
				angle / slices == 0.0 || angle > 180.0 || angle < -180.0)
				return -1;
			if (left < 0.0 || left > 1.0 || right < 0.0 || right > 1.0) return -1;
			if (top < 0.0 || top > 1.0 || bottom < 0.0 || bottom > 1.0) return -1;
			curve2d_set_t* data = ArcXZcalculate(type, angle, aspect, slices, left, right);
			if (!data) return -1;
			char name[25];
			sprintf(name, "arcxz glshape %u", shape_id);
			opengl_vbo_t* pVbo = MakeVBO(name,0);
			if (!pVbo) {
				fprintf(stderr, "Failed to create VBO for %s\n",
					type == SM_GL_ARCXZ ? "ArcXZ" :
					  type == SM_GL_ARCYZ ? "ArcYZ" :
					  type == SM_GL_ARCXY ? "ArcXY" : "ArcUnknown");
				return -1;
			} else {
				if (ConfigVBO(pVbo, GL_QUADS, "t2 v3", GL_DYNAMIC_DRAW)) {
					fprintf(stderr, "Failed to configure VBO for %s\n",
						type == SM_GL_ARCXZ ? "ArcXZ" :
						  type == SM_GL_ARCYZ ? "ArcYZ" :
						  type == SM_GL_ARCXY ? "ArcXY" : "ArcUnknown");
					DestroyVBO(pVbo);
					return -1;
				}
			}
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.left = left;
			m_shapes[shape_id]->pDrawTail->p2.right = right;
			m_shapes[shape_id]->pDrawTail->p3.slices = slices;
			m_shapes[shape_id]->pDrawTail->p4.angled = angle;
			m_shapes[shape_id]->pDrawTail->p5.top = top;
			m_shapes[shape_id]->pDrawTail->p6.bottom = bottom;
			m_shapes[shape_id]->pDrawTail->p7.aspect = aspect;
			m_shapes[shape_id]->pDrawTail->data = (u_int8_t*) data;
			m_shapes[shape_id]->pDrawTail->pVbo = pVbo;
		}
		break;
	  case SM_GL_ORTHO: {
			GLdouble left, right, bottom, top, near, far;
			if (sscanf(str,"%lf %lf %lf %lf %lf %lf", &left, &right, &bottom, &top, &near, &far) != 6)
				return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.left = left;
			m_shapes[shape_id]->pDrawTail->p2.right = right;
			m_shapes[shape_id]->pDrawTail->p6.bottom = bottom;
			m_shapes[shape_id]->pDrawTail->p5.top = top;
			m_shapes[shape_id]->pDrawTail->p3.zNear = near;
			m_shapes[shape_id]->pDrawTail->p4.zFar = far;
		}
		break;
	  case SM_GL_SCISSOR:
	  case SM_GL_VIEWPORT: {
#define enum2string2(a,b,c,d,e,f) (a == b ? c : a == d ? e : f)
			int x, y, width, height;
			if ((sscanf(str, "%d %d %d %d", &x, &y, &width, &height) != 4) ||
				x < 0 || y < 0) return -1;
			if (width < 1) {
				if (m_verbose) fprintf(stderr, "Setting width for glshape "
					"%s to %u\n",
					enum2string2(type,SM_GL_VIEWPORT,"viewport",SM_GL_SCISSOR,"scissor","unknown"),
					m_pVideoMixer->GetSystemWidth());
				width = m_pVideoMixer->GetSystemWidth();
			}
			if (height < 1) {
				if (m_verbose) fprintf(stderr, "Setting height for glshape "
					"%s to %u\n",
					enum2string2(type,SM_GL_VIEWPORT,"viewport",SM_GL_SCISSOR,"scissor","unknown"),
					m_pVideoMixer->GetSystemHeight());
				height = m_pVideoMixer->GetSystemHeight();
			}
			if (height < 1) height = m_pVideoMixer->GetSystemHeight();
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.xi = x;
			m_shapes[shape_id]->pDrawTail->p2.yi = y;
			m_shapes[shape_id]->pDrawTail->p3.width = width;
			m_shapes[shape_id]->pDrawTail->p4.height = height;
		}
		break;
	  case SM_GL_SHADEMODEL: {
			if (!strncasecmp(str, "flat ", 5)) mode = GL_FLAT;
			else if (!strncasecmp(str, "smooth ", 7)) mode = GL_SMOOTH;
			else return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.mode =  mode;
		}
		break;
	  case SM_GL_BLENDFUNC:
		{
			GLenum sfactor, dfactor;
			int i;
			for (i=0; gl_blendfunc_list[i].name; i++) {
				if (!strncmp(str, gl_blendfunc_list[i].name,
					gl_blendfunc_list[i].len)) break;
			}
			if (!gl_blendfunc_list[i].name) return -1;
			sfactor = gl_blendfunc_list[i].mode;
			while (*str && !isspace(*str)) str++;
			while (isspace(*str)) str++;
			for (i=0; gl_blendfunc_list[i].name; i++) {
				if (!strncmp(str, gl_blendfunc_list[i].name,
					gl_blendfunc_list[i].len)) break;
			}
			if (!gl_blendfunc_list[i].name) return -1;
			dfactor = gl_blendfunc_list[i].mode;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.sfactor = sfactor;
			m_shapes[shape_id]->pDrawTail->p2.dfactor = dfactor;
		}
		break;
	  case SM_GL_TEXFILTER2D:
		{
			// Looking for : glshape texfilter2d %u (nearest|linear) (nearest|linear
			// First is MIN_FILTER and second is MAG_FILTER
			GLint min, mag;
			if (!strncmp(str, "near", 4)) min = (GLint) GL_NEAREST;
			else if (!strncmp(str, "lin", 3)) min = (GLint) GL_LINEAR;
			else return -1;
			while (*str && !isspace(*str)) str++;
			while (isspace(*str)) str++;
			if (!strncmp(str, "near", 4)) mag = (GLint) GL_NEAREST;
			else if (!strncmp(str, "lin", 3)) mag = (GLint) GL_LINEAR;
			else return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.mode = GL_TEXTURE_2D;
			m_shapes[shape_id]->pDrawTail->p3.param_i = min;
			m_shapes[shape_id]->pDrawTail->p4.param_i = mag;
		}
		break;
	  case SM_GLU_TEXTURE:
		{
			u_int32_t quad_id;
			u_int32_t texture;
			if ((sscanf(str, "%u", &quad_id) != 1)) return -1;
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
			if ((sscanf(str, "%u", &texture) != 1)) return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.quad_id = quad_id;
			m_shapes[shape_id]->pDrawTail->p2.texture = texture ? GL_TRUE : GL_FALSE;
		}
		break;
	  case SM_GL_CLEARCOLOR:
		{
			float red, green, blue, alpha;
			if ((sscanf(str, "%f %f %f %f", &red, &green, &blue, &alpha) != 4)) return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			if (red < 0.0) red = 0.0; else if (red > 1.0) red = 1.0;
			if (green < 0.0) green = 0.0; else if (green > 1.0) green = 1.0;
			if (blue < 0.0) blue = 0.0; else if (blue > 1.0) blue = 1.0;
			if (alpha < 0.0) alpha = 0.0; else if (alpha > 1.0) alpha = 1.0;
			m_shapes[shape_id]->pDrawTail->p1.redf = red;
			m_shapes[shape_id]->pDrawTail->p2.greenf = green;
			m_shapes[shape_id]->pDrawTail->p3.bluef = blue;
			m_shapes[shape_id]->pDrawTail->p4.alphaf = alpha;
		}
		break;
#ifdef HAVE_GLU
	  case SM_GLU_NORMALS:
		{
			u_int32_t quad_id;
			GLenum normal;
			if ((sscanf(str, "%u", &quad_id) != 1)) return -1;
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
			if (!strncasecmp(str, "none ", 5)) normal = GLU_NONE;
			else if (!strncasecmp(str, "flat ", 5)) normal = GLU_FLAT;
			else if (!strncasecmp(str, "smooth ", 7)) normal = GLU_SMOOTH;
			else return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.quad_id = quad_id;
			m_shapes[shape_id]->pDrawTail->p2.normal = normal;
		}
		break;
	  case SM_GLU_DRAWSTYLE:
		{
			u_int32_t quad_id;
			GLenum draw;
			if ((sscanf(str, "%u", &quad_id) != 1)) return -1;
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
			if (!strncasecmp(str, "fill ", 5)) draw = GLU_FILL;
			else if (!strncasecmp(str, "line ", 5)) draw = GLU_LINE;
			else if (!strncasecmp(str, "point ", 7)) draw = GLU_POINT;
			else if (!strncasecmp(str, "silhouette ", 7)) draw = GLU_SILHOUETTE;
			else return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.quad_id = quad_id;
			m_shapes[shape_id]->pDrawTail->p2.draw = draw;
		}
		break;
	  case SM_GLU_ORIENTATION:
		{
			u_int32_t quad_id;
			GLenum orientation;
			if ((sscanf(str, "%u", &quad_id) != 1)) return -1;
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
			if (!strncasecmp(str, "outside ", 8)) orientation = GLU_OUTSIDE;
			else if (!strncasecmp(str, "inside ", 7)) orientation = GLU_INSIDE;
			else return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			m_shapes[shape_id]->pDrawTail->p1.quad_id = quad_id;
			m_shapes[shape_id]->pDrawTail->p2.orientation = orientation;
		}
		break;
	  case SM_GLU_CYLINDER:
		{
			u_int32_t quad_id;
			GLdouble base, top, height;
			GLint slices, stacks;
			if ((sscanf(str, "%u %lf %lf %lf %d %d", &quad_id, &base,
				&top, &height, &slices, &stacks) != 6) || base < 0 ||
				top < 0 || height <= 0 || slices <= 0 || stacks <= 0) return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			if (!m_shapes || shape_id >= m_max_shapes ||
				!m_shapes[shape_id] || !m_shapes[shape_id]->pDrawTail) {
				fprintf(stderr, "Unexpected missses in COpenglVideo::"
					"set_shape_enum_or_int_parameters <%s>\n", create_str);
				return -1;
			}
			m_shapes[shape_id]->pDrawTail->p1.quad_id = quad_id;
			m_shapes[shape_id]->pDrawTail->p2.base = base;
			m_shapes[shape_id]->pDrawTail->p3.slices = slices;
			m_shapes[shape_id]->pDrawTail->p4.stacks = stacks;
			m_shapes[shape_id]->pDrawTail->p5.top = top;
			m_shapes[shape_id]->pDrawTail->p6.height = height;
		}
		break;
	  case SM_GLU_DISK:
	  case SM_GLU_PARTIALDISK:
		{
			u_int32_t quad_id;
			GLdouble inner, outer, start = 0, sweep = 0;
			GLint slices, loops;
			int n;
			if (!(n = sscanf(str, "%u %lf %lf %d %d %lf %lf", &quad_id, &inner,
				&outer, &slices, &loops, &start, &sweep)) ||
				(type == SM_GLU_DISK && n < 5) ||
				(type == SM_GLU_PARTIALDISK && n < 7) ||
				slices <= 0 || loops <= 0) return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			if (!m_shapes || shape_id >= m_max_shapes ||
				!m_shapes[shape_id] || !m_shapes[shape_id]->pDrawTail) {
				fprintf(stderr, "Unexpected missses in COpenglVideo::"
					"set_shape_enum_or_int_parameters <%s>\n", create_str);
				return -1;
			}
			m_shapes[shape_id]->pDrawTail->p1.quad_id = quad_id;
			m_shapes[shape_id]->pDrawTail->p2.inner = inner;
			m_shapes[shape_id]->pDrawTail->p3.slices = slices;
			m_shapes[shape_id]->pDrawTail->p4.loops = loops;
			m_shapes[shape_id]->pDrawTail->p5.outer = outer;
			m_shapes[shape_id]->pDrawTail->p6.start = start;
			m_shapes[shape_id]->pDrawTail->p7.sweep = sweep;
		}
		break;
	  case SM_GLU_SPHERE:
		{
			u_int32_t quad_id;
			GLdouble radius;
			GLint slices, stacks;
			if ((sscanf(str, "%u %lf %d %d", &quad_id, &radius,
				&slices, &stacks) != 4) || radius <= 0 ||
				slices <= 0 || stacks <= 0) return -1;
			if ((n = AddToShapeCommand(shape_id, type))) return n;
			if (!m_shapes || shape_id >= m_max_shapes ||
				!m_shapes[shape_id] || !m_shapes[shape_id]->pDrawTail) {
				fprintf(stderr, "Unexpected missses in COpenglVideo::"
					"set_shape_enum_or_int_parameters <%s>\n", create_str);
				return -1;
			}
			m_shapes[shape_id]->pDrawTail->p1.quad_id = quad_id;
			m_shapes[shape_id]->pDrawTail->p2.radius = radius;
			m_shapes[shape_id]->pDrawTail->p3.slices = slices;
			m_shapes[shape_id]->pDrawTail->p4.stacks = stacks;
		}
		break;
#endif // #ifdef HAVE_GLU
#ifdef HAVE_GLUT
	case SM_GLUT_TEAPOT: {
		GLdouble size;
		GLboolean solid = true;
		if (!(strncasecmp(str, "solid ", 6))) {
			str += 6;
		} else if (!(strncasecmp(str, "wire ", 5))) {
			str += 5;
			solid = false;
		}
		while (isspace(*str)) str++;
		if (sscanf(str, "%lf", &size) != 1) return -1;
		if ((n = AddToShapeCommand(shape_id, type))) return n;
		m_shapes[shape_id]->pDrawTail->p1.size = size;
		m_shapes[shape_id]->pDrawTail->p2.solid = solid;
		}
		break;
#endif

/*
	  case SM_GL_TEXPARAMETERI:
	  case SM_GL_TEXPARAMETERF:
		 {
		if (!(*str)) return;
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//
		// pname : min_filter : GL_TEXTURE_MIN_FILTER,
		//	   mag_filter : GL_TEXTURE_MAG_FILTER,
		//	   wrap_s     : GL_TEXTURE_WRAP_S
		//	   wrap_t     :GL_TEXTURE_WRAP_T
		//
		// First we look after the target name
		int i;
		for (i=0; gl_parameter_list[i].name; i++) {
			if (!strncmp(str, gl_parameter_list[i].name,
				gl_parameter_list[i].len)) break;
		}
		if (!gl_parameter_list[i].name) return -1;
		mode = gl_parameter_list[i].parmname;

		while (*str && !isspace(*str)) str++;
		while (isspace(*str)) str++;
		if (!(*str)) return;

		// Then we look after the parm name
		int i;
		for (i=0; gl_parameter_list[i].name; i++) {
			if (!strncmp(str, gl_parameter_list[i].name,
				gl_parameter_list[i].len)) break;
		}
		if (!gl_parameter_list[i].name) return -1;
		GLenum parmname = gl_parameter_list[i].parmname;

		while (*str && !isspace(*str)) str++;
		while (isspace(*str)) str++;
		if (!(*str)) return;

		// Now we can look for nearest or linear; .... more code
		}
		break;
*/
	  default:
		fprintf(stderr, "else return -1 enum <%s>\n",str);
		return -1;
		break;
	}
	if (!n) m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	if (!n && m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape %u %s\n", shape_id, m_shapes[shape_id]->pDrawTail->create_str);
	return 0;
}


int COpenglVideo::set_shape_no_parameter(struct controller_type* ctr, const char* str, opengl_operation_enum_t type)
{
	u_int32_t shape_id;
	if (!str) return -1;
	const char* create_str = str;
	while (isspace(*str)) str++;
	if (sscanf(str, "%u", &shape_id) != 1) return -1;
	int n = AddToShapeCommand(shape_id, type);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"glshape %u %s\n",
			shape_id,
			type == SM_GL_END ? "end" :
			  type == SM_GL_FINISH ? "finish" :
			    type == SM_GL_FLUSH ? "flush" :
			      type == SM_GL_NOOP ? "noop" :
				type == SM_GL_LOADIDENTITY ? "loadidentity" :
				  type == SM_GL_PUSHMATRIX ? "pushmatrix" :
				    type == SM_GL_POPMATRIX ? "popmatrix" : "unknown");
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int COpenglVideo::set_shape_list(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id;
	if (!str || !m_shapes) return 1;
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_shapes) {
			for (u_int32_t id=0; id < m_max_shapes; id++) if (m_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape %u ops %u name %s\n",
					id, m_shapes[id]->ops_count, m_shapes[id]->name);
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"\n");

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	int n = sscanf(str, "%u", &shape_id);
	if (!n) return -1;
	return PrintShapeElements(ctr, shape_id);
}


int COpenglVideo::PrintShapeElements(struct controller_type* ctr, u_int32_t shape_id) {
	char prepend[64];
	u_int32_t begins = 0;
	if (!m_shapes) return 1;
	if (shape_id >= m_max_shapes || !m_shapes[shape_id]) return -1;
	opengl_draw_operation_t* pDraw = m_shapes[shape_id]->pDraw;
	u_int32_t line = 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr,
		STATUS" shape %u ops %u name %s\n", shape_id, m_shapes[shape_id]->ops_count,
		m_shapes[shape_id]->name);
	while (pDraw) {
		if (pDraw->type == SM_GL_END && begins) begins--;
		sprintf(prepend, STATUS" - %2u %s%s%s", line++,
			pDraw->execute ? "" : "# ",
			  (!begins ? "" : begins == 1 ? " " : "  "),
			  !pDraw->conditional ? "" :
			    pDraw->conditional == 1 ? "if osmesa " :
			    pDraw->conditional == 2 ? "if glx " : "Unknown conditional ");
		switch (pDraw->type) {
			case SM_GL_END:
			case SM_GL_FINISH:
			case SM_GL_FLUSH:
			case SM_GL_NOOP:
			case SM_GL_LOADIDENTITY:
			case SM_GL_PUSHMATRIX:
			case SM_GL_POPMATRIX:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s\n", prepend,
					pDraw->type == SM_GL_PUSHMATRIX ? "pushmatrix" :
					  pDraw->type == SM_GL_POPMATRIX ? "popmatrix" :
					    pDraw->type == SM_GL_END ? "end" :
					      pDraw->type == SM_GL_FLUSH ? "flush" :
						pDraw->type == SM_GL_FINISH ? "finish" :
						  pDraw->type == SM_GL_NOOP ? "noop" :
						    pDraw->type == SM_GL_LOADIDENTITY ? "loadidentity" :
						      "unknown");
				break;
			case SM_GL_BEGIN:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sbegin %s%s\n", prepend,
					pDraw->p1.mode == GL_TRIANGLES ? "triangles" :
					  pDraw->p1.mode == GL_QUADS ? "quads" :
					  pDraw->p1.mode == GL_POLYGON ? "polygon" :
					  pDraw->p1.mode == GL_POINTS ? "points" :
					  pDraw->p1.mode == GL_LINES ? "lines" :
					  pDraw->p1.mode == GL_LINE_STRIP ? "line_strip" :
					  pDraw->p1.mode == GL_LINE_LOOP ? "line_loop" :
					  pDraw->p1.mode == GL_TRIANGLE_STRIP ? "triangle_strip" :
					  pDraw->p1.mode == GL_TRIANGLE_FAN ? "triangle_fan" :
					  pDraw->p1.mode == GL_QUAD_STRIP ? "quad_strip" : "unknown");
				begins++;
				break;
			case SM_GL_SHADEMODEL:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sshademodel %s \n", prepend,
					pDraw->p1.mode == GL_FLAT ? "flat" :
					  pDraw->p1.mode == GL_SMOOTH ? "flat" : "unknown");
				break;
			case SM_GL_ENABLE:
			case SM_GL_DISABLE: {
				const char* mode;
				Mode2String(mode, pDraw->p1.mode, gl_enable_mode_list);
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s mode %s \n", prepend,
					pDraw->type == SM_GL_ENABLE ? "enable" :
					  pDraw->type == SM_GL_DISABLE ? "disable" : "unknown",
					mode ? mode : "unknown");
				}
				break;
			case SM_GL_RECURSION:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%srecursion %u \n", prepend, pDraw->p1.level);
				break;
			case SM_GL_MATRIXMODE:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%smatrixmode %s \n", prepend,
					pDraw->p1.mode == GL_PROJECTION ? "projection" :
					  pDraw->p1.mode == GL_MODELVIEW ? "modelview" :
					    pDraw->p1.mode == GL_TEXTURE ? "texture" :
					      pDraw->p1.mode == GL_COLOR ? "color" : "unknown");
				break;
			case SM_GL_CLEAR:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sclear %s \n", prepend,
					((pDraw->p1.mode & GL_COLOR_BUFFER_BIT) &&
					 (pDraw->p1.mode & GL_DEPTH_BUFFER_BIT)) ? "color|depth" :
					  pDraw->p1.mode & GL_COLOR_BUFFER_BIT ? "color" :
					  pDraw->p1.mode & GL_DEPTH_BUFFER_BIT ? "depth" : "unknown");
				break;
			case SM_GL_NORMAL:
			case SM_GL_VERTEX3:
			case SM_GL_TEXCOORD3:	// prints 3 values
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s "SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT","SM_OPENGL_PARM_PRINT"\n",
					prepend,
					pDraw->type == SM_GL_TRANSLATE ? "translate" :
					  pDraw->type == SM_GL_NORMAL ? "normal" :
					    pDraw->type == SM_GL_VERTEX3 ? "vertex3" :
					      pDraw->type == SM_GL_TEXCOORD3 ? "texcoord3" :
						pDraw->type == SM_GL_SCALE ? "scale" : "unknown",
					pDraw->p1.x, pDraw->p2.y, pDraw->p3.z);
				break;
			case SM_GL_TRANSLATE:	// prints 3 values
#ifdef SM_OPENGL_PARM_TYPE_INT
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%stranslate "SM_OPENGL_PARM_RS_PRINT","
					SM_OPENGL_PARM_RS_PRINT","SM_OPENGL_PARM_RS_PRINT"\n",
					prepend,
					pDraw->p1.xf, pDraw->p2.yf, pDraw->p3.zf);
#else // for GL functions double or float
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%stranslate "SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT","SM_OPENGL_PARM_PRINT"\n",
					prepend,
					pDraw->p1.x, pDraw->p2.y, pDraw->p3.z);
#endif
				break;
			case SM_GL_SCALE:
#ifdef SM_OPENGL_PARM_TYPE_INT
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sscale "SM_OPENGL_PARM_RS_PRINT","
					SM_OPENGL_PARM_RS_PRINT","SM_OPENGL_PARM_RS_PRINT"\n",
					prepend,
					pDraw->p1.xf, pDraw->p2.yf, pDraw->p3.zf);
#else // for GL functions double or float
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sscale "SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT","SM_OPENGL_PARM_PRINT"\n",
					prepend,
					pDraw->p1.x, pDraw->p2.y, pDraw->p3.z);
#endif
				break;
			case SM_GL_CLEARCOLOR:	// prints 4 values
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sclearcolor %.4f,%.4f,%.4f,%.4f\n",
					prepend,
					pDraw->p1.redf, pDraw->p2.greenf, pDraw->p3.bluef,
					pDraw->p4.alphaf);
				break;
			case SM_GL_COLOR3:	// prints 3 values
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s "SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT","SM_OPENGL_PARM_PRINT"\n",
					prepend,
					pDraw->type == SM_GL_COLOR3 ? "color3" : "unknown",
					pDraw->p1.red, pDraw->p2.green, pDraw->p3.blue);
				break;
			case SM_GL_COLOR4:	// prints 4 values
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s "SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT","SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT"\n", prepend,
					pDraw->type == SM_GL_COLOR4 ? "color4" : "unknown",
					pDraw->p1.red, pDraw->p2.green, pDraw->p3.blue,
					pDraw->p4.alpha);
				break;
			case SM_GL_ROTATE:	// prints 4 values
#ifdef SM_OPENGL_PARM_TYPE_INT
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%srotate "SM_OPENGL_PARM_RS_PRINT" vector "
					SM_OPENGL_PARM_RS_PRINT","SM_OPENGL_PARM_RS_PRINT","
					SM_OPENGL_PARM_RS_PRINT"\n", prepend,
					pDraw->p4.anglef, pDraw->p1.xf, pDraw->p2.yf,
					pDraw->p3.zf);
#else // for GL functions double or float
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%srotate "SM_OPENGL_PARM_PRINT" vector "
					SM_OPENGL_PARM_PRINT","SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT"\n", prepend,
					pDraw->p4.angle, pDraw->p1.x, pDraw->p2.y,
					pDraw->p3.z);
#endif
				break;

			case SM_GL_VERTEX4:	// prints 4 values
			case SM_GL_TEXCOORD4:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s "SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT","SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT"\n", prepend,
					pDraw->type == SM_GL_VERTEX4 ? "vertex4" :
					  pDraw->type == SM_GL_TEXCOORD4 ? "texcoord4" : "unknown",
					pDraw->p1.x, pDraw->p2.y, pDraw->p3.z,
					pDraw->p4.w);
				break;
			case SM_GL_VERTEX2:	// prints 2 values
			case SM_GL_TEXCOORD2:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s "SM_OPENGL_PARM_PRINT","
					SM_OPENGL_PARM_PRINT"\n", prepend,
					pDraw->type == SM_GL_VERTEX2 ? "vertex2" :
					  pDraw->type == SM_GL_TEXCOORD2 ? "texcoord2" : "unknown",
					pDraw->p1.x, pDraw->p2.y);
				break;
			case SM_GL_TEXCOORD1:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s "SM_OPENGL_PARM_PRINT"\n", prepend,
					pDraw->type == SM_GL_TEXCOORD1 ? "texcoord1" : "unknown",
					pDraw->p1.x);
				break;
			case SM_GL_MATERIALV: {
					const char* face, *pname, *name;
					Mode2String(face, pDraw->p3.face, gl_material_face_list);
					Mode2String(pname, pDraw->p2.pname, gl_material_parm_list);
					name = m_vectors && m_vectors[pDraw->p1.vector_id] ?
						m_vectors[pDraw->p1.vector_id]->name : NULL; 
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%smaterialv %s mode %s lightdef %u : %s\n",
					prepend,
					face ? face : "unknown",
					pname ? pname : "unknown", pDraw->p1.vector_id,
					name ? name : "lightdef is not defined");
					
				}
				break;
			case SM_GL_LIGHT:
			case SM_GL_LIGHTV: {
					const char* lighti, *pname, *name;
					Mode2String(lighti, pDraw->p3.lighti, gl_enable_mode_list);
					Mode2String(pname, pDraw->p2.pname, gl_light_parm_list);
					if (pDraw->type == SM_GL_LIGHT) {
						m_pVideoMixer->m_pController->controller_write_msg (ctr,
							"%slight %s mode %s value "
#ifdef SM_OPENGL_PARM_TYPE_INT
							"%u"
#else
							"%f"
#endif
							"\n", prepend,
							lighti ? lighti : "unknown",
							pname ? pname : "unknown", pDraw->p1.light_param);
					} if (pDraw->type == SM_GL_LIGHTV) {
						name = m_vectors &&
							pDraw->p1.vector_id < m_max_vectors &&
							m_vectors[pDraw->p1.vector_id] ?
							m_vectors[pDraw->p1.vector_id]->name : NULL; 
						m_pVideoMixer->m_pController->controller_write_msg (ctr,
							"%slight %s mode %s lightdef %u : %s\n",
							prepend,
							lighti ? lighti : "unknown",
							pname ? pname : "unknown", pDraw->p1.vector_id,
							name ? name : "vector is not defined");
					} 
				}
				break;
			case SM_GL_BINDTEXTURE: {
				opengl_texture_t* pTexture = m_textures && pDraw->p1.texture_id < m_max_textures ? m_textures[pDraw->p1.texture_id] : NULL;
				const char* name = pTexture ? pTexture->name : NULL;
				texture_source_enum_t source = pTexture ? pTexture->source : TEXTURE_SOURCE_NONE;
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%stexture bind id %u min %s mag %s type %s : "
					"Texture %u %s, %s %d\n", prepend,
					pDraw->p1.texture_id,
					pDraw->p3.param_i == (GLint) GL_NEAREST ? "nearest" :
					  pDraw->p3.param_i == (GLint) GL_LINEAR ? "linear" :
					    pDraw->p3.param_i ==
					      (GLint) GL_NEAREST_MIPMAP_NEAREST ?
					        "nearest_mipmap_nearest" :
					      pDraw->p3.param_i ==
						(GLint) GL_LINEAR_MIPMAP_NEAREST ?
					          "linear_mipmap_nearest" :
					        pDraw->p3.param_i ==
					          (GLint) GL_NEAREST_MIPMAP_LINEAR ?
					            "nearest_mipmap_linear" :
					        pDraw->p3.param_i ==
						  (GLint) GL_LINEAR_MIPMAP_LINEAR ?
						    "linear_mipmap_linear" : "unknown",
					pDraw->p4.param_i == (GLint) GL_NEAREST ? "nearest" :
					  pDraw->p4.param_i == (GLint) GL_LINEAR ? "linear" :
					    "unknown",
					pDraw->p2.pname == GL_TEXTURE_2D ? "2d" :
					  pDraw->p2.pname == GL_TEXTURE_CUBE_MAP ?
					    "cube" : "unknown type",
					pDraw->p1.texture_id,
					name ? name : "none",
					source == TEXTURE_SOURCE_NONE ? "none" :
					source == TEXTURE_SOURCE_FEED ? "feed" :
					source == TEXTURE_SOURCE_IMAGE ? "image" :
					source == TEXTURE_SOURCE_FRAME ? "frame" : "unknown",
					pTexture->source_id);
				}
				break;
			case SM_GL_ARCXZ:
			case SM_GL_ARCYZ:
			case SM_GL_ARCXY:
				// glshape arcxz <glshape id> <angle> <aspect> <slices>
				//	<texleft> <texright> <textop> <texbottom>\n"
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s angle %.4lf aspect %.4lf slices %d "
					"tex l,r,t,b %.4lf,%.4lf,%.4lf,%.4lf\n", prepend,
					pDraw->type == SM_GL_ARCXZ ? "arcxz" :
					  pDraw->type == SM_GL_ARCYZ ? "arcyz" :
					    pDraw->type == SM_GL_ARCXY ? "arcxy" : "arcunknown",
					pDraw->p4.angled,
					pDraw->p7.aspect,
					pDraw->p3.slices,
					pDraw->p1.left,
					pDraw->p2.right,
					pDraw->p5.top,
					pDraw->p6.bottom);
				break;
	 		case SM_GL_ORTHO:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sortho left/right %.4lf %.4lf bottom/top "
					"%.4lf %.4lf near/far %.4lf %.4lf\n",
					prepend,
					pDraw->p1.left, pDraw->p2.right,
					pDraw->p6.bottom, pDraw->p5.top,
					pDraw->p3.zNear, pDraw->p4.zFar);
				break;
	 		case SM_GL_VBO:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%svbo %u\n", prepend, pDraw->p1.vbo_id);
				break;
	 		case SM_GL_VIEWPORT:
	 		case SM_GL_SCISSOR:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%s%s %d %d %d %d\n", prepend,
					pDraw->type == SM_GL_VIEWPORT ? "viewport" :
					  pDraw->type == SM_GL_SCISSOR ? "scissor" : "unknown",
					(int) pDraw->p1.xi,
					(int) pDraw->p2.yi,
					(int) pDraw->p3.width,
					(int) pDraw->p4.height);
				break;
	 		case SM_GL_SNOWMIX:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%ssnowmix %s\n", prepend,
					pDraw->create_str ? pDraw->create_str : "");
				break;
	 		case SM_GL_BLENDFUNC:
				{ const char* sfactor;
				  const char* dfactor;
				  Mode2String(sfactor, pDraw->p1.sfactor, gl_blendfunc_list);
				  Mode2String(dfactor, pDraw->p2.dfactor, gl_blendfunc_list);
				  m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sblendfunc %s %s\n", prepend,
					sfactor ? sfactor : "unknown",
					dfactor ? dfactor : "unknown");
				}
				break;
			case SM_GL_TEXFILTER2D:
				// p1.mode = mode
				// p3.parami = min
				// p4.parami = mag
				{ const char* mode;
				  Mode2String(mode, pDraw->p1.mode, gl_enable_mode_list);
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%stexfilter2d %s min %s mag %s\n", prepend,
					mode ? mode : "unknown",
					pDraw->p3.param_i == (GLint) GL_NEAREST ? "nearest" :
					  pDraw->p3.param_i == (GLint) GL_LINEAR ? "linear" : "unknown",
					pDraw->p4.param_i == (GLint) GL_NEAREST ? "nearest" :
					  pDraw->p4.param_i == (GLint) GL_LINEAR ? "linear" : "unknown"
					);
				}
				break;
#ifdef HAVE_GLU
			case SM_GLU_CYLINDER:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sglucylinder %u %.4lf %.4lf %.4lf %d %d\n",
					prepend,
					(unsigned int) pDraw->p1.quad_id,
					(double) pDraw->p2.base,
					(double) pDraw->p5.top,
					(double) pDraw->p6.height,
					(int) pDraw->p3.slices,
					(int) pDraw->p4.stacks);
				break;
	 		case SM_GLU_PERSPECTIVE:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sgluperspective %.5lf %.5lf %.4lf %.4lf\n",
					prepend,
					pDraw->p1.fovy,
					pDraw->p2.aspect,
					pDraw->p3.zNear,
					pDraw->p4.zFar);
				break;
			case SM_GLU_DISK:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sgludisk quad %u inner %.4lf outer %.4lf "
					"slices %d loops %d\n", prepend,
					(unsigned int) pDraw->p1.quad_id,
					(double) pDraw->p2.inner,
					(double) pDraw->p5.outer,
					(int) pDraw->p3.slices,
					(int) pDraw->p4.loops);
				break;
			case SM_GLU_PARTIALDISK:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sglupartialdisk quad %u inner %.4lf outer %.4lf "
					"slices %d loops %d start %.4lf sweep %.4lf\n", prepend,
					(unsigned int) pDraw->p1.quad_id,
					(double) pDraw->p2.inner,
					(double) pDraw->p5.outer,
					(int) pDraw->p3.slices,
					(int) pDraw->p4.loops,
					(double) pDraw->p6.start,
					(double) pDraw->p7.sweep);
				break;
			case SM_GLU_SPHERE:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sglusphere quad %u radius %.4lf slices %d stacks %d\n",
					prepend,
					(unsigned int) pDraw->p1.quad_id,
					(double) pDraw->p2.radius,
					(int) pDraw->p3.slices,
					(int) pDraw->p4.stacks);
				break;
			case SM_GLU_TEXTURE:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sglutexture quad %u texture %s\n", prepend,
					pDraw->p1.quad_id,
					pDraw->p2.texture == GL_TRUE ? "true" :
					  pDraw->p2.texture == GL_FALSE ? "false" : "unknown");
				break;
			case SM_GLU_NORMALS:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sglunormals quad %u %s\n", prepend,
					pDraw->p1.quad_id,
					pDraw->p2.normal == GLU_NONE ? "none" :
					  pDraw->p2.normal == GLU_FLAT ? "flat" :
					    pDraw->p2.normal == GLU_SMOOTH ? "smooth" : "unknown");
				break;
			case SM_GLU_DRAWSTYLE:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sgludrawstyle quad %u draw %s\n", prepend,
					pDraw->p1.quad_id,
					pDraw->p2.draw == GLU_FILL ? "fill" :
					  pDraw->p2.draw == GLU_LINE ? "line" :
					    pDraw->p2.draw == GLU_POINT ? "point" :
					      pDraw->p2.draw == GLU_SILHOUETTE ? "silhouette" :
						"unknown");
				break;
			case SM_GLU_ORIENTATION:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sgluorientation quad %u orientation %s\n", prepend,
					pDraw->p1.quad_id,
					pDraw->p2.orientation == GLU_OUTSIDE ? "outside" :
					  pDraw->p2.orientation == GLU_FLAT ? "inside" :
					    "unknown");
				break;
#endif // #ifdef HAVE_GLU
			case SM_GL_GLSHAPE: {
				const char* name = m_shapes && m_shapes[pDraw->p1.shape_id] ?
					m_shapes[pDraw->p1.shape_id]->name : NULL;
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sinshape %u : (%s)\n", prepend,
					pDraw->p1.shape_id,
					name ? name : "shape does not exist");
				}
				break;
#ifdef HAVE_GLUT
			case SM_GLUT_TEAPOT:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sglutteapot %s %.4lf\n", prepend,
					pDraw->p2.solid ? "solid" : "wire", pDraw->p1.size);
				break;
#endif
			default:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%sunknown\n", prepend);
				break;
		}
		pDraw = pDraw->next;
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}


int COpenglVideo::DestroyContext() {
	if (m_pCtx_OSMesa || m_pCtx_GLX) {
#ifdef HAVE_GLU
		if (m_glu_quadrics) DeleteGluQuadric(m_glu_quadrics, true);
		m_glu_quadrics = NULL;
#endif // #ifdef HAVE_GLU
#ifdef HAVE_OSMESA
		if (m_pCtx_OSMesa) OSMesaDestroyContext(m_pCtx_OSMesa);
		m_pCtx_OSMesa = NULL;
#endif
#ifdef HAVE_GLX
		if (m_pCtx_GLX) {
			if (m_framebuffer_draw) glDeleteFramebuffers((GLsizei) 1, &m_framebuffer_draw);
			if (m_framebuffer_read) glDeleteFramebuffers((GLsizei) 1, &m_framebuffer_read);
			m_framebuffer_draw = m_framebuffer_read = 0;
			glXDestroyContext(m_display, m_pCtx_GLX);
		}
		if (m_display) XCloseDisplay(m_display);
		m_display = NULL;
		m_pCtx_GLX = NULL;
#endif
		m_width = 0;
		m_height = 0;
		m_depth_bits = 0;
		return 0;
	}
	return -1;
}


/* CopyBack will copy from the framebuffer to a destination.
 * Format is BGRA.  If width or height is 0, the width and height
 * of the context is used taking into account x and y.
 * Dst must have the width x height x 4 layout
 */
int COpenglVideo::CopyBack(u_int8_t* dst, u_int32_t x, u_int32_t y,
	u_int32_t width, u_int32_t height) {

	if (!dst || x+width > m_width || y+height > m_height) return -1;
	if (!width) width = m_width - x;
	if (!height) height = m_height - y;
	if (!width || !height) return -1;

	glFinish();		// We need to ensure all commands are finished.
#ifdef HAVE_GLX
	if (m_use_glx && m_pCtx_GLX) {
		if (m_framebuffer_read) {
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_framebuffer_read);
			glReadPixels(x, y, width, height, GL_BGRA, GL_UNSIGNED_BYTE, dst);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);	//releasing
		}
	} else
#endif
	if (m_pCtx_OSMesa) {
		// The CVideoMixer->m_overlay is the baseframe
		if (!m_pVideoMixer->m_overlay || dst == m_pVideoMixer->m_overlay) return -1;	// Nothing to copy from or copying to itself
		if (x || y || width != m_width || height != m_height) {
			u_int8_t* p = m_pVideoMixer->m_overlay + 4*(x + y*width);
			for (; y < m_height ; y++) {
				memcpy(dst, p, 4*width);
				dst += 4*width;
				p += 4*m_width;
			}
		} else {
			memcpy(dst, m_pVideoMixer->m_overlay, 4*width*height);
		}
	} else {
		if (m_verbose) fprintf(stderr, "Copyback was called with no context enabled\n");
		return -1;
	}
	CheckForGlError(m_verbose, "in CopyBack");
	return 0;
}

#ifdef HAVE_GLX
void COpenglVideo::SwapBuffers(u_int8_t* overlay) {
	GLenum glError;
	if (m_use_glx && m_pCtx_GLX) {
		//CopyBack(overlay, 0, 0, m_width, m_height);
		if (m_display) {
			glXSwapBuffers (m_display, m_win);
			if ((glError = glGetError()) != GL_NO_ERROR)
				fprintf(stderr, "Error Swapping buffers\n");
		}
	}
}
#endif


/* 
 * MakeCurrent is called at once every frame period by OverlayShape
 * before the first placed shape is placed. MakeCurrent will call CreateContext()
 * if a context does not exist. If an OSMesa context is created, MakeCurrent
 * will copy baseframe (typically CVideoMixer::m_overlay frame) into the
 * context frame buffer. If a GLX context is created, then this is not possible.
 * MakeCurrent will execute glshape 0 once when the context is created.
 * MakeCurrent will also execute glshape once for every frame period.
 */
int COpenglVideo::MakeCurrent(u_int8_t *baseframe, u_int32_t width, u_int32_t height)
{
	GLuint framebuffers[2];
	bool creating_context = false;
	int n = 0;

	// Ensure MakeCurrent is only called once every frame period.
	if (m_current_context_is_valid) return 0;

	// Make some checks and get geometry
	if (!m_pVideoMixer || !baseframe) return -1;
	if (!width) width = m_pVideoMixer->GetSystemWidth();
	if (!height) height = m_pVideoMixer->GetSystemHeight();
 	if (!width || !height) {
		fprintf(stderr, "Warning. Width or height == 0 in COpenglVideoCreateContext\n");
		return -1;
	}

#ifdef HAVE_GLX
	if (!m_pCtx_OSMesa && !m_pCtx_GLX)
#else
	if (!m_pCtx_OSMesa)
#endif
	{
		fprintf(stderr, "MakeCurrent (%s) will call CreateContext\n",
			m_use_glx ? "GLX" : "OSMesa");
		creating_context = true;
		CreateContext(baseframe, width, height);
		fprintf(stderr, "MakeCurrent continues.\n");
	}
#ifdef HAVE_GLX
	if (!m_pCtx_OSMesa && !m_pCtx_GLX)
#else
	if (!m_pCtx_OSMesa)
#endif
	{
		fprintf(stderr, "Failed to create create GL contxt in COpenglVideoMakeCurrent\n");
		return -1;
	}
	if (m_use_glx) {
#ifdef HAVE_GLX

		// For GLX we only MakeCurrent one time when we create the context.
		// We also generate framebuffers for sending a frame to graphic card
		// and to read from graphic card.
		if (creating_context) {
			if (!m_display) {
				fprintf(stderr, " - Error. m_display was NULL in MakeCurrent\n");
				return -1;
			}
			if (!glXMakeCurrent(m_display, m_win, m_pCtx_GLX)) {
				fprintf(stderr, " - Error. glXMakeCurrent Failed\n");
				return -1;
			}
			if (m_verbose) fprintf(stderr, " - Generating framebuffers\n");
			glGenFramebuffers((GLsizei)2, framebuffers);
			if (!framebuffers[0] || !framebuffers[1]) {
				fprintf(stderr, " - WARNING: Failed to create GLX frame buffers in "
					"COpenglVideo::CreateContext\n");
			}
			m_framebuffer_draw = framebuffers[0];
			m_framebuffer_read = framebuffers[1];
			if (m_verbose) fprintf(stderr, " - Framebuffers generated. Good!\n");
		}
/*
		if (0 && baseframe) {
			if (0 && m_framebuffer_draw) {
				glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer_draw);
				glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, baseframe);
				glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
				//glXSwapBuffers (m_display, m_win);
			} else {
				GLuint tex_id = loadTexture(baseframe, width, height);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glColor4f(1.0f, 1.0f, 1.0f, 0.4);
				glBegin(GL_QUADS);
				  glTexCoord2f(0.0f,0.0f);
				  glVertex3f(0.0f, 0.0f, 0.0f);
				  glTexCoord2f(0.0f,1.0f);
				  glVertex3f(1023.0f, 0.0f, 0.0f);
				  glTexCoord2f(0.0f,1.0f);
				  glVertex3f(1023.0f, 575.0f, 0.0f);
				  glTexCoord2f(0.0f,1.0f);
				  glVertex3f(0.0f, 575.0f, 0.0f);
				glEnd();
				glDeleteTextures(1, &tex_id);
			}
		}
*/


		//SM_GL_OPER_ROTATE(180,0,0,1);

#else
		fprintf(stderr, "ERROR. m_use_glx was true, but Snowmix was "
			"not compiled with GLX support.\n");
		return -1;
#endif
	} else {			// Using OSMesa
#ifdef HAVE_OSMESA
		if (!m_pCtx_OSMesa) {
			fprintf(stderr, " - MakeCurrent failed to create a GL context in "
				"COpenglVideoMakeCurrent\n");
			return -1;
		}
		if (!OSMesaMakeCurrent( m_pCtx_OSMesa, baseframe, GL_UNSIGNED_BYTE, width, height)) {
			fprintf(stderr, " - Error. OSMesaMakeCurrent Failed\n");
			return -1;
		}
#else
		fprintf(stderr, "Error. OSMesa was not included at compile time.\n");
#endif
	}
	if (creating_context) {
#ifdef HAVE_GLUT
		//int c = 0;
		//glutInit(&c, NULL);
#endif
	}

	if (m_verbose && creating_context) fprintf(stderr, " - MakeCurrent initial : Geometry %ux%u depth %u\n"
		"    - OpenGL vendor   : %s\n"
		"    - OpenGL renderer : %s\n"
		"    - OpenGL Version  : %s\n"
		"    - OpenGL Shading  : %s\n",
		m_width, m_height, m_depth_bits,
		glGetString(GL_VENDOR), glGetString(GL_RENDERER),
		glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));

	if (creating_context && m_textures)
		for (u_int32_t id = 0; id < m_max_textures; id++)
			if (m_textures[id]) {
				if (m_textures[id]->real_texture_id) {
					fprintf(stderr, "Delete Texture %u texture id %u\n",
						id, m_textures[id]->real_texture_id);
					glDeleteTextures(1, &(m_textures[id]->real_texture_id));
				}
				m_textures[id]->real_texture_id = 0;
	}
	if (!m_shapes) {
		fprintf(stderr, " - Error. m_shapes was NULL in MakeCurrent\n");
		m_current_context_is_valid = true;
		return 1;
	}

	// Every time a new OpenGL context is created we will execute glshape 1.
	if (creating_context) {
		int shape_id = 1;
		if (m_shapes[shape_id]) {
			if (m_verbose) fprintf(stderr,
				" - MakeCurrent executing glshape %d : %s. %u lines.\n",
				shape_id, m_shapes[shape_id]->name ? m_shapes[shape_id]->name : "",
				m_shapes[shape_id]->ops_count);
			n = ExecuteShape(shape_id, OVERLAY_GLSHAPE_LEVELS);
		} else fprintf(stderr, " - WARNING. MakeCurrent found no glshape 1\n");
	}

	// MakeCurrent is called once for every time frame when the first glshape is Overlayed.
	// When this happens, glshape 1 is executed.
	if (!m_current_context_is_valid) {
		int shape_id = 2;
		if (m_shapes[shape_id]) {
			if ((creating_context && m_verbose) || (m_verbose > 1))
				fprintf(stderr,
					" - MakeCurrent executing glshape %d : %s. %u lines.\n",
					shape_id, m_shapes[shape_id]->name ?
					  m_shapes[shape_id]->name : "",
					m_shapes[shape_id]->ops_count);
			n = ExecuteShape(shape_id, OVERLAY_GLSHAPE_LEVELS);
		} else if (creating_context)
			fprintf(stderr, " - WARNING. MakeCurrent found no glshape 1\n");
	}

	// Ensure
	m_current_context_is_valid = true;
	return n;
}

/*
 * This function instructs the window manager to change this window from
 * NormalState to IconicState.
 */
#ifdef HAVE_GLX
Status XIconifyWindow(Display *dpy, Window w, int screen)
{
    XClientMessageEvent ev;
    Atom prop;

    prop = XInternAtom(dpy, "WM_CHANGE_STATE", False);
    if(prop == None)
    return False;

    ev.type = ClientMessage;
    ev.window = w;
    ev.message_type = prop;
    ev.format = 32;
    ev.data.l[0] = IconicState;
    return XSendEvent(dpy, RootWindow(dpy, screen), False,
            SubstructureRedirectMask|SubstructureNotifyMask,
            (XEvent *)&ev);
}
#endif

int COpenglVideo::CreateContext(u_int8_t *baseframe, u_int32_t width,
	u_int32_t height, int depth_bits) {

	if (m_verbose) fprintf(stderr, "CreateContext %ux%u depth %u\n", width, height, depth_bits);
	int n = 0;
	if (!m_pVideoMixer || !baseframe || depth_bits != 16) return -1;
	if (!width) width = m_pVideoMixer->GetSystemWidth();
	if (!height) height = m_pVideoMixer->GetSystemHeight();
 	if (!width || !height) {
		fprintf(stderr, "Warning. Width or height == 0 in COpenglVideoCreateContext\n");
		return -1;
	}

	m_depth_bits = depth_bits;
	m_width = width;
	m_height = height;
	if (m_verbose) fprintf(stderr, " - Geometry set to %ux%u\n - OpenGL type %s\n",
		width, height, m_use_glx ? "GLX" : "OSMesa");

	if (m_use_glx) {
#ifdef HAVE_GLX
		if (m_display) {
			if (m_verbose) fprintf(stderr, " - Closing open display\n");
			XCloseDisplay(m_display);
		}
		if (!(m_display = XOpenDisplay(0))) {
			fprintf(stderr, "Failed to open X display in COpenglVideo::CreateContext\n");
			goto reload_for_osmesa;
			return -1;
		}
		if (!(glXQueryVersion(m_display, &m_glx_major, &m_glx_minor))) {
			fprintf(stderr, "Failed to get GLX version in COpenglVideo::CreateContext\n");
			goto reload_for_osmesa;
		}
		if (m_verbose) fprintf(stderr, " - GLX version : %d.%d\n", m_glx_major, m_glx_minor);
		if (m_glx_major < 0 || (m_glx_major == 1 && m_glx_minor < 3)) {
			fprintf(stderr, " - WARNING. GLX version is less than 1.3.\n");
			goto reload_for_osmesa;
		}
		m_screen = DefaultScreen(m_display);

		//const char *extensions = glXQueryExtensionsString(display, DefaultScreen(display));

		static int attributeList[] = {
			//GLX_RGBA,
			GLX_DOUBLEBUFFER, true,
			GLX_RENDER_TYPE, GLX_RGBA_BIT,
			//GLX_X_RENDERABLE, true	// true/false/GLX_DONT_CARE
			GLX_RED_SIZE, 8,
			GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8,
			GLX_DEPTH_SIZE, 16,
			//GLX_STENCIL_SIZE, 1
			None };

		// GLXFBConfig *fbc = glXChooseFBConfig(m_display, m_screen, 0, &m_fb_count);
		GLXFBConfig *fbc = glXChooseFBConfig(m_display, m_screen, attributeList, &m_fb_count);
		if (!fbc) {
			fprintf(stderr, " - Error. Failed to get choose FBConfig in "
				"COpenglVideo::CreateContext\n");
			goto reload_for_osmesa;
		} else if (m_verbose) fprintf(stderr, " - GLX Config FB count %d\n", m_fb_count);
/*		for (int i=0 ; i<m_fb_count; i++) {
			int maxdepth, maxbw, maxbh, maxbp;
			glXGetFBConfigAttrib(m_display, fbc[i], GLX_DEPTH_SIZE, &maxdepth);
			glXGetFBConfigAttrib(m_display, fbc[i], GLX_MAX_PBUFFER_WIDTH, &maxbw);
			glXGetFBConfigAttrib(m_display, fbc[i], GLX_MAX_PBUFFER_HEIGHT, &maxbh);
			glXGetFBConfigAttrib(m_display, fbc[i], GLX_MAX_PBUFFER_PIXELS, &maxbp);
			fprintf(stderr, "   - FB %d depth %d maxbufw %d maxbufh %d maxbufp %d\n",
				i, maxdepth, maxbw, maxbh, maxbp);
		}
*/
		
		// XVisualInfo *vi = glXChooseVisual(m_display, m_screen, attributeList);
		if (!(m_vi = glXGetVisualFromFBConfig(m_display, fbc[0]))) {
			fprintf(stderr, "Failed to get get a visual in "
				"COpenglVideo::CreateContext\n");
			goto reload_for_osmesa;
		}
 
		XSetWindowAttributes swa;
		swa.colormap = XCreateColormap(m_display, RootWindow(m_display, m_vi->screen),
			m_vi->visual, AllocNone);
		swa.border_pixel = 0;

		// http://stackoverflow.com/questions/9065669/x11-glx-fullscreen-mode
		// This should bypass the window manager
		swa.override_redirect = true;

		swa.event_mask = StructureNotifyMask;
		if (m_verbose) fprintf(stderr, " - Creating window %ux%u depth %d\n",
			width, height, m_vi->depth);
		m_win = XCreateWindow(m_display, RootWindow(m_display, m_vi->screen),
			0, 0, width, height, 0,
			m_vi->depth, InputOutput, m_vi->visual,
			CWBorderPixel|CWColormap|CWEventMask, &swa);
 
		XMapWindow (m_display, m_win);

		// Signal to window manager that window should not be resized
		XSizeHints xsizeh;
		xsizeh.flags      = USPosition | USSize;
		xsizeh.x          = 0;
		xsizeh.y          = 0;
		xsizeh.width      = width;
		xsizeh.height     = height;
		xsizeh.min_width  = width;
		xsizeh.min_height = height;
		xsizeh.max_width  = width;
		xsizeh.max_height = height;
		XSetNormalHints(m_display, m_win, &xsizeh );
		XSetWMSizeHints(m_display, m_win, &xsizeh, PSize | PMinSize | PMaxSize );
		XStoreName(m_display, m_win, "Snowmix OpenGL Hardware Accelerated Video Mixer - "
			"DO NOT RESIZE OR CLOSE THE WINDOW.");

		if (!(m_pCtx_GLX = glXCreateContext(m_display, m_vi, 0, GL_TRUE))) {
			fprintf(stderr, " - Failed to create GLX context in "
				"COpenglVideo::CreateContext\n");
			goto reload_for_osmesa;
		}
		XIconifyWindow(m_display, m_win, 0);
#endif
	} else {

#ifdef HAVE_OSMESA
		// Use OSMesa
		if (m_pCtx_OSMesa) {
			if (m_verbose) fprintf(stderr, " - OSMesa context existed. "
				"Destroying it first.\n");
			DestroyContext();
		}
		if (!(m_pCtx_OSMesa = OSMesaCreateContextExt( OSMESA_BGRA, depth_bits, 0, 0, NULL ))) {
			if (m_verbose) fprintf(stderr, " - Error. OSMesaCreateContext Failed\n");
			return -1;
		}
#else
		fprintf(stderr, "OSMesa was not included at compile time.\n");
		return -1;
#endif	// HAVE_OSMESA
	}

//	// reset texture_ids
//	if (m_textures)
//		for (u_int32_t id = 0; id < m_max_textures; id++)
//			if (m_textures[id]) m_textures[id]->real_texture_id = 0;
//
	return n;

reload_for_osmesa:
	fprintf(stderr, "Reverting to OSMesa\n");
	if (m_display) {
		XCloseDisplay(m_display);
		m_display = NULL;
	}
	if (m_vi) { XFree(m_vi); m_vi = NULL; }

	m_use_glx = false;
	return CreateContext(baseframe, width, height, depth_bits);
}

/*
static void
Sphere(float radius, int slices, int stacks)
{
   GLUquadric *q = gluNewQuadric();
   gluQuadricNormals(q, GLU_SMOOTH);
   gluSphere(q, radius, slices, stacks);
   gluDeleteQuadric(q);
}


static void
Cone(float base, float height, int slices, int stacks)
{
   GLUquadric *q = gluNewQuadric();
   gluQuadricDrawStyle(q, GLU_FILL);
   gluQuadricNormals(q, GLU_SMOOTH);
   gluCylinder(q, base, 0.0, height, slices, stacks);
   gluDeleteQuadric(q);
}


static void
Torus(float innerRadius, float outerRadius, int sides, int rings)
{
   // from GLUT... 
   int i, j;
   GLfloat theta, phi, theta1;
   GLfloat cosTheta, sinTheta;
   GLfloat cosTheta1, sinTheta1;
   const GLfloat ringDelta = 2.0 * M_PI / rings;
   const GLfloat sideDelta = 2.0 * M_PI / sides;

   theta = 0.0;
   cosTheta = 1.0;
   sinTheta = 0.0;
   for (i = rings - 1; i >= 0; i--) {
      theta1 = theta + ringDelta;
      cosTheta1 = cos(theta1);
      sinTheta1 = sin(theta1);
      glBegin(GL_QUAD_STRIP);
      phi = 0.0;
      for (j = sides; j >= 0; j--) {
	 GLfloat cosPhi, sinPhi, dist;

	 phi += sideDelta;
	 cosPhi = cos(phi);
	 sinPhi = sin(phi);
	 dist = outerRadius + innerRadius * cosPhi;

	 glNormal3f(cosTheta1 * cosPhi, -sinTheta1 * cosPhi, sinPhi);
	 glVertex3f(cosTheta1 * dist, -sinTheta1 * dist, innerRadius * sinPhi);
	 glNormal3f(cosTheta * cosPhi, -sinTheta * cosPhi, sinPhi);
	 glVertex3f(cosTheta * dist, -sinTheta * dist,  innerRadius * sinPhi);
      }
      glEnd();
      theta = theta1;
      cosTheta = cosTheta1;
      sinTheta = sinTheta1;
   }
}
*/




/*
void DeleteTexture(GLuint textureId) {
	glDeleteTextures(1, textureId);
}
*/


#endif // #if defined (HAVE_OSMESA) || defined (HAVE_GLX)
