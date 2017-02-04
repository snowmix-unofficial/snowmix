/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


//#include <unistd.h>
//#include <stdio.h>
#include <ctype.h>
//#include <limits.h>
#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#ifdef WIN32
#define _USE_MATH_DEFINES
#endif
#include <math.h>
//#include <stdlib.h>
//#include <sys/types.h>
//#include <math.h>
//#include <string.h>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>

//#include "cairo/cairo.h"
//#include "video_mixer.h"
//#include "video_text.h"
//#include "video_scaler.h"
//#include "add_by.h"
//#include "video_image.h"

#include "snowmix.h"
#include "video_shape.h"
#include "snowmix_util.h"

#define min0max1(a) (a < 0 ? 0 : a > 1 ? 1 : a)

CVideoShape::CVideoShape(CVideoMixer* pVideoMixer, u_int32_t max_shapes, u_int32_t max_draw_places, u_int32_t max_patterns)
{
	m_max_shapes = max_shapes;
	m_max_shape_places = max_draw_places;
	m_max_patterns = max_patterns;
	m_shape_count = 0;
	m_shape_placed_count = 0;
	m_pattern_count = 0;
	m_verbose = 0;
	m_shapes = NULL;
	m_placed_shapes = NULL;
	m_update = false;
	if (!(m_pVideoMixer = pVideoMixer))
		fprintf(stderr, "Warning : pVideoMixer set to NULL for CVideoShape\n");
	if (m_max_shapes)
		m_shapes = (shape_t**) calloc(sizeof(shape_t*), m_max_shapes);
	if (m_max_shape_places)
		m_placed_shapes = (placed_shape_t**) calloc(sizeof(placed_shape_t*), m_max_shape_places);
	if (m_max_patterns)
		m_patterns = (pattern_t**) calloc(sizeof(pattern_t*), m_max_patterns);
	if (!m_shapes)
		fprintf(stderr, "Warning : no space for shape table in CVideoShape\n");
	if (!m_placed_shapes)
		fprintf(stderr, "Warning : no space for shape place table in CVideoShape\n");
	if (!m_patterns)
		fprintf(stderr, "Warning : no space for shape pattern table in CVideoShape\n");
}

	

CVideoShape::~CVideoShape()
{
	if (m_shapes) {
		for (u_int32_t id=0; id < m_max_shapes; id++)
			if (m_shapes[id]) DeleteShape(id);
	}
	if (m_placed_shapes) {
		for (u_int32_t id=0; id < m_max_shapes; id++)
			if (m_placed_shapes[id]) DeletePlaced(id);
	}
	if (m_patterns) {
		for (u_int32_t id=0; id < m_max_patterns; id++)
			if (m_patterns[id]) DeletePattern(id);
	}
}

int CVideoShape::DeleteShape(u_int32_t shape_id)
{
	if (shape_id >= m_max_shapes || !m_shapes || !m_shapes[shape_id]) return -1;
	//if (m_shapes[shape_id]->name) free(m_shapes[shape_id]->name);
	draw_operation_t* p = m_shapes[shape_id]->pDraw;
	while (p) {
		draw_operation_t* next = p->next;
		if (p->create_str) free(p->create_str);
		free(p);
		p = next;
	}
	free (m_shapes[shape_id]);
	m_shapes[shape_id] = NULL;
	m_shape_count--;
	return 0;
}

int CVideoShape::DeletePlaced(u_int32_t place_id)
{
	if (place_id >= m_max_shape_places || !m_placed_shapes ||
		!m_placed_shapes[place_id]) return -1;
	free (m_placed_shapes[place_id]);
	m_placed_shapes[place_id] = NULL;
	m_shape_placed_count--;
	return 0;
}

int CVideoShape::DeletePattern(u_int32_t pattern_id)
{
	if (pattern_id >= m_max_patterns || !m_patterns ||
		!m_patterns[pattern_id]) return -1;
	if (m_patterns[pattern_id]->pattern)
		cairo_pattern_destroy(m_patterns[pattern_id]->pattern);
	free (m_patterns[pattern_id]);
	m_patterns[pattern_id] = NULL;
	m_pattern_count--;
	return 0;
}

// shape ....
int CVideoShape::ParseCommand(struct controller_type* ctr, const char* str)
{
	// shape
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	
	if (!strncasecmp (str, "overlay ", 8)) {
		return overlay_placed_shape(ctr, str+8);
	}
	else if (!strncasecmp (str, "place overlay ", 14)) {
		return overlay_placed_shape(ctr, str+14);
	// shape info
	}
	else if (!strncasecmp (str, "info ", 5)) {
		return list_shape_info(ctr, str+5);
	}
	// shape moveentry <shape id> <entry> [<to entry>]
	else if (!strncasecmp (str, "moveentry ", 10)) {
		return set_shape_moveentry(ctr, str+10);
	}
	else if (!strncasecmp (str, "place ", 6)) {
		str += 6;
		if (!strncasecmp (str, "move ", 5)) {
			str += 5;
			// shape place move coor <place id> <delta x> <delta y> <steps x> <steps y>
			if (!strncasecmp (str, "coor ", 5)) {
				return set_shape_place_move_coor(ctr, str+5);
			}
			else if (!strncasecmp (str, "offset ", 7)) {
				return set_shape_place_move_offset(ctr, str+7);
			}
			else if (!strncasecmp (str, "scale ", 6)) {
				return set_shape_place_move_scale(ctr, str+6);
			}
			else if (!strncasecmp (str, "alpha ", 6)) {
				return set_shape_place_move_alpha(ctr, str+6);
			}
			else if (!strncasecmp (str, "rotation ", 9)) {
				return set_shape_place_move_rotation(ctr, str+9);
			} else return -1;
		}
		// shape place anchor [<place id> ((n | s | e | w | c | ne | nw | se | sw)]
		else if (!strncasecmp (str, "anchor ", 7)) {
			return set_shape_place_anchor(ctr, str+7);
		}
		// shape place coor <place id> <x> <y> [<rotation>]
		else if (!strncasecmp (str, "coor ", 5)) {
			return set_shape_place_coor(ctr, str+5);
		}
		// shape place shape <place id> <shape id>
		if (!strncasecmp (str, "shape ", 6)) {
			return set_shape_place_shape(ctr, str+6);
		}
		// shape place offset <place id> <offset x> <offset y>
		if (!strncasecmp (str, "offset ", 7)) {
			return set_shape_place_offset(ctr, str+7);
		}
		// shape place scale <place id> <scale x> <scale y>
		if (!strncasecmp (str, "scale ", 6)) {
			return set_shape_place_scale(ctr, str+6);
		}
		// shape place rotation <place id> <rotation>
		if (!strncasecmp (str, "rotation ", 9)) {
			return set_shape_place_rotation(ctr, str+9);
		}
		// shape place rgb <place id> <red> <green> <blue> [<alpha>]
		if (!strncasecmp (str, "rgb ", 4)) {
			return set_shape_place_rgb(ctr, str+4);
		}
		// shape place alpha <place id> <alpha>
		if (!strncasecmp (str, "alpha ", 6)) {
			return set_shape_place_alpha(ctr, str+6);
		} else if (!strncasecmp (str, "status ", 7)) {
			return list_shape_status(ctr, str+7);
		}
		else if (!strncasecmp (str, "help ", 5)) {
			return list_shape_place_help(ctr, str+5);
		}
		// shape place [<place id> [ <shape id> <x> <y> <scale x> <scale y>
		//                  [<rotation> [<red> <green> <blue> [<alpha>]]]]]
		else return set_shape_place(ctr, str);
	}
	// shape list <shape id>
	else if (!strncasecmp (str, "list ", 5)) {
		return set_shape_list(ctr, str+5);
	}
	// shape add [shape id> [<shape name>]]
	else if (!strncasecmp (str, "add ", 4)) {
		return set_shape_add(ctr, str+4);
	}
	// shape inshape <shape id> <inshape_id>
	else if (!strncasecmp (str, "inshape ", 8)) {
		return set_shape_one_parameter(ctr, str+8, SHAPE);
	}
	else if (!strncasecmp (str, "translate ", 10)) {
		return set_shape_xy(ctr, str+10, TRANSLATE);
	}
	else if (!strncasecmp (str, "moverel ", 8)) {
		return set_shape_xy(ctr, str+8, MOVEREL);
	}
	else if (!strncasecmp (str, "moveto ", 7)) {
		return set_shape_xy(ctr, str+7, MOVETO);
	}
	else if (!strncasecmp (str, "lineto ", 7)) {
		return set_shape_xy(ctr, str+7, LINETO);
	}
	else if (!strncasecmp (str, "linerel ", 8)) {
		return set_shape_xy(ctr, str+8, LINEREL);
	}
	else if (!strncasecmp (str, "scale ", 6)) {
		return set_shape_xy(ctr, str+6, SCALE);
	}
	else if (!strncasecmp (str, "arc_cw ", 7)) {
		return set_shape_arc(ctr, str+7, ARC_CW);
	}
	else if (!strncasecmp (str, "arc_ccw ", 8)) {
		return set_shape_arc(ctr, str+8, ARC_CCW);
	}
	else if (!strncasecmp (str, "arcrel_cw ", 10)) {
		return set_shape_arc(ctr, str+10, ARCREL_CW);
	}
	else if (!strncasecmp (str, "arcrel_ccw ", 11)) {
		return set_shape_arc(ctr, str+11, ARCREL_CCW);
	}
	else if (!strncasecmp (str, "source ", 7)) {
		str += 7;
		if (!strncasecmp (str, "pattern ", 8)) {
			return set_shape_source_pattern(ctr, str+8);
		}
		else if (!strncasecmp (str, "rgb ", 4)) {
			return set_shape_source_rgb(ctr, str+4);
		}
		else if (!strncasecmp (str, "rgba ", 5)) {
			return set_shape_source_rgb(ctr, str+5);
		}
		else if (!strncasecmp (str, "alphaadd ", 9)) {
			return set_shape_one_parameter(ctr, str+9, ALPHAADD);
		}
		else if (!strncasecmp (str, "alphamul ", 9)) {
			return set_shape_one_parameter(ctr, str+9, ALPHAMUL);
		}
	}
	else if (!strncasecmp (str, "mask pattern ", 13)) {
		return set_shape_mask_pattern(ctr, str+13);
	}
	else if (!strncasecmp (str, "rectangle ", 10)) {
		return set_shape_rectangle(ctr, str+10);
	}
	else if (!strncasecmp (str, "closepath ", 10)) {
		return set_shape_no_parameter(ctr, str+10, CLOSEPATH);
	}
	else if (!strncasecmp (str, "tell scale ", 11)) {
		return set_shape_no_parameter(ctr, str+11, SCALETELL);
	}
	else if (!strncasecmp (str, "newpath ", 8)) {
		return set_shape_no_parameter(ctr, str+8, NEWPATH);
	}
	else if (!strncasecmp (str, "newsubpath ", 11)) {
		return set_shape_no_parameter(ctr, str+11, NEWSUBPATH);
	}
	else if (!strncasecmp (str, "clip ", 5)) {
		return set_shape_no_parameter(ctr, str+5, CLIP);
	}
	else if (!strncasecmp (str, "stroke preserve ", 16)) {
		return set_shape_no_parameter(ctr, str+16, STROKEPRESERVE);
	}
	else if (!strncasecmp (str, "stroke ", 7)) {
		return set_shape_no_parameter(ctr, str+7, STROKE);
	}
	else if (!strncasecmp (str, "fill preserve ", 14)) {
		return set_shape_no_parameter(ctr, str+14, FILLPRESERVE);
	}
	else if (!strncasecmp (str, "fill ", 5)) {
		return set_shape_no_parameter(ctr, str+5, FILL);
	}
	else if (!strncasecmp (str, "save ", 5)) {
		return set_shape_no_parameter(ctr, str+5, SAVE);
	}
	else if (!strncasecmp (str, "restore ", 8)) {
		return set_shape_no_parameter(ctr, str+8, RESTORE);
	}
	else if (!strncasecmp (str, "line ", 5)) {
		str += 5;
		if (!strncasecmp (str, "width ", 6)) {
			return set_shape_one_parameter(ctr, str+6, LINEWIDTH);
		}
		else if (!strncasecmp (str, "cap ", 4)) {
			return set_shape_one_parameter(ctr, str+4, LINECAP);
		}
		else if (!strncasecmp (str, "join ", 5)) {
			return set_shape_line_join(ctr, str+5);
		}
		else return -1;
	}
	else if (!strncasecmp (str, "paint ", 6)) {
		return set_shape_one_parameter(ctr, str+6, PAINT);
	}
	else if (!strncasecmp (str, "filter ", 7)) {
		return set_shape_filter(ctr, str+7);
	}
	else if (!strncasecmp (str, "operator ", 9)) {
		return set_shape_operator(ctr, str+9);
	}
	else if (!strncasecmp (str, "curveto ", 8)) {
		return set_shape_curve(ctr, str+8, CURVETO);
	}
	else if (!strncasecmp (str, "curverel ", 9)) {
		return set_shape_curve(ctr, str+9, CURVEREL);
	}
	else if (!strncasecmp (str, "rotation ", 9)) {
		return set_shape_rotation(ctr, str+9);
	}
	else if (!strncasecmp (str, "image ", 6)) {
		return set_shape_image(ctr, str+6);
	}
	else if (!strncasecmp (str, "feed ", 5)) {
		return set_shape_feed(ctr, str+5);
	}
	else if (!strncasecmp (str, "pattern ", 8)) {
		str += 8;
		if (!strncasecmp (str, "stoprgb ", 8)) {
			return set_pattern_stoprgba(ctr, str+8);
		}
		else if (!strncasecmp (str, "linear ", 7)) {
			return set_pattern_linear(ctr, str+7);
		}
		else if (!strncasecmp (str, "radial ", 7)) {
			return set_pattern_radial(ctr, str+7);
		}
		else if (!strncasecmp (str, "add ", 4)) {
			return set_pattern_add(ctr, str+4);
		}
	}
	else if (!strncasecmp (str, "recursion ", 10)) {
		return set_shape_one_parameter(ctr, str+10, RECURSION);
	}
	else if (!strncasecmp (str, "transform ", 10)) {
		return set_shape_transform(ctr, str+10);
	}
	else if (!strncasecmp (str, "verbose ", 8)) {
		return set_verbose(ctr, str+8);
	}
	else if (!strncasecmp (str, "help ", 5)) {
		return list_shape_help(ctr, str+5);
	}
	return -1;
}


// shape verbose [<level>] // set control channel to write out
int CVideoShape::set_verbose(struct controller_type* ctr, const char* str)
{
	int n;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_verbose) m_verbose = 0;
		else m_verbose = 1;
		if (m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"shape verbose %s\n", m_verbose ? "on" : "off");
		return 0;
	}
	while (isspace(*str)) str++;
	n = sscanf(str, "%u", &m_verbose);
	if (n != 1) return -1;
	if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape verbose level set to %u for CVideoShape\n",
			m_verbose);
	return 0;
}

int CVideoShape::list_shape_info(struct controller_type* ctr, const char* str)
{
	if (!ctr || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS" shape info\n"
		STATUS" verbose level : %u\n"
		STATUS" shapes        : %u of %u\n"
		STATUS" placed shapes : %u of %u\n"
		STATUS" shapes : ops_count shape_name\n",
		m_verbose,
		m_shape_count, m_max_shapes,
		m_shape_placed_count, m_max_shape_places);
	if (m_shapes) for (u_int32_t id=0 ; id < m_max_shapes; id++) if (m_shapes[id])
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS" - shape %u : %u %s\n",
			id, m_shapes[id]->ops_count, m_shapes[id]->name);
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}

int CVideoShape::list_shape_status(struct controller_type* ctr, const char* str)
{
	if (!ctr || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS" shape status\n"
		STATUS" shape place id : x y scale_x scale_y rotation rgb alpha\n");

	if (m_placed_shapes) for (u_int32_t id=0 ; id < m_max_shape_places; id++)
		if (m_placed_shapes[id])
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS" - shape place %u : %.4lf %.4lf %.4lf %.4lf %.4lf "
				"%.4lf,%.4lf,%.4lf %.4lf\n",
				id, m_placed_shapes[id]->x, m_placed_shapes[id]->y,
				m_placed_shapes[id]->scale_x, m_placed_shapes[id]->scale_y,
				m_placed_shapes[id]->rotation,
				m_placed_shapes[id]->red,
				m_placed_shapes[id]->green,
				m_placed_shapes[id]->blue,
				m_placed_shapes[id]->alpha);
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}


int CVideoShape::set_shape_moveentry(struct controller_type* ctr, const char* str)
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
				MESSAGE"shape %u moventry %u%s", shape_id, entry_id,
				n == 2 ? " (deleted)\n" : "");
			if (n == 3) m_pVideoMixer->m_pController->controller_write_msg (ctr,
				" to %u\n", to_entry);
		}
		return 0;
	}
	return -1;
}

int CVideoShape::set_shape_place_anchor(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++)
				if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place %u anchor %d,%d\n", id,
					m_placed_shapes[id]->anchor_x, m_placed_shapes[id]->anchor_y);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t id;
	char anchor[3];
	anchor[0] = '\0';
	if (sscanf(str, "%u %2["ANCHOR_TAGS"]", &id, &anchor[0]) != 2) return -1;
	int n = SetShapeAnchor(id, anchor);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"shape place %u anchor %s\n", id, anchor);
	return n;
}

int CVideoShape::set_shape_place_coor(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place %u at %.4lf,%.4lf offset "
					"%.4lf,%.4lf rotation %.4lf\n", id,
					m_placed_shapes[id]->x, m_placed_shapes[id]->y,
					id, m_placed_shapes[id]->offset_x, m_placed_shapes[id]->offset_y,
					m_placed_shapes[id]->rotation);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id;
	double x, y;
	char anchor[3];
	anchor[0] = '\0';

	int n = sscanf(str, "%u %lf %lf %2["ANCHOR_TAGS"]", &place_id, &x, &y, &anchor[0]);
	if (n < 3) return -1;
	n = SetPlacedShapeCoor(place_id, x, y, anchor);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place coor shape %u coor %.4lf,%.4lf%s%s\n", 
			place_id, x, y, anchor[0] ? " " : "", anchor);
	return n;
}

int CVideoShape::set_shape_place_move_coor(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place move coor %u delta %.4lf,%.4lf steps %u,%u\n",
					id, m_placed_shapes[id]->delta_x, m_placed_shapes[id]->delta_y,
					m_placed_shapes[id]->steps_x, m_placed_shapes[id]->steps_y);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id, steps_x, steps_y;
	double delta_x, delta_y;

	int n = sscanf(str, "%u %lf %lf %u %u", &place_id, &delta_x, &delta_y, &steps_x, &steps_y);
	if (n != 5) return -1;

	n = SetPlacedShapeMoveCoor(place_id, delta_x, delta_y, steps_x, steps_y);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place move coor shape %u delta %.4lf,%.4lf steps %u,%u\n", 
			place_id, delta_x, delta_y, steps_x, steps_y);
	return n;
}

int CVideoShape::set_shape_place_move_offset(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place move offset %u delta %.4lf,%.4lf steps %u,%u\n",
					id, m_placed_shapes[id]->delta_offset_x, m_placed_shapes[id]->delta_offset_y,
					m_placed_shapes[id]->steps_offset_x, m_placed_shapes[id]->steps_offset_y);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id, steps_x, steps_y;
	double delta_x, delta_y;

	int n = sscanf(str, "%u %lf %lf %u %u", &place_id, &delta_x, &delta_y, &steps_x, &steps_y);
	if (n != 5) return -1;

	n = SetPlacedShapeMoveOffset(place_id, delta_x, delta_y, steps_x, steps_y);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place move offset shape %u delta %.4lf,%.4lf steps %u,%u\n", 
			place_id, delta_x, delta_y, steps_x, steps_y);
	return n;
}

int CVideoShape::set_shape_place_move_scale(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place move scale %u delta %.4lf,%.4lf steps %u,%u\n",
					id, m_placed_shapes[id]->delta_scale_x, m_placed_shapes[id]->delta_scale_y,
					m_placed_shapes[id]->steps_scale_x, m_placed_shapes[id]->steps_scale_y);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id, steps_x, steps_y;
	double delta_x, delta_y;

	int n = sscanf(str, "%u %lf %lf %u %u", &place_id, &delta_x, &delta_y, &steps_x, &steps_y);
	if (n != 5) return -1;

	n = SetPlacedShapeMoveScale(place_id, delta_x, delta_y, steps_x, steps_y);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place move scale shape %u delta %.4lf,%.4lf steps %u,%u\n", 
			place_id, delta_x, delta_y, steps_x, steps_y);
	return n;
}

int CVideoShape::set_shape_place_move_alpha(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place move alpha %u delta %.4lf steps %u\n",
					id, m_placed_shapes[id]->delta_alpha,
					m_placed_shapes[id]->steps_alpha);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id, steps_alpha;
	double delta_alpha;

	int n = sscanf(str, "%u %lf %u", &place_id, &delta_alpha, &steps_alpha);
	if (n != 3) return -1;

	n = SetPlacedShapeMoveAlpha(place_id, delta_alpha, steps_alpha);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place move alpha shape %u delta %.4lf steps %u\n", 
			place_id, delta_alpha, steps_alpha);
	return n;
}

int CVideoShape::set_shape_place_move_rotation(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place move rotation %u delta %.4lf steps %u\n",
					id, m_placed_shapes[id]->delta_rotation,
					m_placed_shapes[id]->steps_rotation);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id, steps_rotation;
	double delta_rotation;

	int n = sscanf(str, "%u %lf %u", &place_id, &delta_rotation, &steps_rotation);
	if (n != 3) return -1;

	n = SetPlacedShapeMoveRotation(place_id, delta_rotation, steps_rotation);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place move rotation shape %u delta %.4lf steps %u\n", 
			place_id, delta_rotation, steps_rotation);
	return n;
}

int CVideoShape::set_shape_place_shape(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place %u shape %u\n",
					id, m_placed_shapes[id]->shape_id);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id, shape_id;

	if (sscanf(str, "%u %u", &place_id, &shape_id) != 2) return -1;
	int n = SetPlacedShapeShape(place_id, shape_id);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place %u shape %u\n", place_id, shape_id);
	}
	return n;
}

int CVideoShape::set_shape_place_scale(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place %u scale %.3lf,%.3lf\n",
					id, m_placed_shapes[id]->scale_x, m_placed_shapes[id]->scale_y);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id;
	double scale_x, scale_y;

	if (sscanf(str, "%u %lf %lf", &place_id, &scale_x, &scale_y) != 3) return -1;
	int n = SetPlacedShapeScale(place_id, scale_x, scale_y);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place scale shape %u scale %.4lf,%.4lf\n", 
			place_id, scale_x, scale_y);
	return n;
}

int CVideoShape::set_shape_place_offset(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place %u at %.4lf,%.4lf offset %.4lf,%.4lf\n", id,
					m_placed_shapes[id]->x, m_placed_shapes[id]->y,
					m_placed_shapes[id]->offset_x, m_placed_shapes[id]->offset_y);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id;
	double offset_x, offset_y;

	if (sscanf(str, "%u %lf %lf", &place_id, &offset_x, &offset_y) != 3) return -1;
	int n = SetPlacedShapeOffset(place_id, offset_x, offset_y);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place %u offset %.4lf,%.4lf\n", 
			place_id, offset_x, offset_y);
	return n;
}

int CVideoShape::set_shape_place_rotation(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place %u rotation %.3lf\n",
					id, m_placed_shapes[id]->rotation);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id;
	double rotation;

	if (sscanf(str, "%u", &place_id) != 1) return -1;
	while (*str && !isspace(*str)) str++;
	if (NumberWithPI(str, &rotation)) {
		int n = SetPlacedShapeRotation(place_id, rotation);
		if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"shape place %u rotation %.4lf\n", 
				place_id, rotation);
		return n;
	}
	return -1;
}

// shape place alpha [<place id> <alpha>]
int CVideoShape::set_shape_place_alpha(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place %u rgb %.3lf,%.3lf,%.3lf alpha %.3lf\n",
					id, m_placed_shapes[id]->red, m_placed_shapes[id]->green,
					m_placed_shapes[id]->blue, m_placed_shapes[id]->alpha);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id;
	double alpha;

	if (sscanf(str, "%u %lf", &place_id, &alpha) != 2) return -1;
	int n = SetPlacedShapeAlpha(place_id, alpha);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place %u alpha %.4lf\n", 
			place_id, alpha);
	return n;
}

// shape place alpha <place id> <red> <green> <blue> [<alpha>]
int CVideoShape::set_shape_place_rgb(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place %u rgb %.3lf,%.3lf,%.3lf alpha %.3lf\n",
					id, m_placed_shapes[id]->red, m_placed_shapes[id]->green,
					m_placed_shapes[id]->blue, m_placed_shapes[id]->alpha);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id;
	double red, green, blue, alpha;
	alpha = 1.0;

	int n = sscanf(str, "%u %lf %lf %lf %lf", &place_id, &red, &green, &blue, &alpha);

	if (n == 4) {
		n = SetPlacedShapeRGB(place_id, red, green, blue);
		if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"shape place %u rgb %.4lf,%.4lf,%.4lf\n",
				place_id, red, green, blue);
		}
	} else

	if (n == 5) {
		n = SetPlacedShapeRGB(place_id, red, green, blue);
		if (n) return n;
		n = SetPlacedShapeAlpha(place_id, alpha);
		if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"shape place %u rgba %.4lf,%.4lf,%.4lf,%.4lf\n",
				place_id, red, green, blue, alpha);
		}
	} else n = -1;
	return n;
}


int CVideoShape::set_shape_place(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_placed_shapes) {
			for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape place %u shape %u at %.3lf,%.3lf off "
					"%.4lf,%.4lf rot %.3lf scale %.3lf,%.3lf "
					"rgb %.3lf,%.3lf,%.3lf alpha %.3lf anchor %u,%u"
/*
					"ext %.lf,%.1lf,%.1lf,%.1lf"
*/
					"\n",
					id, m_placed_shapes[id]->shape_id,
					m_placed_shapes[id]->x, m_placed_shapes[id]->y,
					m_placed_shapes[id]->offset_x,
					m_placed_shapes[id]->offset_y,
					m_placed_shapes[id]->rotation,
					m_placed_shapes[id]->scale_x,
					m_placed_shapes[id]->scale_y,
					m_placed_shapes[id]->red,
					m_placed_shapes[id]->green,
					m_placed_shapes[id]->blue,
					m_placed_shapes[id]->alpha,
					m_placed_shapes[id]->anchor_x,
					m_placed_shapes[id]->anchor_y
/*
					,m_placed_shapes[id]->ext_x1,
					m_placed_shapes[id]->ext_y1,
					m_placed_shapes[id]->ext_x2,
					m_placed_shapes[id]->ext_y2
*/
					);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t place_id, shape_id;

	if (sscanf(str, "%u", &place_id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (place_id < m_max_shape_places && m_placed_shapes && m_placed_shapes[place_id]) {
			int n = DeletePlaced(place_id);
			if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"shape place %u deleted\n", place_id);
			}
			return n;
		}
		return -1;
	}
	double x, y, scale_x, scale_y;
	double rotation = 0.0;
	double red = 0.0;
	double green = 0.0;
	double blue = 0.0;
	double alpha = 1.0;
	char anchor[3];
	anchor[0] = '\0';
	if (sscanf(str, "%u %lf %lf %lf %lf", &shape_id, &x, &y, &scale_x, &scale_y) != 5)
		return -1;
	while (!isspace(*str)) str++;	// shape_id
	while (isspace(*str)) str++;
	while (!isspace(*str)) str++;	// x
	while (isspace(*str)) str++;
	while (!isspace(*str)) str++;	// y
	while (isspace(*str)) str++;
	while (!isspace(*str)) str++;	// scale_x
	while (isspace(*str)) str++;
	while (*str && !isspace(*str)) str++;	// scale_y
	while (isspace(*str)) str++;
	if (*str) {
		if (sscanf(str, "%lf", &rotation) != 1) return -1;
		while (*str && !isspace(*str)) str++;	// rotation
		while (isspace(*str)) str++;
		if (*str) {
			if (sscanf(str, "%lf %lf %lf", &red, &green, &blue) != 3) return -1;
			while (!isspace(*str)) str++;	// red
			while (isspace(*str)) str++;
			while (!isspace(*str)) str++;	// green
			while (isspace(*str)) str++;
			while (*str && !isspace(*str)) str++;	// blue
			while (isspace(*str)) str++;
			if (*str) {
				if (sscanf(str, "%lf %2["ANCHOR_TAGS"]", &alpha, &anchor[0]) < 1) return -1;
			}
		}
	}
	int n = AddPlacedShape(place_id, shape_id, x, y, scale_x, scale_y, rotation,
		red, green, blue, alpha, anchor);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape place %u shape %u at %.1lf,%.1lf scale %.3lf,%.3lf "
			"rotation %.4lf rgb %.3lf,%.3lf,%.3lf alpha %.3lf%s%s\n",
			place_id, shape_id, x, y, scale_x, scale_y, rotation,
			m_placed_shapes[place_id]->red,
			m_placed_shapes[place_id]->green,
			m_placed_shapes[place_id]->blue,
			m_placed_shapes[place_id]->alpha, anchor[0] ? " anchor " : "", anchor);
	}
	return n;
}

int CVideoShape::set_shape_add(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_shapes) {
			for (u_int32_t id=0; id < m_max_shapes; id++) if (m_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape %u ops %u name %s\n",
					id, m_shapes[id]->ops_count, m_shapes[id]->name);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t shape_id = 0;
	if (sscanf(str, "%u", &shape_id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (shape_id < m_max_shapes && m_shapes && m_shapes[shape_id]) {
			int n = DeleteShape(shape_id);
			if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"shape %u deleted\n", shape_id);
			}
			return n;
		}
	} else {
		int n = AddShape(shape_id, str);
		if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"shape %u add %s\n", shape_id, str);
		}
		return n;
	}
	return -1;
}

int CVideoShape::set_pattern_add(struct controller_type* ctr, const char* str)
{
//	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		if (m_patterns) {
			for (u_int32_t id=0; id < m_max_patterns; id++) if (m_patterns[id]) {
				int stop_count = -1;
				cairo_pattern_type_t type = CAIRO_PATTERN_TYPE_SOLID;
				if (m_patterns[id]->pattern) {
					if (cairo_pattern_get_color_stop_count(
						m_patterns[id]->pattern, &stop_count) !=
						CAIRO_STATUS_SUCCESS) stop_count = -1;
					type = cairo_pattern_get_type(m_patterns[id]->pattern);
				}
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape pattern %u type %s stops %d name %s\n",
					id, type == CAIRO_PATTERN_TYPE_SOLID ? "solid" :
					  type == CAIRO_PATTERN_TYPE_SURFACE ? "surface" :
					    type == CAIRO_PATTERN_TYPE_LINEAR ? "linear" :
					      type == CAIRO_PATTERN_TYPE_RADIAL ? "radial" :
#ifdef CAIRO_PATTERN_TYPE_MESH
					        type == CAIRO_PATTERN_TYPE_MESH ? "mesh" :
#endif
#ifdef CAIRO_PATTERN_TYPE_RASTER_SOURCE
					          type == CAIRO_PATTERN_TYPE_RASTER_SOURCE ? "raster" :
#endif
						"unknown",
					stop_count, m_patterns[id]->name ?
					  m_patterns[id]->name : "");
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	u_int32_t pattern_id = 0;
	if (sscanf(str, "%u", &pattern_id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (pattern_id < m_max_patterns && m_patterns && m_patterns[pattern_id]) {
			int n = DeletePattern(pattern_id);
			if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					MESSAGE"shape pattern %u deleted\n", pattern_id);
			}
			return n;
		}
	} else {
		int n = AddPattern(pattern_id, str);
		if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"shape pattern %u add %s\n", pattern_id, str);
		}
//		if (!n) {
//			if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
//				free(m_shapes[shape_id]->pDrawTail->create_str);
//			m_shapes[shape_id]->pDrawTail->create_str = strdup(create_str);
//		}
		return n;
	}
	return -1;
}

// shape pattern radial <pattern id> <cx0> <cy0> <radius0> <cx1> <cy1> <radius1>
int CVideoShape::set_pattern_radial(struct controller_type* ctr, const char* str)
{
//	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str) || !m_patterns) return -1;
	u_int32_t pattern_id;
	double cx0, cy0, cx1, cy1, radius0, radius1;
	if (sscanf(str, "%u %lf %lf %lf %lf %lf %lf",
		&pattern_id, &cx0, &cy0, &radius0, &cx1, &cy1, &radius1) != 7) return -1;
	int n = PatternCreateRadial(pattern_id, cx0, cy0, radius0, cx1, cy1, radius1);
	if (n) return -1;
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
		MESSAGE"shape pattern %u radial %u %.4lf %.4lf %.4lf %.4lf %.4lf %.4lf\n",
			pattern_id, cx0, cy0, radius0, cx1, cy1, radius1);
	}
//	if (!n) {
//		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
//			free(m_shapes[shape_id]->pDrawTail->create_str);
//		m_shapes[shape_id]->pDrawTail->create_str = strdup(create_str);
//	}
	return n;
}

int CVideoShape::set_pattern_stoprgba(struct controller_type* ctr, const char* str)
{
//	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str) || !m_patterns) return -1;
	u_int32_t pattern_id;
	double offset, red, green, blue, alpha;
	alpha = -1.0;
	int n = sscanf(str, "%u %lf %lf %lf %lf %lf", &pattern_id, &offset, &red, &green, &blue, &alpha);
	if (n != 5 && n != 6) return -1;
	n = PatternStopRGBA(pattern_id, offset, red, green, blue, alpha);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
		MESSAGE"shape pattern %u stoprgba offset %.4lf rgb %.4lf,%.4lf,%.4lf alpha %.4lf\n",
			pattern_id, alpha < 0.0 ? "" : "a",
			offset, red, green, blue, alpha < 0.0 ? 1.0 : alpha);
	}
//	if (!n) {
//		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
//			free(m_shapes[shape_id]->pDrawTail->create_str);
//		m_shapes[shape_id]->pDrawTail->create_str = strdup(create_str);
//	}
	return n;
}

// 
int CVideoShape::set_pattern_linear(struct controller_type* ctr, const char* str)
{
//	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str) || !m_patterns) return -1;
	u_int32_t pattern_id;
	double x1, y1, x2, y2;
	if (sscanf(str, "%u %lf %lf %lf %lf",
		&pattern_id, &x1, &y1, &x2, &y2) != 5) return -1;
	int n = PatternCreateLinear(pattern_id, x1, y1, x2, y2);
	if (n) return -1;
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
		MESSAGE"shape pattern %u linear %u %.4lf %.4lf %.4lf %.4lf\n",
			pattern_id, x1, y1, x2, y2);
	}
//	if (!n) {
//		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
//			free(m_shapes[shape_id]->pDrawTail->create_str);
//		m_shapes[shape_id]->pDrawTail->create_str = strdup(create_str);
//	}
	return n;
}

int CVideoShape::set_shape_list(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id;
	if (!str || !m_shapes) return -1;
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_shapes) {
			for (u_int32_t id=0; id < m_max_shapes; id++) if (m_shapes[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" shape %u ops %u name %s\n",
					id, m_shapes[id]->ops_count, m_shapes[id]->name);
			}

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	int n = sscanf(str, "%u", &shape_id);
	if (!n) return -1;
	if (shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr,
		STATUS" shape %u ops %u name %s\n", shape_id, m_shapes[shape_id]->ops_count,
		m_shapes[shape_id]->name);
	draw_operation_t* pDraw = m_shapes[shape_id]->pDraw;
	u_int32_t line = 1;
	while (pDraw) {
		//m_pVideoMixer->m_pController->controller_write_msg (ctr,
		//	STATUS" - draw op :\n");
		switch (pDraw->type) {
			case MOVETO:
			case MOVEREL:
			case SCALE:
			case LINETO:
			case TRANSLATE:
			case LINEREL:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u %s %.4lf %.4lf\n", line++,
					pDraw->type == MOVETO ? "moveto" :
					  pDraw->type == MOVEREL ? "moverel" :
					    pDraw->type == SCALE ? "scale" :
					      pDraw->type == TRANSLATE ? "translate" :
					        pDraw->type == LINETO ? "lineto" : "linerel",
					pDraw->x, pDraw->y);
				break;
			case CLOSEPATH:
			case NEWPATH:
			case NEWSUBPATH:
			case SCALETELL:
			case STROKE:
			case STROKEPRESERVE:
			case CLIP:
			case SAVE:
			case RESTORE:
			case FILL:
			case FILLPRESERVE:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u %s\n", line++,
					pDraw->type == CLOSEPATH ? "closepath" :
		  			  pDraw->type == NEWPATH ? "newpath" :
		  			    pDraw->type == NEWSUBPATH ? "newsubpath" :
		  			      pDraw->type == STROKE ? "stroke" :
		  			        pDraw->type == SCALETELL ? "tell scale" :
		  			          pDraw->type == STROKEPRESERVE ? "stroke preserve" :
		  			            pDraw->type == SAVE ? "save" :
		  			              pDraw->type == RESTORE ? "restore" :
		  			                pDraw->type == FILL ? "fill" :
		  			                  pDraw->type == FILLPRESERVE ? "fill preserve" :
		  			                    pDraw->type == CLIP ? "clip" : "unknown");
				break;
			case ROTATION:
				if (pDraw->parameter)
					m_pVideoMixer->m_pController->controller_write_msg (
						ctr, STATUS" - %u rotation %.4lf %.4lf\n",
						line++, pDraw->angle_from, pDraw->angle_to);
				else m_pVideoMixer->m_pController->controller_write_msg (
						ctr, STATUS" - %u rotation %.4lf\n",
						line++, pDraw->angle_from);
				break;
			case RECURSION:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u recursion %d\n", line++, pDraw->parameter);
				break;
			case PAINT:
				if (pDraw->alpha < 0.0)
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS" - %u paint\n", line++);
				else m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS" - %u paint %.4lf\n", line++, pDraw->alpha);
				break;
			case ARC_CW:
			case ARC_CCW:
			case ARCREL_CW:
			case ARCREL_CCW:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u %s %.4lf %.4lf %.4lf %.4lf %.4lf\n", line++,
					pDraw->type == ARC_CW ? "arc_cw" :
					  pDraw->type == ARC_CCW ? "arc_cww" :
					   pDraw->type == ARCREL_CW ? "arcrel_cw" :
					    pDraw->type == ARCREL_CCW ? "arcrel_cww" : "unknown",
					pDraw->x, pDraw->y, pDraw->radius,
					pDraw->angle_from, pDraw->angle_to);
				break;
			case MASK_PATTERN:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u mask pattern %u\n", line++,
					pDraw->parameter);
				break;
			case SOURCE_PATTERN:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u source pattern %u\n", line++,
					pDraw->parameter);
				break;
			case ALPHAADD:
			case ALPHAMUL:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u source %s %.4lf\n", line++,
					pDraw->type == ALPHAADD ? "alphaadd" : "alphamul",
					pDraw->alpha);
				break;
			case SOURCE_RGB:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u source rgb %.4lf %.4lf %.4lf\n", line++,
					pDraw->red, pDraw->green, pDraw->blue);
				break;
			case SOURCE_RGBA:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u source rgba %.4lf %.4lf %.4lf %.4lf\n", line++,
					pDraw->red, pDraw->green, pDraw->blue, pDraw->alpha);
				break;
			case LINEWIDTH:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u width %.4lf\n", line++, pDraw->width);
				break;
			case LINEJOIN:
				if (pDraw->x < 0.0)
					m_pVideoMixer->m_pController->controller_write_msg (ctr,
						STATUS" - %u line join miter %.4lf\n",
						line++, pDraw->y);
				else m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u line join %s\n",
					line++, pDraw->x == 0.0 ? "round" : "bevel");
				break;
			case LINECAP:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u line cap %s\n", line++,
						pDraw->width < 0.0 ? "butt" :
						  pDraw->width == 0.0 ? "round" : "square");
				break;
			case SHAPE:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u inshape %u\n", line++,
					pDraw->parameter);
				break;
			case CURVETO:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u curveto %.4lf,%.4lf %.4lf,%.4lf "
					"%.4lf,%.4lf\n", line++,
					pDraw->x, pDraw->y, pDraw->radius, pDraw->width,
					pDraw->angle_from, pDraw->angle_to);
					// x2=raius, y2=width
					// x3=angle_from y3=angle_to
				break;
			case CURVEREL:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u curverel %.4lf,%.4lf %.4lf,%.4lf "
					"%.4lf,%.4lf\n", line++,
					pDraw->x, pDraw->y, pDraw->radius, pDraw->width,
					pDraw->angle_from, pDraw->angle_to);
					// x2=raius, y2=width
					// x3=angle_from y3=angle_to
				break;
			case RECTANGLE:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u rectangle %.4lf %.4lf %.4lf %.4lf\n", line++,
					pDraw->x, pDraw->y, pDraw->angle_from, pDraw->angle_to);
					// angle_from and angle_to are width and height
				break;
			case TRANSFORM:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u transform %.4lf %.4lf %.4lf %.4lf %.4lf %.4lf\n", line++,
					pDraw->x, pDraw->y, pDraw->radius, pDraw->width,
					pDraw->angle_from, pDraw->angle_to);
				break;
			case FILTER:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u filter %s\n", line++,
					CairoFilter2String(pDraw->parameter));
				break;
			case OPERATOR:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u operator %s\n", line++,
					pDraw->parameter == CAIRO_OPERATOR_CLEAR ? "clear" :
					pDraw->parameter == CAIRO_OPERATOR_SOURCE ? "source" :
					pDraw->parameter == CAIRO_OPERATOR_OVER ? "over" :
					pDraw->parameter == CAIRO_OPERATOR_IN ? "in" :
					pDraw->parameter == CAIRO_OPERATOR_OUT ? "out" :
					pDraw->parameter == CAIRO_OPERATOR_ATOP ? "atop" :
					pDraw->parameter == CAIRO_OPERATOR_DEST ? "dest" :
					pDraw->parameter == CAIRO_OPERATOR_DEST_OVER ? "dest_over" :
					pDraw->parameter == CAIRO_OPERATOR_DEST_IN ? "dest_in" :
					pDraw->parameter == CAIRO_OPERATOR_DEST_OUT ? "dest_out" :
					pDraw->parameter == CAIRO_OPERATOR_DEST_ATOP ? "dest_atop" :
					pDraw->parameter == CAIRO_OPERATOR_XOR ? "xor" :
					pDraw->parameter == CAIRO_OPERATOR_ADD ? "add" :
					pDraw->parameter == CAIRO_OPERATOR_SATURATE ? "saturate" :
					pDraw->parameter == CAIRO_OPERATOR_MULTIPLY ? "multiply" :
					pDraw->parameter == CAIRO_OPERATOR_SCREEN ? "screen" :
					pDraw->parameter == CAIRO_OPERATOR_OVERLAY ? "overlay" :
					pDraw->parameter == CAIRO_OPERATOR_DARKEN ? "darken" :
					pDraw->parameter == CAIRO_OPERATOR_LIGHTEN ? "lighten" :
					pDraw->parameter == CAIRO_OPERATOR_COLOR_DODGE ? "dodge" :
					pDraw->parameter == CAIRO_OPERATOR_COLOR_BURN ? "burn" :
					pDraw->parameter == CAIRO_OPERATOR_HARD_LIGHT ? "hard" :
					pDraw->parameter == CAIRO_OPERATOR_SOFT_LIGHT ? "soft" :
					pDraw->parameter == CAIRO_OPERATOR_DIFFERENCE ? "difference" :
					pDraw->parameter == CAIRO_OPERATOR_EXCLUSION ? "exclusion" :
					pDraw->parameter == CAIRO_OPERATOR_HSL_HUE ? "hsl_hue" :
					pDraw->parameter == CAIRO_OPERATOR_HSL_SATURATION ? "hsl_saturation" :
					pDraw->parameter == CAIRO_OPERATOR_HSL_COLOR ? "hsl_color" :
					pDraw->parameter == CAIRO_OPERATOR_HSL_LUMINOSITY ? "hsl_luminosity" : "unknown");
				break;
			case IMAGE:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u image %u %.4lf,%.4lf scale %.4lf,%.4lf\n", line++,
					pDraw->parameter, pDraw->x, pDraw->y,
					pDraw->width, pDraw->radius);
				break;
			case FEED:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u feed %u at %.4lf,%.4lf scale %.4lf,%.4lf\n", line++,
					pDraw->parameter, pDraw->x, pDraw->y,
					pDraw->width, pDraw->radius);
				break;
			default:
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					STATUS" - %u unknown\n", line++);
				break;
		}

		pDraw = pDraw->next;
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}

int CVideoShape::set_shape_xy(struct controller_type* ctr, const char* str, draw_operation_enum_t type)
{
	u_int32_t shape_id;
	double x, y;
	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (sscanf(str, "%u %lf %lf", &shape_id, &x, &y) != 3) return -1;
	int n = AddToShapeXY(shape_id, type, x, y);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u %s %.3lf,%.3lf\n", shape_id,
			type == MOVEREL ? "moverel" :
			  type == MOVETO ? "moveto" :
			    type == SCALE ? "scale" :
			      type == TRANSLATE ? "translate" :
			        type == LINETO ? "lineto" :
			          type == LINEREL ? "linerel" : "unknown", x, y);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_transform(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id;
	double xx, yx, xy, yy, x0, y0;
	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u %lf %lf %lf %lf %lf %lf", &shape_id, &xx, &yx, &xy, &yy, &x0, &y0);
	if (n != 7) return -1;

	n = AddToShapeTransform(shape_id, xx, yx, xy, yy, x0, y0);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u transform %.4lf %.4lf %.4lf %.4lf %.4lf %.4lf\n",
			shape_id, xx, yx, xy, yy, x0, y0);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_filter(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id;
	cairo_filter_t filter;
	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u", &shape_id);
	if (!n) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
        if (!strncasecmp (str, "fast ", 5)) filter=CAIRO_FILTER_FAST;
        else if (!strncasecmp (str, "good ", 5)) filter=CAIRO_FILTER_GOOD;
        else if (!strncasecmp (str, "best ", 5)) filter=CAIRO_FILTER_BEST;
        else if (!strncasecmp (str, "nearest ", 8)) filter=CAIRO_FILTER_NEAREST;
        else if (!strncasecmp (str, "bilinear ", 9)) filter=CAIRO_FILTER_BILINEAR;
        else if (!strncasecmp (str, "gaussian ", 9)) filter=CAIRO_FILTER_GAUSSIAN;
        else return -1;

	n = AddToShapeFilter(shape_id, filter);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u filter %s\n",
			shape_id, str);
	}

	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}

	return n;
}

int CVideoShape::set_shape_operator(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id;
	cairo_operator_t c_operator;
	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u", &shape_id);
	if (!n) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	if (!strncasecmp (str, "clear ", 6))           c_operator = CAIRO_OPERATOR_CLEAR;
	else if (!strncasecmp (str, "source ", 7))     c_operator = CAIRO_OPERATOR_SOURCE;
	else if (!strncasecmp (str, "over ", 5))       c_operator = CAIRO_OPERATOR_OVER;
	else if (!strncasecmp (str, "in ", 3))         c_operator = CAIRO_OPERATOR_IN;
	else if (!strncasecmp (str, "out ", 4))        c_operator = CAIRO_OPERATOR_OUT;
	else if (!strncasecmp (str, "atop ", 5))       c_operator = CAIRO_OPERATOR_ATOP;
	else if (!strncasecmp (str, "dest ", 5))       c_operator = CAIRO_OPERATOR_DEST;
	else if (!strncasecmp (str, "dest_over ", 10)) c_operator = CAIRO_OPERATOR_DEST_OVER;
	else if (!strncasecmp (str, "dest_in ", 8))    c_operator = CAIRO_OPERATOR_DEST_IN;
	else if (!strncasecmp (str, "dest_out ", 9))   c_operator = CAIRO_OPERATOR_DEST_OUT;
	else if (!strncasecmp (str, "dest_atop ", 10)) c_operator = CAIRO_OPERATOR_DEST_ATOP;
	else if (!strncasecmp (str, "xor ", 5))        c_operator = CAIRO_OPERATOR_XOR;
	else if (!strncasecmp (str, "add ", 5))        c_operator = CAIRO_OPERATOR_ADD;
	else if (!strncasecmp (str, "saturate ", 5))   c_operator = CAIRO_OPERATOR_SATURATE;
	else if (!strncasecmp (str, "multiply ", 5))   c_operator = CAIRO_OPERATOR_MULTIPLY;
	else if (!strncasecmp (str, "screen ", 5))     c_operator = CAIRO_OPERATOR_SCREEN;
	else if (!strncasecmp (str, "overlay ", 5))    c_operator = CAIRO_OPERATOR_OVERLAY;
	else if (!strncasecmp (str, "darken ", 5))     c_operator = CAIRO_OPERATOR_DARKEN;
	else if (!strncasecmp (str, "lighten ", 5))    c_operator = CAIRO_OPERATOR_LIGHTEN;
	else if (!strncasecmp (str, "dodge ", 5))      c_operator = CAIRO_OPERATOR_COLOR_DODGE;
	else if (!strncasecmp (str, "burn ", 5))       c_operator = CAIRO_OPERATOR_COLOR_BURN;
	else if (!strncasecmp (str, "hard ", 5))       c_operator = CAIRO_OPERATOR_HARD_LIGHT;
	else if (!strncasecmp (str, "soft ", 5))       c_operator = CAIRO_OPERATOR_SOFT_LIGHT;
	else if (!strncasecmp (str, "difference ", 5)) c_operator = CAIRO_OPERATOR_DIFFERENCE;
	else if (!strncasecmp (str, "exclusion ", 5))  c_operator = CAIRO_OPERATOR_EXCLUSION;
	else if (!strncasecmp (str, "hsl_hue ", 5))    c_operator = CAIRO_OPERATOR_HSL_HUE;
	else if (!strncasecmp (str, "hsl_saturation ", 5)) c_operator = CAIRO_OPERATOR_HSL_SATURATION;
	else if (!strncasecmp (str, "hsl_color ", 5))  c_operator = CAIRO_OPERATOR_HSL_COLOR;
	else if (!strncasecmp (str, "hsl_luminosity ", 5)) c_operator = CAIRO_OPERATOR_HSL_LUMINOSITY;
	else return -1;

	n = AddToShapeOperator(shape_id, c_operator);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u operator %s\n",
			shape_id, str);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_image(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id, image_id;
	double x, y, scale_x, scale_y;
	const char* create_str = str;
	scale_x = -1.0;
	scale_y = -1.0;
	if (!m_pVideoMixer || !m_pVideoMixer->m_pVideoImage) return 1;
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u %u %lf %lf %lf %lf", &shape_id, &image_id, &x, &y, &scale_x, &scale_y);
	if (n != 4 && n != 6) return -1;
	if (!m_pVideoMixer->m_pVideoImage->GetImageItem(image_id)) return -1;
	n = AddToShapeImage(shape_id, image_id, x, y, scale_x, scale_y);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u image %u at %.3lf,%.3lf scale %.4lf,%.4lf\n",
			shape_id, image_id, x, y, scale_x, scale_y);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_feed(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id, feed_id;
	double x, y, scale_x, scale_y;
	const char* create_str = str;
	scale_x = -1.0;
	scale_y = -1.0;
	if (!m_pVideoMixer || !m_pVideoMixer->m_pVideoFeed) return 1;
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u %u %lf %lf %lf %lf", &shape_id, &feed_id, &x, &y, &scale_x, &scale_y);
	if (n != 4 && n != 6) return -1;

	// Just checking the feed exist, otherwise complain
	if (!m_pVideoMixer->m_pVideoFeed->FindFeed(feed_id)) return -1;

	n = AddToShapeFeed(shape_id, feed_id, x, y, scale_x, scale_y);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u feed %u at %.3lf,%.3lf scale %.4lf,%.4lf\n",
			shape_id, feed_id, x, y, scale_x, scale_y);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_arc(struct controller_type* ctr, const char* str, draw_operation_enum_t type)
{
	u_int32_t shape_id;
	double x, y, radius, angle_from, angle_to;
	const char* create_str = str;
	if (!str) return -1;
	// the type will be checked in AddToShapeArc

	while (isspace(*str)) str++;
	if (sscanf(str, "%u %lf %lf %lf", &shape_id, &x, &y, &radius) != 4)
		return -1;
	while (!isspace(*str)) str++;		// shape_id
	while (isspace(*str)) str++;
	while (!isspace(*str)) str++;		// x
	while (isspace(*str)) str++;
	while (!isspace(*str)) str++;		// y
	while (isspace(*str)) str++;
	while (*str && !isspace(*str)) str++;		// radius
	if (!isspace(*str)) return -1;
	while (isspace(*str)) str++;
	if (!NumberWithPI(str, &angle_from)) return -1;
	while (*str && !isspace(*str)) str++;
	if (!NumberWithPI(str, &angle_to)) return -1;
	int n = AddToShapeArc(shape_id, type, x, y, radius, angle_from, angle_to);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u %s at %.3lf,%.3lf radius %.3lf angle from %.4lf to %.4lf\n",
			shape_id,
			type == ARC_CW ? "arc_cw" :
			  type == ARC_CCW ? "arc_cw" :
			    type == ARCREL_CW ? "arcrel_cw" :
			      type == ARCREL_CCW ? "arcrel_ccw" : "unknown",
			x, y, radius, angle_from, angle_to);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_curve(struct controller_type* ctr, const char* str, draw_operation_enum_t type)
{
	u_int32_t shape_id;
	double x1, y1, x2, y2, x3, y3;
	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (sscanf(str, "%u %lf %lf %lf %lf %lf %lf", &shape_id, &x1, &y1, &x2, &y2, &x3, &y3) != 7)
		return -1;
	int n = AddToShapeCurve(shape_id, type, x1, y1, x2, y2, x3, y3);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u %s point1 %.4lf,%.4lf point2 %.4lf,%.4lf point3 %.4lf,%.4lf\n",
			shape_id,
			type == CURVETO ? "curve_to" :
			  type == CURVEREL ? "curve_rel" : "unknown",
			x1, y1, x2, y2, x3, y3);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_no_parameter(struct controller_type* ctr, const char* str, draw_operation_enum_t type)
{
	u_int32_t shape_id;
	if (!str) return -1;
	const char* create_str = str;
	while (isspace(*str)) str++;
	if (sscanf(str, "%u", &shape_id) != 1) return -1;
	int n = AddToShapeCommand(shape_id, type);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u %s\n",
			shape_id,
			type == CLOSEPATH ? "closepath" :
			  type == NEWPATH ? "newpath" :
			    type == NEWSUBPATH ? "newsubpath" :
			      type == CLIP ? "clip" :
			        type == STROKE ? "stroke" :
			          type == SCALETELL ? "tell scale" :
			            type == STROKEPRESERVE ? "stroke preserve" :
			              type == FILLPRESERVE ? "fill preserve" :
			                type == SAVE ? "save" :
			                  type == RESTORE ? "restore" :
			                    type == FILL ? "fill" : "unknown");
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_line_join(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id;
	double join, miter = 10.0;
	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u", &shape_id);
	if (n != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!strncasecmp (str, "miter ", 6)) join = -1.0;
	else if (!strncasecmp (str, "round ", 6)) join = 0.0;
	else if (!strncasecmp (str, "bevel ", 6)) join = 1.0;
	else return -1;
	while (*str && !isspace(*str)) str++;
	while (isspace(*str)) str++;
	if (join < 0.0 && *str) {
		n = sscanf(str, "%lf", &miter);
		if (n != 1) return -1;
	}
	n = AddToShapeLineJoin(shape_id, join, miter);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		if (join < 0.0) m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u line join miter %.4lf\n", shape_id, miter);
		else m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u line join %s\n", shape_id,
				join == 0.0 ? "round" : "bevel");
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_one_parameter(struct controller_type* ctr, const char* str, draw_operation_enum_t type)
{
	u_int32_t shape_id;
	double parameter = -1.0; // for PAINT without parameter
	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u %lf", &shape_id, &parameter);
	if ((n == 2 && (type == LINEWIDTH || type == SHAPE || type == RECURSION)) || type == ALPHAADD || type == ALPHAMUL ||
        	((type == PAINT || type == LINECAP) && (n == 1 || n == 2))) {
		if (n == 1) {
			parameter = 1.0;
			if (type == LINECAP) {
				while (isdigit(*str)) str++;
				while (isspace(*str)) str++;
				if (!strncasecmp (str, "butt", 4)) parameter = -1.0;
				else if (!strncasecmp (str, "round", 5)) parameter = 0.0;
				else if (!strncasecmp (str, "square", 6)) parameter = 1.0;
				else {
					return -1;
				}
			}
		}
		n = AddToShapeWithParameter(shape_id, type, parameter);
	}
	else n = -1;
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		if (type == LINECAP) m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u line cap %s\n",
			shape_id,
			parameter < 0.0 ? "butt" :
			  parameter == 0.0 ? "round" : "square");
		else if (type == PAINT || type == SHAPE || type == RECURSION)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u %s %u\n",
			shape_id,
			type == PAINT ? "paint" :
			  type == SHAPE ? "inshape" :
			    type == RECURSION ? "recursion" : "unknown",
			(unsigned int)(parameter+ 0.5));
		else m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u %s %.4lf\n",
			shape_id,
		 	type == LINEWIDTH ? "line width" :
		 	  type == ALPHAADD ? "alphadd" :
		 	    type == ALPHAMUL ? "alphamul" : "unknown", parameter);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_source_pattern(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id, pattern_id;
	if (!str) return -1;
	const char* create_str = str;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u %u", &shape_id, &pattern_id);
	if (n != 2) return -1;
	n = AddToShapeSourcePattern(shape_id, pattern_id);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u source pattern %u\n",
			shape_id, pattern_id);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_mask_pattern(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id, pattern_id;
	if (!str) return -1;
	const char* create_str = str;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u %u", &shape_id, &pattern_id);
	if (n != 2) return -1;
	n = AddToShapeMaskPattern(shape_id, pattern_id);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u mask pattern %u\n",
			shape_id, pattern_id);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_source_rgb(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id;
	double red, green, blue, alpha;
	const char* create_str = str;
	alpha = -1.0;
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u %lf %lf %lf %lf", &shape_id, &red, &green, &blue, &alpha);
	if (n == 4) n = AddToShapeSourceRGBA(shape_id, red, green, blue, -1.0);
	else if (n == 5 && alpha >= 0.0) n = AddToShapeSourceRGBA(shape_id, red, green, blue, alpha);
	else n = -1;
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		if (alpha >= 0.0)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"shape %u source rgba %.3lf,%.3lf,%.3lf,%.3lf\n",
				shape_id, red, green, blue, alpha);
		else
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				MESSAGE"shape %u source rgb %.3lf,%.3lf,%.3lf\n",
				shape_id, red, green, blue);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_rectangle(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id;
	double x, y, width, height;
	const char* create_str = str;
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u %lf %lf %lf %lf", &shape_id, &x, &y, &width, &height);
	if (n == 5) n = AddToShapeRectangle(shape_id, x, y, width, height);
	else n = -1;
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u rectangle %.3lf,%.3lf,%.3lf,%.3lf\n",
			shape_id, x, y, width, height);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::set_shape_rotation(struct controller_type* ctr, const char* str)
{
	u_int32_t shape_id;
	double rotation, absolute_rotation;
	bool absolute = true;
	const char* create_str = str;
	
	if (!str) return -1;
	while (isspace(*str)) str++;
	int n = sscanf(str, "%u", &shape_id);
	if (n != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!NumberWithPI(str, &rotation)) return -1;
	while (*str && !isspace(*str)) str++;
	if (!NumberWithPI(str, &absolute_rotation)) absolute = false;
	n = AddToShapeRotation(shape_id, rotation, absolute_rotation, absolute);
	if (m_verbose && !n && m_pVideoMixer && m_pVideoMixer->m_pController) {
		if (absolute) m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u rotation %.4lf %.4lf\n",
			shape_id, rotation, absolute_rotation);
		else m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape %u rotation %.4lf\n",
			shape_id, rotation);
	}
	if (!n) {
		if (m_shapes[shape_id]->pDrawTail && m_shapes[shape_id]->pDrawTail->create_str)
			free(m_shapes[shape_id]->pDrawTail->create_str);
		m_shapes[shape_id]->pDrawTail->create_str = DuplicateCommandString(create_str);
	}
	return n;
}

int CVideoShape::ShapeOps(u_int32_t shape_id)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return -1;
	return m_shapes[shape_id]->ops_count;
}

int CVideoShape::ShapeMoveEntry(u_int32_t shape_id, u_int32_t entry_id, u_int32_t to_entry)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id] ||
		!m_shapes[shape_id]->pDraw || entry_id < 1) return 1;
	draw_operation_t* p = m_shapes[shape_id]->pDraw;
	draw_operation_t* pInsert = NULL;
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

int CVideoShape::AddToShapeCommand(u_int32_t shape_id, draw_operation_enum_t type)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	if (type != STROKE && type != STROKEPRESERVE && type != CLIP && type != FILL && type != FILLPRESERVE && type != CLOSEPATH && type != NEWPATH && type != NEWSUBPATH && type != SAVE && type != RESTORE && type != SCALETELL) return -1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = type;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;		// No memory leak. Allocated memory is freed when shape is
				// deleted. Deconstructor deletes all shapes
}

int CVideoShape::AddToShapeLineJoin(u_int32_t shape_id, double join, double miter)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = LINEJOIN;
	p->x = join;
	p->y = miter;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++; // No memory leak. Allocated memory is freed when shape is
	return 0;			 // deleted. Deconstructor deletes all shapes
}

int CVideoShape::AddToShapeWithParameter(u_int32_t shape_id, draw_operation_enum_t type, double parameter)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	if (type != LINEWIDTH && type != PAINT && type != SHAPE &&
		type != RECURSION && type != LINECAP && type != ALPHAMUL && type != ALPHAADD)
		return -1;
	if (type == SHAPE && !GetShape((u_int32_t)(0.5+parameter))) return -1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = type;

	if (type == SHAPE) p->parameter = (typeof(p->parameter))(parameter + 0.5);
	else if (type == LINEWIDTH) p->width = parameter;
	else if (type == PAINT) p->alpha = parameter;
	else if (type == LINECAP) p->width = parameter;
#ifdef WIN32
	else if (type == RECURSION) p->parameter = (decltype(p->parameter))(parameter + 0.5);
#else
	else if (type == RECURSION) p->parameter = (typeof(p->parameter))(parameter + 0.5);
#endif
	else if (type == ALPHAADD) p->alpha = parameter;
	else if (type == ALPHAMUL) p->alpha = parameter;
	else {
		free(p);
		return -1;
	}
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;		// No memory leak. Allocated memory is freed when shape is
                                // deleted. Deconstructor deletes all shapes

}

int CVideoShape::AddToShapeTransform(u_int32_t shape_id, double xx, double yx, double xy, double yy, double x0, double y0)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	cairo_matrix_t* m = (cairo_matrix_t*) &(p->x);
	p->type = TRANSFORM;
	p->x = xx;
	p->y = yx;
	p->radius = xy;
	p->width = yy;
	p->angle_from = x0;
	p->angle_to = y0;
	// check that the cairo_matrix_t maps into draw_operation. 
	if (m->xx != xx || m->yx != yx || m->xy != xy || m->yy != yy || m->x0 != x0 || m->y0 != y0) {
		free(p);
		return -1;
	}
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;	// No memory leak. Allocated memory is freed when shape is
                        // deleted. Deconstructor deletes all shapes

}

int CVideoShape::AddToShapeFilter(u_int32_t shape_id, cairo_filter_t filter)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = FILTER;
	p->parameter = filter;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;
}

int CVideoShape::AddToShapeOperator(u_int32_t shape_id, cairo_operator_t c_operator)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = OPERATOR;
	p->parameter = c_operator;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;
}

int CVideoShape::AddToShapeXY(u_int32_t shape_id, draw_operation_enum_t type, double x, double y)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	if (type != MOVETO && type != MOVEREL && type != LINETO &&
		type != LINEREL && type != SCALE && type != TRANSLATE) return -1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = type;
	p->x = x;
	p->y = y;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;	// No memory leak. Allocated memory is freed when shape is
                        // deleted. Deconstructor deletes all shapes

}

int CVideoShape::AddToShapeImage(u_int32_t shape_id, u_int32_t image_id, double x, double y, double scale_x, double scale_y)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	if ((scale_x > 0.0 && scale_y < 0.0) || (scale_x == 0.0 && scale_y != 0.0) ||
		(scale_x != 0.0 && scale_y == 0.0)) return -1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = IMAGE;
	p->x = x;
	p->y = y;
	p->width = scale_x;	// scale_x saved in width
	p->radius = scale_y;	// scale_y saved in raius
	p->parameter = image_id;	// save image id in parameter
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;	// No memory leak. Allocated memory is freed when shape is
                        // deleted. Deconstructor deletes all shapes

}

int CVideoShape::AddToShapeFeed(u_int32_t shape_id, u_int32_t feed_id, double x, double y, double scale_x, double scale_y)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	if ((scale_x > 0.0 && scale_y < 0.0) || (scale_x == 0.0 && scale_y != 0.0) ||
		(scale_x != 0.0 && scale_y == 0.0)) return -1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = FEED;
	p->x = x;
	p->y = y;
	p->width = scale_x;	// scale_x saved in width
	p->radius = scale_y;	// scale_y saved in raius
	p->parameter = feed_id;	// save feed id in parameter
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;
}

int CVideoShape::AddToShapeArc(u_int32_t shape_id, draw_operation_enum_t type, double x, double y, double radius, double angle_from, double angle_to)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	if (radius <= 0.0 || (type != ARC_CW && type != ARC_CCW && type != ARCREL_CW && type != ARCREL_CCW)) return -1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = type;
	p->x = x;
	p->y = y;
	p->radius = radius;
	p->angle_from = angle_from;
	p->angle_to = angle_to;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;	// No memory leak. Allocated memory is freed when shape is
                        // deleted. Deconstructor deletes all shapes

}

int CVideoShape::AddToShapeCurve(u_int32_t shape_id, draw_operation_enum_t type, double x1, double y1, double x2, double y2, double x3, double y3)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	if (type != CURVETO && type != CURVEREL) return -1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = type;
	p->x = x1;
	p->y = y1;
	p->radius = x2;
	p->width = y2;
	p->angle_from = x3;
	p->angle_to = y3;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;	// No memory leak. Allocated memory is freed when shape is
                        // deleted. Deconstructor deletes all shapes
}


int CVideoShape::AddToShapeSourcePattern(u_int32_t shape_id, u_int32_t pattern_id)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id] ||
		!m_patterns || pattern_id >= m_max_patterns || !m_patterns[pattern_id])
		return 1;
	if (!GetCairoPattern(pattern_id)) return -1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->parameter = pattern_id;
	p->type = SOURCE_PATTERN;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;
}

int CVideoShape::AddToShapeMaskPattern(u_int32_t shape_id, u_int32_t pattern_id)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id] ||
		!m_patterns || pattern_id >= m_max_patterns || !m_patterns[pattern_id])
		return 1;
	if (!GetCairoPattern(pattern_id)) return -1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->parameter = pattern_id;
	p->type = MASK_PATTERN;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;
}

int CVideoShape::AddToShapeSourceRGBA(u_int32_t shape_id, double red, double green, double blue, double alpha)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = alpha < 0.0 ? SOURCE_RGB : SOURCE_RGBA;
	p->red = red;
	p->green = green;
	p->blue = blue;
	p->alpha = alpha;
	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;	// No memory leak. Allocated memory is freed when shape is
                        // deleted. Deconstructor deletes all shapes
}

int CVideoShape::AddToShapeRectangle(u_int32_t shape_id, double x, double y, double width, double height)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = RECTANGLE;
	p->x = x;
	p->y = y;
	//p->red = x;
	//p->green = y;
	p->angle_from = width;	// angle_from contains the width
	p->angle_to = height;	// angle_from contains the height

	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;	// No memory leak. Allocated memory is freed when shape is
                        // deleted. Deconstructor deletes all shapes
}

int CVideoShape::AddToShapeRotation(u_int32_t shape_id, double rotation, double absolute_rotation, bool absolute)
{
	if (!m_shapes || shape_id >= m_max_shapes || !m_shapes[shape_id]) return 1;
	draw_operation_t* p = (draw_operation_t*) calloc(sizeof(draw_operation_t),1);
	if (!p) return 1;
	p->type = ROTATION;
	p->parameter = absolute;
	p->angle_from = rotation;
	if (absolute) p->angle_to = absolute_rotation;

	if (m_shapes[shape_id]->pDrawTail) m_shapes[shape_id]->pDrawTail->next = p;
	m_shapes[shape_id]->pDrawTail = p;
	if (!m_shapes[shape_id]->pDraw) m_shapes[shape_id]->pDraw = p;
	m_shapes[shape_id]->ops_count++;
	return 0;
}

int CVideoShape::AddShape(u_int32_t shape_id, const char* name)
{
	if (!name) return -1;
	if (!m_shapes || shape_id >= m_max_shapes) return 1;
	if (m_shapes[shape_id]) DeleteShape(shape_id);
	shape_t* pShape = m_shapes[shape_id] =
		(shape_t*) calloc(sizeof(shape_t)+strlen(name)+1,1);
	if (!pShape) return 1;
	pShape->name = ((char*)pShape)+sizeof(shape_t);
	strcpy(pShape->name,name);
	trim_string(pShape->name);
	pShape->id = shape_id;
	m_shape_count++;
	return 0;
}

int CVideoShape::PatternCreateRadial(u_int32_t pattern_id, double cx0, double cy0, double radius0, double cx1, double cy1, double radius1)
{
	if (!m_patterns || pattern_id >= m_max_patterns || !m_patterns[pattern_id] ||
		radius0 < 0.0 || radius1 < 0.0 || (radius0 == 0.0 && radius1 == 0.0))
		return -1;
	if (m_patterns[pattern_id]->pattern)
		cairo_pattern_destroy(m_patterns[pattern_id]->pattern);
	m_patterns[pattern_id]->pattern =
		cairo_pattern_create_radial(cx0, cy0, radius0, cx1, cy1, radius1);
	m_patterns[pattern_id]->x1 = cx0;
	m_patterns[pattern_id]->y1 = cy0;
	m_patterns[pattern_id]->radius0 = radius0;
	m_patterns[pattern_id]->x2 = cx1;
	m_patterns[pattern_id]->y2 = cy1;
	m_patterns[pattern_id]->radius1 = radius1;
	m_patterns[pattern_id]->type = RADIAL_PATTERN;
	
	return (m_patterns[pattern_id]->pattern ? 0 : -1);
}

int CVideoShape::PatternStopRGBA(u_int32_t pattern_id, double offset, double red, double green, double blue, double alpha)
{
	if (!m_patterns || pattern_id >= m_max_patterns || !m_patterns[pattern_id] ||
		!m_patterns[pattern_id]->pattern ||
		offset < 0.0 || offset > 1.0 || red < 0.0 || red > 1.0 ||
		green < 0.0 || green > 1.0 || blue < 0.0 || blue > 1.0 || alpha > 1.0)
		return -1;
	if (alpha < 0.0)
		cairo_pattern_add_color_stop_rgb(m_patterns[pattern_id]->pattern,
			offset, red, green, blue);
	else cairo_pattern_add_color_stop_rgba(m_patterns[pattern_id]->pattern,
			offset, red, green, blue, alpha);
	return 0;
}

int CVideoShape::PatternCreateLinear(u_int32_t pattern_id, double x1, double y1, double x2, double y2)
{
	if (!m_patterns || pattern_id >= m_max_patterns || !m_patterns[pattern_id] ||
		(x1 == x2 && y1 == y2)) return -1;
	if (m_patterns[pattern_id]->pattern)
		cairo_pattern_destroy(m_patterns[pattern_id]->pattern);
	m_patterns[pattern_id]->pattern = cairo_pattern_create_linear(x1, y1, x2, y2);
	m_patterns[pattern_id]->type = LINEAR_PATTERN;
	return (m_patterns[pattern_id]->pattern ? 0 : -1);
	
	return 0;
}

int CVideoShape::AddPattern(u_int32_t pattern_id, const char* name)
{
	if (!name) return -1;
	if (!m_patterns || pattern_id >= m_max_patterns) return 1;
	if (m_patterns[pattern_id]) DeletePattern(pattern_id);
	pattern_t* pPattern = m_patterns[pattern_id] =
		(pattern_t*) calloc(sizeof(pattern_t)+strlen(name)+1,1);
	if (!pPattern) return 1;
	pPattern->name = ((char*)pPattern)+sizeof(pattern_t);
	strcpy(pPattern->name,name);
	trim_string(pPattern->name);
	pPattern->id		= pattern_id;
	pPattern->type		= NO_PATTERN;
	pPattern->x1		= 0.0;
	pPattern->y1		= 0.0;
	pPattern->x2		= 0.0;
	pPattern->y2		= 0.0;
	pPattern->radius0	= 0.0;
	pPattern->radius1	= 0.0;
	m_pattern_count++;
	return 0;
}

int CVideoShape::SetPlacedShapeCoor(u_int32_t place_id, double x, double y, const char* anchor)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id])
		return -1;
	m_placed_shapes[place_id]->x = x;
	m_placed_shapes[place_id]->y = y;
	if (anchor && *anchor) return SetShapeAnchor(place_id, anchor);
	return 0;
}

int CVideoShape::SetShapeAnchor(u_int32_t place_id, const char* anchor)
{
	if (!m_pVideoMixer || !m_placed_shapes ||
		place_id >= m_max_shape_places || !m_placed_shapes[place_id]) return -1;
	//placed_shape_t* pShape = m_placed_shapes[place_id];

	return SetAnchor(&(m_placed_shapes[place_id]->anchor_x),
                &(m_placed_shapes[place_id]->anchor_y),
                anchor, m_pVideoMixer->GetSystemWidth(), m_pVideoMixer->GetSystemHeight());

/*
	if (strcasecmp("ne", anchor) == 0) {
		pShape->anchor_x = m_pVideoMixer->GetSystemWidth();
		pShape->anchor_y = 0;
	} else if (strcasecmp("se", anchor) == 0) {
		pShape->anchor_x = m_pVideoMixer->GetSystemWidth();
		pShape->anchor_y = m_pVideoMixer->GetSystemHeight();
	} else if (strcasecmp("sw", anchor) == 0) {
		pShape->anchor_x = 0;
		pShape->anchor_y = m_pVideoMixer->GetSystemHeight();
	} else if (strcasecmp("n", anchor) == 0) {
		pShape->anchor_x = m_pVideoMixer->GetSystemWidth()>>1;
		pShape->anchor_y = 0;
	} else if (strcasecmp("w", anchor) == 0) {
		pShape->anchor_x = m_pVideoMixer->GetSystemWidth();
		pShape->anchor_y = m_pVideoMixer->GetSystemHeight()>>1;
	} else if (strcasecmp("s", anchor) == 0) {
		pShape->anchor_x = m_pVideoMixer->GetSystemWidth()>>1;
		pShape->anchor_y = m_pVideoMixer->GetSystemHeight();
	} else if (strcasecmp("e", anchor) == 0) {
		pShape->anchor_x = m_pVideoMixer->GetSystemWidth();
		pShape->anchor_y = m_pVideoMixer->GetSystemHeight()>>1;
	} else {
		pShape->anchor_x = 0;
		pShape->anchor_y = 0;
	}
	return 0;
*/
}

int CVideoShape::SetPlacedShapeMoveCoor(u_int32_t place_id, double delta_x, double delta_y, u_int32_t steps_x, u_int32_t steps_y)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id])
		return -1;
	m_placed_shapes[place_id]->delta_x = delta_x;
	m_placed_shapes[place_id]->delta_y = delta_y;
	m_placed_shapes[place_id]->steps_x = steps_x;
	m_placed_shapes[place_id]->steps_y = steps_y;
	if (steps_x || steps_y) m_update = m_placed_shapes[place_id]->update = true;
	return 0;
}

int CVideoShape::SetPlacedShapeMoveOffset(u_int32_t place_id, double delta_x, double delta_y, u_int32_t steps_x, u_int32_t steps_y)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id])
		return -1;
	m_placed_shapes[place_id]->delta_offset_x = delta_x;
	m_placed_shapes[place_id]->delta_offset_y = delta_y;
	m_placed_shapes[place_id]->steps_offset_x = steps_x;
	m_placed_shapes[place_id]->steps_offset_y = steps_y;
	if (steps_x || steps_y) m_update = m_placed_shapes[place_id]->update = true;
	return 0;
}

int CVideoShape::SetPlacedShapeMoveScale(u_int32_t place_id, double delta_x, double delta_y, u_int32_t steps_x, u_int32_t steps_y)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id])
		return -1;
	m_placed_shapes[place_id]->delta_scale_x = delta_x;
	m_placed_shapes[place_id]->delta_scale_y = delta_y;
	m_placed_shapes[place_id]->steps_scale_x = steps_x;
	m_placed_shapes[place_id]->steps_scale_y = steps_y;
	if (steps_x || steps_y) m_update = m_placed_shapes[place_id]->update = true;
	return 0;
}

int CVideoShape::SetPlacedShapeMoveAlpha(u_int32_t place_id, double delta_alpha, u_int32_t steps_alpha)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id])
		return -1;
	m_placed_shapes[place_id]->delta_alpha = delta_alpha;
	m_placed_shapes[place_id]->steps_alpha = steps_alpha;
	if (steps_alpha) m_update = m_placed_shapes[place_id]->update = true;
	return 0;
}

int CVideoShape::SetPlacedShapeMoveRotation(u_int32_t place_id, double delta_rotation, u_int32_t steps_rotation)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id])
		return -1;
	m_placed_shapes[place_id]->delta_rotation = delta_rotation;
	m_placed_shapes[place_id]->steps_rotation = steps_rotation;
	if (steps_rotation) m_update = m_placed_shapes[place_id]->update = true;
	return 0;
}


int CVideoShape::SetPlacedShapeShape(u_int32_t place_id, u_int32_t shape_id)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id] ||
		!m_shapes || shape_id >= m_max_shapes) return -1;
	m_placed_shapes[place_id]->shape_id = shape_id;
	return 0;
}

int CVideoShape::SetPlacedShapeRotation(u_int32_t place_id, double rotation)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id]) return -1;
	while (rotation < -2*M_PI) rotation += 2*M_PI;
	while (rotation > 2*M_PI) rotation -= 2*M_PI;
	m_placed_shapes[place_id]->rotation = rotation;
	return 0;
}

int CVideoShape::SetPlacedShapeAlpha(u_int32_t place_id, double alpha)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id]) return -1;
	m_placed_shapes[place_id]->alpha = alpha < 0.0 ? 0.0 : alpha > 1.0 ? 1.0 : alpha;
	return 0;
}

int CVideoShape::SetPlacedShapeScale(u_int32_t place_id, double scale_x, double scale_y)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id] ||
		scale_x <= 0.0 || scale_y <= 0.0) return -1;
	m_placed_shapes[place_id]->scale_x = scale_x;
	m_placed_shapes[place_id]->scale_y = scale_y;
	return 0;
}

int CVideoShape::SetPlacedShapeOffset(u_int32_t place_id, double offset_x, double offset_y)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id]) return -1;
	m_placed_shapes[place_id]->offset_x = offset_x;
	m_placed_shapes[place_id]->offset_y = offset_y;
	return 0;
}

int CVideoShape::SetPlacedShapeRGB(u_int32_t place_id, double red, double green, double blue)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places || !m_placed_shapes[place_id]) return -1;
	if (red < 0.0) red = 0.0;
	else if (red > 1.0) red = 1.0;
	if (green < 0.0) green = 0.0;
	else if (green > 1.0) green = 1.0;
	if (blue < 0.0) blue = 0.0;
	else if (blue > 1.0) blue = 1.0;
	m_placed_shapes[place_id]->red = red;
	m_placed_shapes[place_id]->green = green;
	m_placed_shapes[place_id]->blue = blue;
	return 0;
}

pattern_t* CVideoShape::GetPattern(u_int32_t pattern_id)
{
	if (!m_patterns || pattern_id >= m_max_patterns ||
		!m_patterns[pattern_id]) return NULL;
	return m_patterns[pattern_id];
}
cairo_pattern_t* CVideoShape::GetCairoPattern(u_int32_t pattern_id)
{
	if (!m_patterns || pattern_id >= m_max_patterns ||
		!m_patterns[pattern_id] || !m_patterns[pattern_id]->pattern)
		return NULL;
	return m_patterns[pattern_id]->pattern;
}

shape_t* CVideoShape::GetShape(u_int32_t shape_id)
{
	if (!m_shapes || shape_id >= m_max_shapes) return NULL;
	return m_shapes[shape_id];
}

placed_shape_t* CVideoShape::GetPlacedShape(u_int32_t place_id)
{
	if (!m_placed_shapes || place_id >= m_max_shape_places) return NULL;
	return m_placed_shapes[place_id];
}

int CVideoShape::AddPlacedShape(u_int32_t place_id, u_int32_t shape_id, double x, double y,
	double scale_x, double scale_y, double rotation, double red, double green, double blue,
	double alpha, const char* anchor)
{
	if (!m_shapes || !m_placed_shapes || place_id >= m_max_shape_places ||
		shape_id >= m_max_shapes || !m_shapes[shape_id] ||
		scale_x <= 0.0 || scale_y <= 0.0) return -1;
	if (!m_placed_shapes[place_id]) m_placed_shapes[place_id] =
		(placed_shape_t*) calloc(sizeof(placed_shape_t), 1);
	if (!m_placed_shapes[place_id]) return 1;
	placed_shape_t* p = m_placed_shapes[place_id];
	p->shape_id = shape_id;
	p->x = x;
	p->y = y;
	p->rotation = rotation;
	p->scale_x = scale_x;
	p->scale_y = scale_y;
	p->red = min0max1(red);
	p->green = min0max1(green);
	p->blue = min0max1(blue);
	p->alpha = min0max1(alpha);
	p->update = false;
	//p->anchor_x = 0;	// Set by calloc
	//p->anchor_y = 0;	// Set by calloc
	if (anchor && *anchor) {
		int n = SetShapeAnchor(place_id, anchor);
		if (n) {
			free(p);
			return -1;
		}
	}
	m_shape_placed_count++;
	return 0;
}

// shape [ place ] overlay (<id> | <id>..<id> | <id>..end | all) [(<id> | <id>..<id> | <id>..end | all)] ....
int CVideoShape::overlay_placed_shape(struct controller_type* ctr, const char *str)
{
	if (!str || !m_pVideoMixer) return 1;
	if (!m_pVideoMixer->m_overlay || !m_pVideoMixer->m_pCairoGraphic)
		return 0;	// return silently
	while (isspace(*str)) str++;
	if (!(*str)) return 1;
	int from, to;
	from = to = 0;
	while (*str) {
		if (!strncmp(str,"all",3)) {
			from = 0;
			to = MaxPlaces();
			if (to) to--;
			while (*str) str++;
		} else if (!strncmp(str,"end",3)) {
			to = MaxPlaces();
			if (to) to--;
			while (*str) str++;
		} else {
			int n=sscanf(str, "%u", &from);
			if (n != 1) return 1;
			while (isdigit(*str)) str++;
			n=sscanf(str, "..%u", &to);
			if (n != 1) to = from;
			else {
				str += 2;
			}
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
		}
		if (from > to) return 1;
		while (from <= to) {
			if (m_placed_shapes[from]) {
				m_pVideoMixer->m_pCairoGraphic->OverlayPlacedShape(m_pVideoMixer, from);
			}
			from++;
		}
	}
	return 0;
}

void CVideoShape::Update()
{
	if (!m_placed_shapes || !m_update) return;
	bool update = false;
	for (u_int32_t id=0; id < m_max_shape_places; id++) if (m_placed_shapes[id] && m_placed_shapes[id]->update) {

		placed_shape_t* p = m_placed_shapes[id];
		bool changed = false;
		if (p->steps_x) {
			p->steps_x--; p->x += p->delta_x; changed = true;
		}
		if (p->steps_y) {
			p->steps_y--; p->y += p->delta_y; changed = true;
		}
		if (p->steps_offset_x) {
			p->steps_offset_x--; p->offset_x += p->delta_offset_x; changed = true;
		}
		if (p->steps_offset_y) {
			p->steps_offset_y--; p->offset_y += p->delta_offset_y; changed = true;
		}
		if (p->steps_scale_x) {
			p->steps_scale_x--;
			if (p->scale_x + p->delta_scale_x > 0.0)
				p->scale_x += p->delta_scale_x;
			changed = true;
		}
		if (p->steps_scale_y) {
			p->steps_scale_y--;
			if (p->scale_y + p->delta_scale_y > 0.0)
				p->scale_y += p->delta_scale_y;
			changed = true;
		}
		if (p->steps_alpha) {
			p->steps_alpha--;
			p->alpha += p->delta_alpha;
			if (p->alpha > 1.0) p->alpha = 1.0;
			else if (p->alpha < 0.0) p->alpha = 0.0;
			changed = true;
		}
		if (p->steps_rotation) {
			p->steps_rotation--;
			p->rotation += p->delta_rotation;
			if (p->rotation > 2*M_PI) p->rotation -= (2*M_PI);
			else if (p->rotation < -2*M_PI) p->rotation += (2*M_PI);
			changed = true;
		}
		if ((p->update = changed)) update = true;
	}
	m_update = update;
}


// Query type 0=info 1=list 2=place 3=move
// format	: format 
// ids		: list of IDs
// maxid	: max number of ID
// nextavail	: Next available ID
// id or ids	: listing each ID
char* CVideoShape::Query(const char* query, bool tcllist)
{
	u_int32_t type, maxid;


	if (!query) return NULL;
	while (isspace(*query)) query++;
	if (!(strncasecmp("alpha ", query, 6))) {
		query += 6; type = 4;
	}
	else if (!(strncasecmp("info ", query, 5))) {
		query += 5; type = 0;
	}
       	else if (!(strncasecmp("list ", query, 5))) {
		query += 5; type = 1;
	}
       	else if (!(strncasecmp("place ", query, 6))) {
		query += 6; type = 2;
	}
	else if (!(strncasecmp("move ", query, 5))) {
		query += 5; type = 3;
	}
	else if (!(strncasecmp("syntax", query, 6))) {
		return strdup("shape (info | list | place | move | alpha) ( format | "
			"ids | maxid | nextavail | <id_list> )");
	} else return NULL;

	while (isspace(*query)) query++;
	maxid = (type < 2 ? MaxShapes() : type < 5 ? MaxPlaces() : MaxPatterns());
	if (!strncasecmp(query, "format", 6)) {
		char* str;
		if (type == 4) {
			if (tcllist) str = strdup("id alpha");
			else str = strdup("shape place <id> <alpha>");
		}
		else if (type == 0) {	// info
			if (tcllist) str = strdup("id ops_count { name }");
			else str = strdup("shape <id> <ops_count> <name>");
		}
		else if (type == 1) {	// list
			// shape_id ops_count {{ draw_operation } { draw_operation } ...}
			// shape <shape_id> <ops_count> <<draw_operation>> <<draw_operation>>
			if (tcllist) str = strdup("shape_id ops_count { draw_operations }");
			else str = strdup("shape <shape_id> <ops_count> <<draw_operations>> ");
		}
		else if (type == 2) {	// place
			if (tcllist) str = strdup("place_id shape_id coor offset anchor scale rotation rgba");
			else str = strdup("shape place <place_id> <shape_id> <coor> <offset> <anchor> <scale> <rotation> <rgba>");
			//if (tcllist) str = strdup("place_id shape_id {coor x y} {offset xoff yoff} {anchor anchor_x anchor_y} {scale scale_x scale_y} rotation {rgba red green blue alpha}");
		}
		else {	// type == 3
			if (tcllist) str = strdup("place_id coor_move offset_move scale_move rotation_move alpha_move");
			else str = strdup("shape place move <coor move> <offset move> <scale move> <rotation move> <alpha move>");
		}
		if (!str) fprintf(stderr, "CVideoShape::Query failed to allocate memory\n");
		return str;
	}
	else if (!strncasecmp(query, "maxid", 5)) {
		int len = (int)(log10((double)maxid)) + 3;	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) fprintf(stderr, "CVideoShape::Query failed to allocate memory\n");
		else if (maxid) sprintf(str, "%u", maxid-1);
		else {
			free(str);
			str=NULL;
		}
		return str;
	}
	else if (!strncasecmp(query, "nextavail", 9)) {
		u_int32_t from = 0;
		query += 9;
		while (isspace(*query)) query++;
		sscanf(query,"%u", &from);	// get "from" if it is there
		
		int len = (int)(log10((double)maxid)) + 3;	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) {
			fprintf(stderr, "CVideoShape::Query failed to allocate memory\n");
			return NULL;
		}
		*str = '\0';
		for (u_int32_t i = from; i < maxid ; i++) {
			if ((type < 2 && !m_shapes[i]) ||
				(type > 1 && type < 4 && !m_placed_shapes[i]) ||
				(type > 3 && !m_patterns[i])) {
				sprintf(str, "%u ", i);
				break;
			}
		}
		if (str && !(*str)) {
			free(str);
			str = NULL;
		}
		return str;
	}
	else if (!strncasecmp(query, "ids", 3)) {
		int len = (int)(0.5 + log10((double)maxid)) + 1;
		len *= maxid;
		char* str = (char*) malloc(len+1);
		if (!str) fprintf(stderr, "CVideoShape::Query failed to allocate memory\n");
		else {
			*str = '\0';
			char* p = str;
			for (u_int32_t i = 0; i < maxid ; i++) {
				if ((type < 2 && m_shapes[i]) ||
					(type > 1 && type < 4 && m_placed_shapes[i]) ||
					(type > 3 && m_patterns[i])) {
					sprintf(p, "%u ", i);
					while (*p) p++;
				}
			}
		}
		return str;
	}

	const char* str = query;
	int len = 1;
	while (*str) {
		u_int32_t from, to;
		const char* nextstr = GetIdRange(str, &from, &to, 0, maxid ? maxid-1 : 0);
		if (!nextstr) return NULL;
		while (from <= to) {
			if ((type < 2 && !m_shapes[from]) ||
				(type > 1 && type < 5 && !m_placed_shapes[from]) ||
				(type == 5 && !m_patterns[from])) {
				from++ ; continue;
			}
			// type 4 - shape place <id> <alpha>
			//          {1234567890 0.123}
			//          shape place 01234567890 0.123
			//	    123456789012345678901234567890
			if (type == 4) {
				len += 30;
			}

			// type 0 - id ops_count {name}
			// type 0 - shape <shape_id> <ops_count> <name>
			// shape 1234567890 1234567890 <.....>
			// 1234567890123456789012345678901
			else if (type == 0) {
				len += (31 + (m_shapes[from]->name ? strlen(m_shapes[from]->name) : 0));
			}
			// shape_id ops_count {{draw_operation}{draw_operation} ...}
			// shape <shape_id> <ops_count> <<draw_operation>> <<draw_operation>>
			// shape 1234567890 1234567890 <
			// 12345678901234567890123456789
			else if (type == 1) {
				len += 29;
				draw_operation_t* p = m_shapes[from]->pDraw;
				while (p) {
					len += (2 + (p->create_str ? strlen(p->create_str) : 0));
					p = p->next;
				}
			}
			else if (type == 2) {
				//place_id shape_id coor offset anchor scale rotation rgba
				// shape place <place_id> <shape_id> <coor> <offset> <anchor> <scale> <rotation> <rgba>
				// shape place 1234567890 1234567890 1234567890.12345,1234567890.12345 1234567890.12345,1234567890.12345 1234567890,1234567890 1234567890.1234567890,1234567890.1234567890 1.1234567890 0.12345,0.12345,0.12345,0.12345
				// 1234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
				len += 214;
			}
			else if (type == 3) {
				// place_id coor_move offset_move scale_move rotation_move alpha_move
				// shape place move <coor move> <offset move> <scale move> <rotation move> <alpha move>
				// { 1234567890 {1234567890.123456789 1234567890.123456789 1234567890 1234567890} {1234567890.123456789 1234567890.123456789 1234567890 1234567890} {1234567890.123456789 1234567890.123456789 1234567890 1234567890 {1234567890.123456789 1234567890} {1234567890.123456789 123456789} 
				// shape place move 1234567890 1234567890.123456789,1234567890.123456789,1234567890,1234567890 1234567890.123456789,1234567890.123456789,1234567890,1234567890 1234567890.123456789,1234567890.123456789,1234567890,1234567890 1234567890.123456789,1234567890 1234567890.123456789,123456789
				// 12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890
				len += 284;
			}
			else return NULL;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	if (len < 2) {
		fprintf(stderr, "CVideoShape::Query malformed query\n");
		return NULL;
	}
	char* retstr = (char*) malloc(len);
	if (!retstr) {
		fprintf(stderr, "CVideoShape::Query failed to allocate memory\n");
		return NULL;
	}

	str = query;
	char* pstr = retstr;
	*pstr = '\0';
	while (*str) {
		u_int32_t from, to;
		const char* nextstr = GetIdRange(str, &from, &to, 0, maxid ? maxid-1 : 0);
		if (!nextstr) {
            free(retstr);
            return NULL;
        }
		while (from <= to) {

			if ((type < 2 && !m_shapes[from]) ||
				(type > 1 && type < 5 && !m_placed_shapes[from]) ||
				(type == 5 && !m_patterns[from])) {
				from++ ; continue;
			}

			// shape place <id> <alpha>
			// {1234567890 0.123}
			// shape place 01234567890 0.123
			if (type == 4) {
				sprintf(pstr, "%s%u %.3lf%s",
					tcllist ? "{" : "shape place ",
					from,
					m_placed_shapes[from]->alpha,
					tcllist ? "} " : "\n");
			}
			// id geometry bits color file_name
			// image load <width> <height> <bits> <color> <<file_name>>
			else if (type == 0) {
				// type 0 - id ops_count {name}
				// type 0 - shape <shape_id> <ops_count> <name>
				sprintf(pstr, "%s%u %u %s%s%s",
					tcllist ? "{" : "shape ",
					from,
					m_shapes[from]->ops_count,
					tcllist ? "{" : "<",
					m_shapes[from]->name ? m_shapes[from]->name : "",
					tcllist ? "}} " : ">\n");
			}
			// shape_id ops_count {{draw_operation}{draw_operation} ...}
			// shape <shape_id> <ops_count> <<draw_operation>> <<draw_operation>>
			else if (type == 1) {
				sprintf(pstr,
					(tcllist ? "{%u %u {" : "shape %u %u\n"),
					from, m_shapes[from]->ops_count);
				while (*pstr) pstr++;

				draw_operation_t* p = m_shapes[from]->pDraw;
				while (p) {
					sprintf(pstr, (tcllist ? "{%s" : " - %s\n"),
						 p->create_str ? p->create_str : "");
					while (*pstr) pstr++;
					if (tcllist) {
						if (*(pstr-1) == ' ') pstr--;
						sprintf(pstr, "}%s",
							p->next ? " " : "");
						while (*pstr) pstr++;
					}
					p = p->next;
				}
				while (*pstr) pstr++;
				if (tcllist) {
					sprintf(pstr, "} } ");
					while (*pstr) pstr++;
				}
			}
			else if (type == 2) {
				// place_id shape_id coor offset anchor scale rotation rgba
				// shape place <place_id> <shape_id> <coor> <offset> <anchor> <scale> <rotation> <rgba>
				placed_shape_t* p = m_placed_shapes[from];
				sprintf(pstr,
					"%s%u %u %s%.9lf%s%.9lf%s %s%.9lf%s%.9lf%s %s%u%s%u%s "
					"%s%.9lf%s%.9lf%s %.9lf %s%.5lf%s%.5lf%s%.5lf%s%.5lf%s",
					tcllist ? "{" : "shape place ",
					from, p->shape_id,
					tcllist ? "{" : "",
					p->x, tcllist ? " " : ",", p->y,
					tcllist ? "}" : "",
					tcllist ? "{" : "",
					p->offset_x, tcllist ? " " : ",", p->offset_y,
					tcllist ? "}" : "",
					tcllist ? "{" : "",
					p->anchor_x, tcllist ? " " : ",", p->anchor_y,
					tcllist ? "}" : "",
					tcllist ? "{" : "",
					p->scale_x, tcllist ? " " : ",", p->scale_y,
					tcllist ? "}" : "",
					p->rotation,
					tcllist ? "{" : "",
					p->red, tcllist ? " " : ",",
					p->blue, tcllist ? " " : ",",
					p->green, tcllist ? " " : ",",
					p->alpha,
					tcllist ? "}} " : "\n");
			}
			else if (type == 3) {
				// place_id coor_move offset_move scale_move rotation_move alpha_move
				// shape place move <coor move> <offset move> <scale move> <rotation move> <alpha move>
				placed_shape_t* p = m_placed_shapes[from];
				sprintf(pstr, "%s%u %s%.9lf%s%.9lf%s%u%s%u%s "
					"%s%.9lf%s%.9lf%s%u%s%u%s %s%.9lf%s%.9lf%s%u%s%u%s "
					"%s%.9lf%s%u%s %s%.9lf%s%u%s",

					tcllist ? "{" : "shape place move ", from,

					tcllist ? "{" : "",		// coor move
					p->delta_x,
					tcllist ? " " : ",",
					p->delta_y,
					tcllist ? " " : ",",
					p->steps_x,
					tcllist ? " " : ",",
					p->steps_y,
					tcllist ? "}" : "",
					
					tcllist ? "{" : "",		// offset move
					p->delta_offset_x,
					tcllist ? " " : ",",
					p->delta_offset_y,
					tcllist ? " " : ",",
					p->steps_offset_x,
					tcllist ? " " : ",",
					p->steps_offset_y,
					tcllist ? "}" : "",

					tcllist ? "{" : "",		// scale move
					p->delta_scale_x,
					tcllist ? " " : ",",
					p->delta_scale_y,
					tcllist ? " " : ",",
					p->steps_scale_x,
					tcllist ? " " : ",",
					p->steps_scale_y,
					tcllist ? "}" : "",

					tcllist ? "{" : "",		// rotation move
					p->delta_rotation,
					tcllist ? " " : ",",
					p->steps_rotation,
					tcllist ? "}" : "",

					tcllist ? "{" : "",		// alpha move
					p->delta_alpha,
					tcllist ? " " : ",",
					p->steps_alpha,
					tcllist ? "}} " : "\n");

			} else {
				if (retstr) free (retstr);
				return NULL;
			}
			while (*pstr) pstr++;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	return retstr;
}

// help list for shapes
int CVideoShape::list_shape_help(struct controller_type* ctr, const char* str)
{
	if (m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"Commands:\n"

			MESSAGE"shape add [shape id> [<shape name>]]  // shape id alone deletes shape\n"
			MESSAGE"shape arc_cw <shape id> <x> <y> <radius> <angle from> <angle to>\n"
			MESSAGE"shape arc_ccw <shape id> <x> <y> <radius> <angle from> <angle to>\n"
			MESSAGE"shape clip <shape id>\n"
			MESSAGE"shape closepath <shape_id>\n"
			MESSAGE"shape curveto <shape id> <x1> <y1> <x2> <y2> <x3> <y3>\n"
			MESSAGE"shape curverel <shape id> <x1> <y1> <x2> <y2> <x3> <y3>\n"
			MESSAGE"shape feed <shape id> <feed id> <x> <y> <scale x> <scale y>\n"   // (left | center | right | top | middle | bottom)\n"
			MESSAGE"shape fill <shape id>\n"
			MESSAGE"shape fill preserve <shape id>\n"
			MESSAGE"shape filter <shape id> (fast | good | best | nearest | bilinear | gaussian)\n"
			MESSAGE"shape help  // list this list\n"
			MESSAGE"shape info\n"
			MESSAGE"shape image <shape id> <image id> <x> <y> <scale_x> <scale_y>\n"  // (left | center | right | top | middle | bottom) NOT IMPLEMENTED"
			MESSAGE"shape inshape <shape id> <inshape id>\n"
			MESSAGE"shape line width <shape id> <width>\n"
			MESSAGE"shape line join <shape id> (miter | round | bevel) [<miter limit>]\n"
			MESSAGE"shape line cap <shape id> (butt | round | square)\n"
			MESSAGE"shape lineto <shape id> <x> <y>\n"
			MESSAGE"shape linerel <shape id> <x> <y>\n"
			MESSAGE"shape list <shape id>\n"
			MESSAGE"shape mask pattern <shape_id> <pattern id>\n"
			MESSAGE"shape moveentry <shape id> <entry id> [<to entry>]\n"
			MESSAGE"shape moverel <shape id> <x> <y>\n"
			MESSAGE"shape moveto <shape id> <x> <y>\n"
			MESSAGE"shape newpath <shape id>\n"
			MESSAGE"shape newsubpath <shape id>\n");
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"shape operator <shape id> (clear | source | over | in | out | atop | dest | dest_over | dest_in | dest_out | dest_atop | xor | add | saturate | multiply | screen | overlay | darken | lighten | dodge | burn | hard | soft | difference | exclusion | hsl_hue | hsl_saturation | hsl_color | hsl_luminosity)\n"
			MESSAGE"shape paint <shape id> [<alpha>]\n"
			MESSAGE"shape pattern add [<pattern id> [<pattern name>]]\n"
			MESSAGE"shape pattern radial <pattern id> <cx0> <cy0> <radius0> <cx1> <cy1> <radius1>\n"
			MESSAGE"shape pattern linear <pattern id> <x1> <y1> <x2> <y2>\n"
			MESSAGE"shape pattern stoprgb <pattern id> <offset> <red> <green> <blue> [<alpha>]\n"
			MESSAGE"shape rectangle <shape id> <x> <y> <width> <height>\n"
			MESSAGE"shape recursion <shape id> <level>\n"
			MESSAGE"shape restore <shape id>\n"
			MESSAGE"shape rotation <shape id> <relative rotation> [<absolute rotation>]\n"
			MESSAGE"shape save <shape id>\n"
			MESSAGE"shape scale <shape id> <scale x> <scale y>\n"
			MESSAGE"shape source alphamul <shape id> <alpha>\n"
			MESSAGE"shape source rgb <shape id> <red> <green> <blue> [<alpha>]\n"
			MESSAGE"shape source rgba <shape id> <red> <green> <blue> [<alpha>]\n"
			MESSAGE"shape source pattern <shape_id> <pattern id>\n"
			MESSAGE"shape stroke <shape id>\n"
			MESSAGE"shape stroke preserve <shape id>\n"
			MESSAGE"shape translate <shape id> <offset x> <offset y>\n"
			MESSAGE"shape transform <shape id> <xx> <yx> <xy> <yy> <x0> <y0>\n"
			MESSAGE"shape verbose [<level>]\n"
			MESSAGE"shape place help\n"
			MESSAGE"\n");
	}
	return 0;
}
int CVideoShape::list_shape_place_help(struct controller_type* ctr, const char* str)
{
	if (m_pVideoMixer && m_pVideoMixer->m_pController) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"Commands:\n"
			MESSAGE"shape [ place ] overlay <place id> (<id> | <id>..<id> | all | end | <id>..end) [ (<id> | <id>..<id> | all | end | <id>..end) ] ....\n"
			MESSAGE"shape place [<place id> [ <shape id> <x> <y> <scale x> <scale y> [<rotation> [<red> <green> <blue> [<alpha>]]]]] // shape id alone deletes placed shape\n"
			MESSAGE"shape place alpha [<place id> <alpha>]\n"
			MESSAGE"shape place anchor [<place id> "ANCHOR_SYNTAX"\n"
			MESSAGE"shape place coor [<place id> <x> <y> ["ANCHOR_SYNTAX"]]\n"
			MESSAGE"shape place move alpha <place id> <delta alpha> <steps>\n"
			MESSAGE"shape place move coor <place id> <delta x> <delta y> <steps x> <steps y>\n"
			MESSAGE"shape place move offset <place id> <delta x> <delta y> <steps x> <steps y>\n"
			MESSAGE"shape place move rotation <place id> <delta rotation> <steps>\n"
			MESSAGE"shape place move scale <place id> <delta scale_x> <delta scale y> <steps x> <steps y>\n"
			MESSAGE"shape place offset [<place id> <offset x> <offset y>]\n"
			MESSAGE"shape place overlay (<id> | <id>..<id> | all | end | <id>..end) [ (<id> | <id>..<id> | all | end | <id>..end) ] ....\n"
			MESSAGE"shape place rgb [<place id> <red> <green> <blue> [<alpha>]]\n"
			MESSAGE"shape place rotation [<place id> <rotation>]\n"
			MESSAGE"shape place scale [<place id> <scale x> <scale y>]\n"
			MESSAGE"shape place shape [<place id> <shape id>]\n"
			MESSAGE"shape place status    // list shape placed\n"
			MESSAGE"\n");
	}
	return 0;
}


// Will return a copy of the command string. str is expected to point somewhere
// in the command string that begins with "shape ......" meaning we search backwards.
// We then omit the the 'shape ' part, as it is implicit for all.
char* CVideoShape::DuplicateCommandString(const char* str)
{
	if (!str) return NULL;

	// initially we subtract 6 which is the length of "shape "
	str -= 6;
	while (strncasecmp(str, "shape ", 6)) str--;
	str += 6;
	while (isspace(*str)) str++;
	return strdup(str);
}
