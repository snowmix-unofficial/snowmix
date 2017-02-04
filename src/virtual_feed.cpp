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
#include <stdio.h>
#include <ctype.h>
//#include <limits.h>
//#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
//#include <stdlib.h>
//#include <sys/types.h>
#include <math.h>
//#include <string.h>

//#include "cairo/cairo.h"
#include "video_mixer.h"
//#include "video_text.h"
//#include "video_scaler.h"
//#include "add_by.h"
//#include "video_image.h"

#include "snowmix.h"
#include "virtual_feed.h"
#include "snowmix_util.h"

#ifdef WIN32
#define _USE_MATH_DEFINES
#include <math.h>
#include "windows_util.h"
#endif
#ifndef M_PI
#error M_PI not defined
#endif

CVirtualFeed::CVirtualFeed(CVideoMixer* pVideoMixer, u_int32_t max_feeds)
{
	m_max_feeds = 0;
	m_verbose = 0;
	m_feed_count = 0;
	m_feeds = (virtual_feed_t**) calloc(
		sizeof(virtual_feed_t*), max_feeds);
	//for (unsigned int i=0; i< m_max_feeds; i++) m_feeds[i] = NULL;
	if (m_feeds) {
		m_max_feeds = max_feeds;
	}
	m_pVideoMixer = pVideoMixer;
}

CVirtualFeed::~CVirtualFeed()
{
	if (m_feeds) {
		for (unsigned int i=0; i<m_max_feeds; i++) {
			DeleteFeed(i);
//			if (m_feeds[i]) {
//				if (m_feeds[i]->name)
//					free(m_feeds[i]->name);
//				free(m_feeds[i]);
//			}
		}
		free(m_feeds);
		m_feeds = NULL;
	}
	m_max_feeds = 0;
}

int CVirtualFeed::ParseCommand(struct controller_type* ctr, const char* str)
{
	if (!m_pVideoMixer) return 0;

	if (!strncasecmp (str, "add ", 4)) {
		if (set_virtual_feed_add(ctr, str+4)) return -1;
	// virtual feed overlay
	} else if (!strncasecmp (str, "overlay ", 8)) {
		if (!m_pVideoMixer) return 1;
		if (OverlayFeed(m_pVideoMixer->m_pCairoGraphic, str+8)) return -1;
	} else if (!strncasecmp (str, "align ", 6)) {
		if (set_virtual_feed_align(ctr, str+6)) return -1;
	} else if (!strncasecmp (str, "alpha ", 6)) {
		if (set_virtual_feed_alpha(ctr, str+6)) return -1;
	} else if (!strncasecmp (str, "anchor ", 7)) {
		if (set_virtual_feed_anchor(ctr, str+7)) return -1;
	} else if (!strncasecmp (str, "coor ", 5)) {
		if (set_virtual_feed_coor(ctr, str+5)) return -1;
	} else if (!strncasecmp (str, "clip ", 5)) {
		if (set_virtual_feed_clip(ctr, str+5)) return -1;
	} else if (!strncasecmp (str, "geometry ", 9)) {
		if (set_virtual_feed_geometry(ctr, str+9)) return -1;
	} else if (!strncasecmp (str, "rotation ", 9)) {
		if (set_virtual_feed_rotation(ctr, str+9)) return -1;
	} else if (!strncasecmp (str, "offset ", 7)) {
		if (set_virtual_feed_offset(ctr, str+7)) return -1;
	} else if (!strncasecmp (str, "source ", 7)) {
		if (set_virtual_feed_source(ctr, str+7)) return -1;
	// virtual feed place rect [<vir id> [<x> <y> <width> <height> <src_x> <src_y>
	//	[<rotation> <scale_x> <scale_y> <alpha>]]]
	} else if (!strncasecmp (str, "place rect ", 11)) {
		if (set_virtual_feed_place_rect(ctr, str+11)) return -1;
	} else if (!strncasecmp (str, "move ", 5)) {
		str += 5;
		while (isspace(*str)) str++;
		if (!strncasecmp (str, "coor ", 5)) {
			if (set_virtual_feed_move_coor(ctr, str+5)) return -1;
		} else if (!strncasecmp (str, "clip ", 5)) {
			if (set_virtual_feed_move_clip(ctr, str+5)) return -1;
		} else if (!strncasecmp (str, "scale ", 6)) {
			if (set_virtual_feed_move_scale(ctr, str+6)) return -1;
		} else if (!strncasecmp (str, "rotation ", 9)) {
			if (set_virtual_feed_move_rotation(ctr, str+9)) return -1;
		} else if (!strncasecmp (str, "alpha ", 6)) {
			if (set_virtual_feed_move_alpha(ctr, str+6)) return -1;
		} else if (!strncasecmp (str, "offset ", 7)) {
			if (set_virtual_feed_move_offset(ctr, str+7)) return -1;
		} else return -1;
	} else if (!strncasecmp (str, "scale ", 6)) {
		if (set_virtual_feed_scale(ctr, str+6)) return -1;
	} else if (!strncasecmp (str, "matrix ", 7)) {
		if (set_virtual_feed_matrix(ctr, str+7)) return -1;
	} else if (!strncasecmp (str, "filter ", 7)) {
		if (set_virtual_feed_filter(ctr, str+7)) return -1;
	} else if (!strncasecmp (str, "info ", 5)) {
		if (set_info(ctr, str+5)) return -1;
	} else if (!strncasecmp (str, "verbose ", 8)) {
		if (set_verbose(ctr, str+8)) return -1;
	} else if (!strncasecmp (str, "help ", 5)) {
		if (list_help(ctr, str+5)) return -1;
	} else if (!strncasecmp (str, "mode ", 5)) {
		u_int32_t id, mode;
		while (isspace(*str)) str++;
		if (sscanf(str,"%u %u", &id, &mode) == 2) {
			if (m_feeds && m_feeds[id]) m_feeds[id]->mode = mode ? 1 : 0;
		} else if (m_pVideoMixer && m_pVideoMixer->m_pController) {
			for (id = 0; id < m_max_feeds; id++) if (m_feeds && m_feeds[id])
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"MSG: vfeed %u mode %u\n", id, m_feeds[id]->mode);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		}
	} else return 1;
	return 0;
}

// Query type 0=geometry 1=move 2=extended
// format	: format 
// ids		: list of IDs
// maxid	: max number of ID
// nextavail	: Next available ID
// id or ids	: listing each ID
char* CVirtualFeed::Query(const char* query, bool tcllist)
{
	int len;
	u_int32_t type, maxid;
	char *pstr, *retstr;
	const char *str;

	if (!query) return NULL;
	while (isspace(*query)) query++;
	if (!(strncasecmp("place ", query, 6))) {
		query += 6; type = 0;
	}
       	else if (!(strncasecmp("move ", query, 5))) {
		query += 5; type = 1;
	}
	else if (!(strncasecmp("extended ", query, 9))) {
		query += 9; type = 2;
	}
       	else if (!(strncasecmp("syntax", query, 6))) {
		return strdup("(vfeed | virtual feed) (place | move | extended | syntax) "
			"(format | ids | maxid | nextavail | <id_list>)");
	} else return NULL;

	while (isspace(*query)) query++;
	maxid = MaxFeeds();

	if (!strncasecmp(query, "format", 6)) {
		char* str;
		if (type == 0) {
			// geometry - feed_id source_id source_type coor offset geometry scale rotate alpha clip
			// geometry - feed <feed_id> <source_id> <source_type> <coor> <offset> <geometry> <scale> <rotate> <alpha> <clip>");
			if (tcllist) str = strdup("feed_id source_id source_type coor offset geometry scale rotate alpha clip filter");
			else str = strdup("vfeed <feed_id> <source_id> <source_type> <coor> <offset> <geometry> <scale> <rotate> <alpha> <clip> <filter>");

		}
		else if (type == 1) {
			// move - feed_id coor_move scale_move rotate_move alpha_move clip_move
			// move - virtual feed move <feed_id> <coor_move> <scale_move> <rotate_move> <alpha_move> <clip_move>
			if (tcllist) str = strdup("feed_id coor_move scale_move rotate_move alpha_move clip_move");
			else str = strdup("vfeed move <feed_id> <coor_move> <scale_move> <rotate_move> <alpha_move> <clip_move>");
		} else {
			// extended - feed_id matrix
			// extended - virtual feed extended <feed_id> <matrix>

			if (tcllist) str = strdup("feed_id matrix");
			else str = strdup("vfeed extended <feed_id> <matrix>");
		}
		if (!str) goto malloc_failed;
		return str;
	}
	else if (!strncasecmp(query, "maxid", 5)) {
		len = (int) (log10((double)maxid) + 3);	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) goto malloc_failed;
		if (maxid) sprintf(str, "%u", maxid-1);
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
		
		len = (int)(log10((double)maxid) + 3);	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) goto malloc_failed;
		*str = '\0';
		for (u_int32_t i = from; i < maxid ; i++) {
			if (!m_feeds[i]) {
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
		len = (int)(0.5+log10((double)maxid) + 1);
		len *= maxid;
		char* str = (char*) malloc(len+1);
		if (!str) goto malloc_failed;
		*str = '\0';
		char* p = str;
		for (u_int32_t i = 0; i < maxid ; i++) {
			if (m_feeds[i]) {
				sprintf(p, "%u ", i);
				while (*p) p++;
			}
		}
		return str;
	}

	str = query;
	len = 1;
	while (*str) {
		u_int32_t from, to;
		const char* nextstr = GetIdRange(str, &from, &to, 0, maxid ? maxid-1 : 0);
		if (!nextstr) return NULL;
		virtual_feed_t* pf;
		while (from <= to) {
			if (!(pf = m_feeds[from])) { from++ ; continue; }
			// geometry - feed_id source_id source_type coor offset geometry scale rotate alpha clip
			// move - feed_id coor_move scale_move rotate_move alpha_move clip_move
			// extended - feed_id matrix
			if (type < 2)
				len += 150;
			else len += 256;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	if (len < 2) {
		fprintf(stderr, "CVirtualFeed::Query malformed query\n");
		return NULL;
	}
	retstr = (char*) malloc(len);
	if (!retstr) goto malloc_failed;

	str = query;
	pstr = retstr;
	*pstr = '\0';

	while (*str) {
		u_int32_t from, to;
		const char* nextstr = GetIdRange(str, &from, &to, 0, maxid ? maxid-1 : 0);
		if (!nextstr) {
	    free(retstr);
	    return NULL;
	}
		while (from <= to) {

			// Find the feed and if not, then skip to the next id
			virtual_feed_t* pf = m_feeds[from];
			if (!(pf)) { from++ ; continue; }

			// geometry - feed_id source_id source_type coor offset geometry scale rotate alpha clip
			if (type == 0) {
				sprintf(pstr, (tcllist ?
					"{%u %u %s {%u %u} {%u %u} {%u %u} {%.9lf %.9lf} %.9lf %.5lf "
					  "{%u %u %u %u} %s} " :
					"vfeed %u %u %s %u,%u %u,%u %ux%u %.9lf,%.9lf %.9lf "
					"%.5lf %u,%u,%u,%u %s\n"),
					from, pf->source_id, 
					pf->source_type == VIRTUAL_FEED_IMAGE ? "image" :
					  pf->source_type == VIRTUAL_FEED_FEED ? "feed" : "none",
					pf->x, pf->y, pf->offset_x, pf->offset_y,
					pf->width, pf->height, pf->scale_x, pf->scale_y,
					pf->rotation, pf->alpha,
					pf->clip_x, pf->clip_y, pf->clip_w, pf->clip_h,
					CairoFilter2String(pf->filter));
			}
			// move - feed_id coor_move scale_move rotate_move alpha_move clip_move
			else if (type == 1) {
				sprintf(pstr, (tcllist ?
					"{%u {%d %d %u %u} {%.9lf %.9lf %u %u} {%.9lf %u} "
					  "{%.9lf %u} {%d %d %d %d %u %u %u %u}} " :
					"vfeed move %u %d,%d,%u,%u %.9lf,%.9lf,%u,%u %.9lf,%u "
					  "%.9lf,%u %d,%d,%d,%d,%u,%u,%u,%u\n"),
					from, pf->delta_x, pf->delta_y,
					pf->delta_x_steps, pf->delta_y_steps, 
					pf->delta_scale_x, pf->delta_scale_y,
					pf->delta_scale_x_steps, pf->delta_scale_y_steps, 
					pf->delta_rotation, pf->delta_rotation_steps,
					pf->delta_alpha, pf->delta_alpha_steps,
					pf->delta_clip_x, pf->delta_clip_y,
					pf->delta_clip_w, pf->delta_clip_h,
					pf->delta_clip_x_steps, pf->delta_clip_y_steps,
					pf->delta_clip_w_steps, pf->delta_clip_h_steps);

			}
			// extended - feed_id matrix
			else if (type == 2) {
				sprintf(pstr, (tcllist ? "{ %u {%.9lf %.9lf %.9lf %.9lf %.9lf %.9lf}} " :
					  "image extended %u %.9lf,%.9lf,%.9lf,%.9lf,%.9lf,%.9lf\n"),
					from,
					pf->pMatrix ? pf->pMatrix->matrix_xx : 0.0,
					pf->pMatrix ? pf->pMatrix->matrix_xy : 0.0,
					pf->pMatrix ? pf->pMatrix->matrix_yx : 0.0,
					pf->pMatrix ? pf->pMatrix->matrix_yy : 0.0,
					pf->pMatrix ? pf->pMatrix->matrix_x0 : 0.0,
					pf->pMatrix ? pf->pMatrix->matrix_y0 : 0.0);

			}
			while (*pstr) pstr++;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	return retstr;

malloc_failed:
	fprintf(stderr, "CVirtualFeed::Query failed to allocate memory\n");
	return NULL;
}

// Create a virtual feed
int CVirtualFeed::list_help(struct controller_type* ctr, const char* str)
{
	if (m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Commands:\n"
		MESSAGE"vfeed add [<vir id> [<feed name>]]  // empty <feed name> deletes entry\n"
		MESSAGE"vfeed align [<vir id> (left | center | right) (top | middle | bottom)]\n"
		MESSAGE"vfeed alpha [<vir id> <alpha>]\n"
		MESSAGE"vfeed anchor [<vir id> "ANCHOR_SYNTAX"]\n"
		MESSAGE"vfeed coor [<vir id> <x> <y>]\n"
		MESSAGE"vfeed clip [<vir id> <clip w> <clip h> <clip x> <clip y>]\n"
		MESSAGE"vfeed filter [<place id> (fast | good | best | nearest | bilinear | gaussian)]\n"
		//MESSAGE"vfeed geometry [<vir id> <width> <height>]\n"
		MESSAGE"vfeed geometry [<vir id>]\n"
		MESSAGE"vfeed place rect [<vir id> [<x> <y> <clip w> <clip h> <clip x> <clip y> "
			"[<rotation> <scale_x> <scale_y> <alpha>]]]\n"
//		"vfeed place circle <vir id> <x> <y> <radius> <src_x> <src_y> "
//			"<scale_x> <scale_y> <alpha>\n"
		MESSAGE"vfeed move alpha [<vir id> <delta alpha> <steps>]\n"
		MESSAGE"vfeed move coor [<vir id> <delta x> <delta y> <step x> <step y>]\n"
		MESSAGE"vfeed move clip [<vir id> <delta clip x> <delta clip y> <delta clip w> "
			"<delta clip h> <step x> <step y> <step w> <step y>]\n"
		MESSAGE"vfeed move offset [<vir id> <delta offset x> <delta offset y> <step offset x> <step offset y>]\n"
		MESSAGE"vfeed move rotation [<vir id> <delta rotation> <step step rotation>]\n"
		MESSAGE"vfeed move scale [<vir id> <delta sx> <delta sy> <step sx> <step sy>]\n"
		MESSAGE"vfeed overlay (<id> | <id>..<id> | all | end | <id>..end) "
			"[ (<id> | <id>..<id> | all | end | <id>..end) ] ....\n"
		MESSAGE"vfeed rotation [<vir id> <rotation>]\n"
		MESSAGE"vfeed scale [<vir id> <scale_x> <scale_y>]\n"
		MESSAGE"vfeed source [(feed | image) <vir id> (<feed id> | <image id>)]\n"
		MESSAGE"\n"
		);
	return 0;
}

// vfeed info
int CVirtualFeed::set_info(struct controller_type* ctr, const char* str)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
//#define MSG "MSG: "
	m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"virtual feed info:\n"
		MESSAGE"vfeed count : %u of %u\n"
		MESSAGE"vfeed verbose level : %u\n"
		MESSAGE"vfeed id : state source source_id coors geometry scale clip_cors clip_geometry rotation alpha filter\n",
		m_feed_count, m_max_feeds,
		m_verbose
		);
	for (u_int32_t id = 0; (unsigned) id < m_max_feeds ; id++) if (m_feeds[id]) {
		feed_state_enum state = UNDEFINED;
		if (m_feeds[id]->source_type == VIRTUAL_FEED_FEED && m_pVideoMixer->m_pVideoFeed) {
			feed_state_enum* pState = m_pVideoMixer->m_pVideoFeed->FeedState(m_feeds[id]->source_id);
			if (pState) state = *pState;
		}
			
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
		MESSAGE"vfeed %u : %s %s %u %d,%d %ux%u %.4lf,%.4lf %u,%u %ux%u %.5lf %.4lf %s\n",
		id, feed_state_string(state),
		m_feeds[id]->source_type == VIRTUAL_FEED_FEED ? "feed" :
		  m_feeds[id]->source_type == VIRTUAL_FEED_IMAGE ? "image" : "unknown",
		m_feeds[id]->source_id,
		m_feeds[id]->x, m_feeds[id]->y,
		m_feeds[id]->width, m_feeds[id]->height,
		m_feeds[id]->scale_x, m_feeds[id]->scale_y,
		m_feeds[id]->clip_x, m_feeds[id]->clip_y,
		m_feeds[id]->clip_w, m_feeds[id]->clip_h,
		m_feeds[id]->rotation, m_feeds[id]->alpha,
		CairoFilter2String(m_feeds[id]->filter)
		);
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
	return 0;
}



// vfeed verbose [<level>] 
int CVirtualFeed::set_verbose(struct controller_type* ctr, const char* str)
{
	int n;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_verbose) m_verbose = 0;
		else m_verbose = 1;
		if (m_pVideoMixer && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"vfeed verbose %s\n", m_verbose ? "on" : "off");
		return 0;
	}
	while (isspace(*str)) str++;
	n = sscanf(str, "%u", &m_verbose);
	if (n != 1) return -1;
	if (m_verbose && m_pVideoMixer && m_pVideoMixer->m_pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"vfeed verbose level set to %u for CVirtualFeed\n",
			m_verbose);
	return 0;
}

// Create a virtual feed
int CVirtualFeed::set_virtual_feed_add(struct controller_type* ctr, const char* str)
{
	int id, n;

	if (!m_feeds || !str || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;

	while (isspace(*str)) str++;
	if (*str == '\0') {
		
		for (id = 0; (unsigned) id < m_max_feeds ; id++) if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"vfeed %2u : <%s>\n",
				id, m_feeds[id]->name ? m_feeds[id]->name : "");
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}

	// Get the feed id.
	n = sscanf(str, "%u", &id);
	if (n != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;

	// Now if eos, only id was given and we delete the feed
	if (!(*str)) {
		n = DeleteFeed(id);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u deleted\n", id);
	} else {
		// Now we know there is more after the id and we can create the feed
		if (CreateFeed(id)) return -1;
		n = SetFeedName(id, str);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u created named %s\n", id, str);
	}
	return n;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_clip(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"move clip %2d delta %d,%d,%d,%d steps %u,%u,%u,%u\n", id,
					m_feeds[id]->delta_clip_x, m_feeds[id]->delta_clip_y,
					m_feeds[id]->delta_clip_w, m_feeds[id]->delta_clip_h,
					m_feeds[id]->delta_clip_x_steps,
					m_feeds[id]->delta_clip_y_steps,
					m_feeds[id]->delta_clip_w_steps,
					m_feeds[id]->delta_clip_h_steps);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	// virtual feed move clip <vir id> <delta_clip_x> <delta_clip_y> <delta_clip_w>
	//	<delta_clip_h> <steps_x> <steps_y> <steps_w> <steps_h>
	int32_t delta_clip_x, delta_clip_y, delta_clip_w, delta_clip_h, n = -1;
	u_int32_t id, step_x, step_y, step_w, step_h;
	if (sscanf(str, "%u %d %d %d %d %u %u %u %u", &id, &delta_clip_x, &delta_clip_y, &delta_clip_w, &delta_clip_h, &step_x, &step_y, &step_w, &step_h) == 9) {
		n = SetFeedClipDelta(id, delta_clip_x, delta_clip_y, delta_clip_w, delta_clip_h,
			step_x, step_y, step_w, step_h);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u move clip %d,%d %d,%d steps %u,%u,%u,%u\n",
				id, delta_clip_x, delta_clip_y, delta_clip_w, delta_clip_h,
				step_x, step_y, step_w, step_h);
	}
	return n;
}

// Set virtual feed matrix params
int CVirtualFeed::set_virtual_feed_matrix(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	// virtual feed matrix <vir id> <xx> <xy> <yx> <yy> <x0> <y0>
	double xx, xy, yx, yy, x0, y0;
	u_int32_t id;
	int32_t n = -1;
	if (sscanf(str, "%u %lf %lf %lf %lf %lf %lf", &id, &xx, &xy, &yx, &yy, &x0, &y0) == 7) {
		n = SetFeedMatrix(id, xx, xy, yx, yy, x0, y0);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u matrix  %lf %lf %lf %lf %lf %lf\n",
				id, xx, xy, yx, yy, x0, y0);
	}
	return n;
}

// Set virtual feed filter
// virtual feed filter [<place id> (fast | good | best | nearest | bilinear | gaussian)]
int CVirtualFeed::set_virtual_feed_filter(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"filter %2d %s\n", id,
					CairoFilter2String(m_feeds[id]->filter));
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}


	u_int32_t id;
	int32_t n = -1;
	cairo_filter_t filter;
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!strncasecmp (str, "fast ", 5)) filter=CAIRO_FILTER_FAST;
	else if (!strncasecmp (str, "good ", 5)) filter=CAIRO_FILTER_GOOD;
	else if (!strncasecmp (str, "best ", 5)) filter=CAIRO_FILTER_BEST;
	else if (!strncasecmp (str, "nearest ", 8)) filter=CAIRO_FILTER_NEAREST;
	else if (!strncasecmp (str, "bilinear ", 9)) filter=CAIRO_FILTER_BILINEAR;
	else if (!strncasecmp (str, "gaussian ", 9)) filter=CAIRO_FILTER_GAUSSIAN;
	else return -1;
	n = SetFeedFilter(id, filter);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u filter %s\n",
			id, CairoFilter2String(filter));
	return n;
}

// vfeed alpha [<vfeed id> <alpha>]
// Set virtual feed alpha
int CVirtualFeed::set_virtual_feed_alpha(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"alpha %2d %.4lf\n", id, m_feeds[id]->alpha);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	double alpha;
	u_int32_t id;
	if (sscanf(str, "%u %lf", &id, &alpha) != 2) return -1;
	int n = SetFeedAlpha(id, alpha);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u alpha set to %.5lf\n", id, alpha);
	return n;
}

// vfeed scale [<vfeed id> <scale_x> <scale_y>]
// Set virtual feed scale
int CVirtualFeed::set_virtual_feed_scale(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"%2d scale %.5lf,%0.5lf\n", id,
					m_feeds[id]->scale_x, m_feeds[id]->scale_y);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	double scale_x, scale_y;
	u_int32_t id;
	if (sscanf(str, "%u %lf %lf", &id, &scale_x, &scale_y) != 3) return -1;
	int n = SetFeedScale(id, scale_x, scale_y);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u scale set to %.5lf,%.5lf\n", id,
			m_feeds[id]->scale_x, m_feeds[id]->scale_y);
	return n;
}

// vfeed align [<vfeed id> ( left | center | right ) ( top | middle | bottom) ]
// Set virtual feed align
int CVirtualFeed::set_virtual_feed_align(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"%2d align %s %s\n", id,
					m_feeds[id]->align & SNOWMIX_ALIGN_TOP ? "top" :
					  m_feeds[id]->align & SNOWMIX_ALIGN_MIDDLE ? "middle" :
					    m_feeds[id]->align & SNOWMIX_ALIGN_BOTTOM ? "bottom" :
					    "unknown",
					m_feeds[id]->align & SNOWMIX_ALIGN_LEFT ? "left" :
					  m_feeds[id]->align & SNOWMIX_ALIGN_CENTER ? "center" :
					    m_feeds[id]->align & SNOWMIX_ALIGN_RIGHT ? "right" :
					    "unknown");
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	int n = -1;
	u_int32_t id;
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;
	const char* alignstr = str;
	while (*str) {
 		u_int32_t align = 0;
		if (strncasecmp(str, "left", 4) == 0) {
			align |= SNOWMIX_ALIGN_LEFT; str += 4;
		} else if (strncasecmp(str, "center", 6) == 0) {
			align |= SNOWMIX_ALIGN_CENTER; str += 6;
		} else if (strncasecmp(str, "right", 5) == 0) {
			align |= SNOWMIX_ALIGN_RIGHT; str += 5;
		} else if (strncasecmp(str, "top", 3) == 0) {
			align |= SNOWMIX_ALIGN_TOP; str += 3;
		} else if (strncasecmp(str, "middle", 6) == 0) {
			align |= SNOWMIX_ALIGN_MIDDLE; str += 6;
		} else if (strncasecmp(str, "bottom", 6) == 0) {
			align |= SNOWMIX_ALIGN_BOTTOM; str += 6;
		} else return 1;	       // parameter error
		if ((n = SetFeedAlign(id, align))) break;
		while (isspace(*str)) str++;
	}
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u align %s\n", id, alignstr);
	return n;
}

// vfeed anchor [<vfeed id> (n | s | e | w | c | ne | nw | se | sw)]
// Set virtual feed anchor
int CVirtualFeed::set_virtual_feed_anchor(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
				"%2d anchor %d,%d\n", id,
				m_feeds[id]->anchor_x, m_feeds[id]->anchor_y);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	u_int32_t id;
	char anchor[3];
	anchor[0] = '\0';
	if (sscanf(str, "%u %2["ANCHOR_TAGS"]", &id, &anchor[0]) != 2) return -1;
	int n = SetFeedAnchor(id, anchor);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u anchor %s\n", id, anchor);
	return n;
}

// vfeed coor [<vfeed id> <x> <y>]
// Set virtual feed coor
int CVirtualFeed::set_virtual_feed_coor(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"coor %2d %d %d\n", id, m_feeds[id]->x, m_feeds[id]->y);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	u_int32_t id;
	int32_t x, y;
	if (sscanf(str, "%u %d %d", &id, &x, &y) != 3) return -1;
	int n = SetFeedCoordinates(id, x, y);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u coordinates set to %d,%d\n", id, x, y);
	return n;
}

// vfeed offset [<vfeed id> <offset x> <offset y>]
// Set virtual feed coor
int CVirtualFeed::set_virtual_feed_offset(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"coor %2d %d %d\n", id, m_feeds[id]->offset_x,
					m_feeds[id]->offset_y);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	u_int32_t id;
	int32_t offset_x, offset_y;
	if (sscanf(str, "%u %d %d", &id, &offset_x, &offset_y) != 3) return -1;
	int n = SetFeedOffset(id, offset_x, offset_y);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u offset %d,%d\n", id, offset_x, offset_y);
	return n;
}

// vfeed clip [<vfeed id> <clip w> <clip h> <clip x> <clip y>]
// Set virtual feed clip
int CVirtualFeed::set_virtual_feed_clip(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
				"clip %2d %ux%u %u,%u\n", id, m_feeds[id]->clip_w,
				m_feeds[id]->clip_h, m_feeds[id]->clip_x, m_feeds[id]->clip_y);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	u_int32_t id, clip_w, clip_h, clip_x, clip_y;
	if (sscanf(str, "%u %u %u %u %u", &id, &clip_w, &clip_h, &clip_x, &clip_y) != 5) return -1;
	int n = SetFeedClip(id, clip_x, clip_y, clip_w, clip_h);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u clip from %u,%u %ux%u\n", id, clip_x, clip_y, clip_w, clip_h);
	return n;
}

// vfeed geometry [ <vfeed id> <width> <height> ]
// Set virtual feed geometry
int CVirtualFeed::set_virtual_feed_geometry(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	if (!m_feeds || !m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
				"geometry %2d %ux%u\n", id, m_feeds[id]->width,
				m_feeds[id]->height);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	u_int32_t id;
	if (sscanf(str, "%u", &id) == 1 && m_feeds[id]) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
				"geometry %2d %ux%u\n", id, m_feeds[id]->width,
				m_feeds[id]->height);
		return 0;
	}
	return -1;

	// You can not set the geometry as it is defined by the source for the vfeed
/*
	return -1;

	u_int32_t width, height;
	if (sscanf(str, "%u %u %u", &id, &width, &height) != 5) return -1;
	int n = SetFeedGeometry(id, width, height);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u geometry %ux%u\n", id, width, height);
	return n;
*/
}


// vfeed rotation [<vfeed id> <rotation>]
// Set virtual feed rotation
int CVirtualFeed::set_virtual_feed_rotation(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"rotation %2d %.4lf\n", id, m_feeds[id]->rotation);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	double rotation;
	u_int32_t id;
	int n = -1;

	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (NumberWithPI(str, &rotation)) {
		n = SetFeedRotation(id, rotation);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u rotation %.5lf\n", id, rotation);
	}
	return n;
}


// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_alpha(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"move alpha %2d delta %.5lf steps %u\n", id,
					m_feeds[id]->delta_alpha, m_feeds[id]->delta_alpha_steps);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	// place rect <vir id> <delta_alpha> <step_alpha>
	double delta_alpha;
	u_int32_t id, step_alpha;
	int32_t n = -1;
	if (sscanf(str, "%u %lf %u", &id, &delta_alpha, &step_alpha) == 3) {
		n = SetFeedAlphaDelta(id, delta_alpha, step_alpha);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u move alpha %.5lf steps %u\n",
				id, delta_alpha, step_alpha);
	}
	return n;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_rotation(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"move rotation %2d delta %.5lf steps %u\n", id,
					m_feeds[id]->delta_rotation, m_feeds[id]->delta_rotation_steps);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	// place rect <vir id> <delta_rotation> <step_rotation>
	double delta_rotation;
	u_int32_t id, step_rotation;
	int32_t n = -1;
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!NumberWithPI(str, &delta_rotation)) return -1;
	while (*str && !(isspace(*str))) str++;
	while (isspace(*str)) str++;
	if (sscanf(str, "%u", &step_rotation) == 1) {
		n = SetFeedRotationDelta(id, delta_rotation, step_rotation);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u move rotation %.5lf steps %u\n",
				id, delta_rotation, step_rotation);
	}
	return n;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_scale(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"move scale %2d delta %.5lf,%.5lf steps %u,%u\n", id,
					m_feeds[id]->delta_scale_x, m_feeds[id]->delta_scale_y,
					m_feeds[id]->delta_scale_x_steps,
					m_feeds[id]->delta_scale_y_steps);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	// place rect <vir id> <scale_x> <scale_y> <step_x> <step_y>
	double delta_scale_x, delta_scale_y;
	u_int32_t id, step_x, step_y;
	int n = -1;
	if (sscanf(str, "%u %lf %lf %u %u", &id, &delta_scale_x, &delta_scale_y, &step_x, &step_y) == 5) {
		n = SetFeedScaleDelta(id, delta_scale_x, delta_scale_y, step_x, step_y);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u move scale %.5lf %.5lf steps %u,%u\n",
				id, delta_scale_x, delta_scale_y, step_x, step_y);
	}
	return n;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_coor(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"move scale %2d delta %.5lf,%.5lf steps %u,%u\n", id,
					m_feeds[id]->delta_x, m_feeds[id]->delta_y,
					m_feeds[id]->delta_x_steps,
					m_feeds[id]->delta_y_steps);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	int32_t delta_x, delta_y, n = -1;
	u_int32_t id, step_x, step_y;
	if (sscanf(str, "%u %d %d %u %u", &id, &delta_x, &delta_y, &step_x, &step_y) == 5) {
		n = SetFeedCoordinatesDelta(id, delta_x, delta_y, step_x, step_y);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u move coor %d,%d steps %u,%u\n",
				id, delta_x, delta_y, step_x, step_y);
	}
	return n;
}

// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_move_offset(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: vfeed "
					"move offset %2d delta %.5lf,%.5lf steps %u,%u\n", id,
					m_feeds[id]->delta_offset_x, m_feeds[id]->delta_offset_y,
					m_feeds[id]->delta_offset_x_steps,
					m_feeds[id]->delta_offset_y_steps);
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	int32_t delta_offset_x, delta_offset_y, n = -1;
	u_int32_t id, step_offset_x, step_offset_y;
	if (sscanf(str, "%u %d %d %u %u", &id, &delta_offset_x, &delta_offset_y,
		&step_offset_x, &step_offset_y) == 5) {
		n = SetFeedOffsetDelta(id, delta_offset_x, delta_offset_y, step_offset_x, step_offset_y);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u move coor %d,%d steps %u,%u\n",
				id, delta_offset_x, delta_offset_y, step_offset_x, step_offset_y);
	}
	return n;
}

// place rect <vir id> [ <x> <y> <clip w> <clip h> <clip x> <clip y> [ <rotation> <scale x> <scale y> <alpha> ] ]
// Set virtual feed place params
int CVirtualFeed::set_virtual_feed_place_rect(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		for (unsigned int id = 0; id < MaxFeeds(); id++) {
			if (m_feeds[id]) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"MSG: vfeed %u place rect %d,%d clip %ux%u %u,%u "
				"rot %.5lf scale %.4lf,%.4lf alpha %.4lf %s %s\n", id,
				m_feeds[id]->x, m_feeds[id]->y,
				m_feeds[id]->clip_w, m_feeds[id]->clip_h,
				m_feeds[id]->clip_x, m_feeds[id]->clip_y,
				m_feeds[id]->rotation,
				m_feeds[id]->scale_x, m_feeds[id]->scale_y,
				m_feeds[id]->alpha,
				m_feeds[id]->align & SNOWMIX_ALIGN_TOP ? "top" :
				  m_feeds[id]->align & SNOWMIX_ALIGN_MIDDLE ? "middle" :
				    m_feeds[id]->align & SNOWMIX_ALIGN_BOTTOM ? "bottom" :
				    "unknown",
				m_feeds[id]->align & SNOWMIX_ALIGN_LEFT ? "left" :
				  m_feeds[id]->align & SNOWMIX_ALIGN_CENTER ? "center" :
				    m_feeds[id]->align & SNOWMIX_ALIGN_RIGHT ? "right" :
				    "unknown");
			}
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	int32_t x, y, n;
	u_int32_t id, clip_x, clip_y, clip_w, clip_h;
	double rotation, scx, scy, alpha;
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;

	// Is this the delete vfeed command ?
	if (!(*str)) {
		n = DeleteFeed(id);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u deleted\n", id);
		return n;
	}
		
	if (sscanf(str, "%d %d %u %u %u %u", &x, &y, &clip_w, &clip_h, &clip_x, &clip_y) != 6)
		return -1;
	for (int i=0 ; i<5 ; i++) { while (!(isspace(*str))) str++; while (isspace(*str)) str++; }
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	if (!(*str)) {
		n = PlaceFeed(id, &x, &y, &clip_x, &clip_y, &clip_w, &clip_h);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u place at %u,%u from %u,%u %ux%u\n", id,
				x, y, clip_x, clip_y, clip_w, clip_h);
		return n;
	}
	if (!NumberWithPI(str, &rotation)) return -1;
	while (*str && !(isspace(*str))) str++;
	while (isspace(*str)) str++;
	if (sscanf(str, "%lf %lf %lf", &scx, &scy, &alpha) != 3) return -1;
	n = PlaceFeed(id, &x, &y, &clip_x, &clip_y, &clip_w, &clip_h, &rotation, &scx, &scy, &alpha);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"vfeed %u place at %u,%u from %u,%u %ux%u rotation "
			"%.5lf scale %.5lf,%.5lf alpah %.5lf\n", id,
			x, y, clip_x, clip_y, clip_w, clip_w, rotation, scx, scy, alpha);
	return n;
}

// Set virtual feed source
int CVirtualFeed::set_virtual_feed_source(struct controller_type* ctr, const char* str)
{
	if (!str || !m_pVideoMixer || !m_pVideoMixer->m_pController) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		for (u_int32_t id=0; id < m_max_feeds; id++) if (m_feeds[id]) {
			//feed_type* feed = NULL;
			feed_state_enum state = UNDEFINED;

			// Get the state from feed if we have a feed.
			if (m_feeds[id]->source_type == VIRTUAL_FEED_FEED) {
				feed_state_enum* pState = m_pVideoMixer->m_pVideoFeed->FeedState(m_feeds[id]->source_id);
				if (pState) state = *pState;
			} else if (m_feeds[id]->source_type == VIRTUAL_FEED_IMAGE)
				state = NO_FEED;
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"vfeed %u : source %s %u %ux%u %s\n",
				id, m_feeds[id]->source_type == VIRTUAL_FEED_FEED ? "feed" :
				  m_feeds[id]->source_type == VIRTUAL_FEED_IMAGE ? "image" : "unknown",
				m_feeds[id]->source_id,
				m_feeds[id]->width,
				m_feeds[id]->height,
				m_feeds[id]->source_type == VIRTUAL_FEED_IMAGE ? "STILL" : feed_state_string(state));

		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"\n");
		return 0;
	}
	int vir_id, source_id, n = -1;

	if (sscanf(str, "feed %u %u", &vir_id, &source_id) == 2) {
		if (!m_pVideoMixer->m_pVideoFeed) {
			fprintf(stderr, "pVideoFeed is NULL for set_virtual_feed_source\n");
			return -1;
		}

		n = SetVirtualSource(vir_id, source_id, true);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u source feed %u\n", vir_id, source_id);
	} else if (sscanf(str, "image %u %u", &vir_id, &source_id) == 2) {
		if (!m_pVideoMixer->m_pVideoImage) {
			fprintf(stderr, "pVideoImageis NULL for set_virtual_feed_source\n");
			return -1;
		}
		n = SetVirtualSource(vir_id, source_id, false);
		if (m_verbose && !n)
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"vfeed %u source image %u\n", vir_id, source_id);
	}
	return n;
}

int CVirtualFeed::CreateFeed(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds) return -1;
	virtual_feed_t* p = m_feeds[id];
	if (p) m_feed_count--;
	else p = (virtual_feed_t*) calloc(sizeof(virtual_feed_t),1);
	if (!p) return -1;
	m_feeds[id]		= p;
	p->name			= NULL;
	p->id			= 0;
	p->source_id		= 0;
	p->source_type		= VIRTUAL_FEED_NONE;
	p->width		= 0;
	p->height		= 0;
	p->scale_x		= 1.0;
	p->scale_y		= 1.0;
	p->x			= 0;
	p->y			= 0;
	p->offset_x		= 0;
	p->offset_y		= 0;
	p->anchor_x		= 0;		// anchor nw
	p->anchor_y		= 0;		// anchor nw
	p->align		= SNOWMIX_ALIGN_TOP | SNOWMIX_ALIGN_LEFT;
	p->mode			= 0;
	p->clip_x		= 0;
	p->clip_y		= 0;
	p->clip_w		= 0;
	p->clip_h		= 0;
	p->rotation		= 0.0;
	p->alpha		= 1.0;
	p->filter		= CAIRO_FILTER_FAST;
	p->update		= false;

	p->delta_x		= 0;
	p->delta_y		= 0;
	p->delta_offset_x	= 0;
	p->delta_offset_y	= 0;
	p->delta_clip_x		= 0;
	p->delta_clip_y		= 0;
	p->delta_clip_w		= 0;
	p->delta_clip_h		= 0;
	p->delta_scale_x	= 0.0;
	p->delta_scale_x	= 0.0;
	p->delta_rotation	= 0.0;
	p->delta_alpha		= 0.0;

	p->delta_x_steps	= 0;
	p->delta_y_steps	= 0;
	p->delta_offset_x_steps	= 0;
	p->delta_offset_y_steps	= 0;
	p->delta_clip_x_steps	= 0;
	p->delta_clip_y_steps	= 0;
	p->delta_clip_w_steps	= 0;
	p->delta_clip_h_steps	= 0;
	p->delta_scale_x_steps	= 0;
	p->delta_scale_x_steps	= 0;
	p->delta_rotation_steps	= 0;
	p->delta_alpha_steps	= 0;

	p->pMatrix	= NULL;

/*
	p->matrix_xx	= 1.0;
	p->matrix_yx	= 0.0;
	p->matrix_xy	= 0.0;
	p->matrix_yy	= 1.0;
	p->matrix_x0	= 0.0;
	p->matrix_y0	= 0.0;
*/
	m_feed_count++;

	return 0;
}
int CVirtualFeed::DeleteFeed(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feed_count--;
	if (m_feeds[id]->name) free(m_feeds[id]->name);
	if (m_feeds[id]->pMatrix) free(m_feeds[id]->pMatrix);
	if (m_feeds[id]) free(m_feeds[id]);
	m_feeds[id] = NULL;
	return 0;
}

int CVirtualFeed::SetFeedName(u_int32_t id, const char* feed_name)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !feed_name)
		return -1;
	if (m_feeds[id]->name) free(m_feeds[id]->name);
	m_feeds[id]->name = strdup(feed_name);	// name will be freed if feed is deleted. No memory leak
	if (m_feeds[id]->name) trim_string(m_feeds[id]->name);
	return m_feeds[id]->name ? 0 : -1;	// Freed is deleted in deconstructor so no memory leak
}
	
char* CVirtualFeed::GetFeedName(u_int32_t id)
{
	return (id >= m_max_feeds || !m_feeds || !m_feeds[id]) ?
		NULL : m_feeds[id]->name;
}

int CVirtualFeed::SetVirtualSource(u_int32_t id, u_int32_t source_id, bool feed)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	virtual_feed_t* p = m_feeds[id];
	if (feed) {
		if (!m_pVideoMixer->m_pVideoFeed) return -1;
		if (!m_pVideoMixer->m_pVideoFeed->FeedGeometry(source_id, &p->width, &p->height)) return -1;
		p->source_type = VIRTUAL_FEED_FEED;
	} else {
		if (!m_pVideoMixer->m_pVideoImage) return -1;
		image_item_t* pImage = m_pVideoMixer->m_pVideoImage->GetImageItem(source_id);
		if (!pImage) return -1;
		p->source_type = VIRTUAL_FEED_IMAGE;
		p->width = pImage->width;
		p->height = pImage->height;
	}
	p->source_id = source_id;
	return 0;
}

int CVirtualFeed::GetFeedSourceId(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id])
		return -1;
	return m_feeds[id]->source_id;
}
virtual_feed_enum_t CVirtualFeed::GetFeedSourceType(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id])
		return VIRTUAL_FEED_NONE;
	return m_feeds[id]->source_type;
}

int CVirtualFeed::SetFeedScale(u_int32_t id, double scale_x, double scale_y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] ||
		scale_x < 0.0 || scale_y < 0.0) return -1;
	m_feeds[id]->scale_x = scale_x;
	m_feeds[id]->scale_y = scale_y;
	return 0;
}
int CVirtualFeed::GetFeedScale(u_int32_t id, double* scale_x, double* scale_y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] ||
		!scale_x || !scale_y) return -1;
	*scale_x = m_feeds[id]->scale_x;
	*scale_y = m_feeds[id]->scale_y;
	return 0;
}
int CVirtualFeed::SetFeedScaleDelta(u_int32_t id, double delta_scale_x, double delta_scale_y, u_int32_t delta_scale_x_steps, u_int32_t delta_scale_y_steps)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_scale_x = delta_scale_x;
	m_feeds[id]->delta_scale_y = delta_scale_y;
	m_feeds[id]->delta_scale_x_steps = delta_scale_x_steps;
	m_feeds[id]->delta_scale_y_steps = delta_scale_y_steps;
	if (delta_scale_x_steps || delta_scale_y_steps) m_feeds[id]->update = true;
	return 0;
}

int CVirtualFeed::SetFeedRotation(u_int32_t id, double rotation)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->rotation = rotation;
	return 0;
}
int CVirtualFeed::GetFeedRotation(u_int32_t id, double* rotation)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !rotation) return -1;
	*rotation = m_feeds[id]->rotation;
	return 0;
}
int CVirtualFeed::SetFeedRotationDelta(u_int32_t id, double delta_rotation, u_int32_t delta_rotation_steps)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_rotation = delta_rotation;
	m_feeds[id]->delta_rotation_steps = delta_rotation_steps;
	if (delta_rotation_steps) m_feeds[id]->update = true;
	return 0;
}
int CVirtualFeed::GetFeedAlign(u_int32_t id, u_int32_t* align)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !align) return -1;
	*align = m_feeds[id]->align;
	return 0;
}
int CVirtualFeed::SetFeedAlign(u_int32_t id, u_int32_t align)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (align & (SNOWMIX_ALIGN_LEFT | SNOWMIX_ALIGN_CENTER | SNOWMIX_ALIGN_RIGHT)) {
		m_feeds[id]->align = (m_feeds[id]->align &
			(~(SNOWMIX_ALIGN_LEFT | SNOWMIX_ALIGN_CENTER | SNOWMIX_ALIGN_RIGHT))) | align;
	} else if  (align & (SNOWMIX_ALIGN_TOP | SNOWMIX_ALIGN_MIDDLE | SNOWMIX_ALIGN_BOTTOM)) {
		m_feeds[id]->align = (m_feeds[id]->align &
			(~(SNOWMIX_ALIGN_TOP  | SNOWMIX_ALIGN_MIDDLE | SNOWMIX_ALIGN_BOTTOM))) | align;
	}
	return 0;
}
int CVirtualFeed::SetFeedAlpha(u_int32_t id, double alpha)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (alpha > 1.0) alpha = 1.0;
	else if (alpha < 0.0) alpha = 0.0;
	m_feeds[id]->alpha = alpha;
	return 0;
}
int CVirtualFeed::SetFeedAnchor(u_int32_t id, const char* anchor)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !m_pVideoMixer) return -1;
	return SetAnchor(&(m_feeds[id]->anchor_x),
                &(m_feeds[id]->anchor_y),
                anchor, m_pVideoMixer->GetSystemWidth(), m_pVideoMixer->GetSystemHeight());
}

int CVirtualFeed::GetFeedAlpha(u_int32_t id, double* alpha)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !alpha) return -1;
	*alpha = m_feeds[id]->alpha;
	return 0;
}
int CVirtualFeed::GetFeedFilter(u_int32_t id, cairo_filter_t* filter)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] || !filter) return -1;
	*filter = m_feeds[id]->filter;
	return 0;
}
transform_matrix_t* CVirtualFeed::GetFeedMatrix(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] ) return NULL;
	return m_feeds[id]->pMatrix;
}
int CVirtualFeed::SetFeedAlphaDelta(u_int32_t id, double delta_alpha, u_int32_t delta_alpha_steps)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_alpha = delta_alpha;
	m_feeds[id]->delta_alpha_steps = delta_alpha_steps;
	if (delta_alpha_steps) m_feeds[id]->update = true;
	return 0;
}
int CVirtualFeed::PlaceFeed(u_int32_t id, int32_t* x, int32_t* y,
	u_int32_t* clip_x, u_int32_t* clip_y,
	u_int32_t* clip_w, u_int32_t* clip_h,
	double* rotation, double* scale_x, double* scale_y, double* alpha)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	virtual_feed_t* p = m_feeds[id];
	if (x) p->x = *x;
	if (y) p->y = *y;
	if (clip_x) p->clip_x = *clip_x;
	if (clip_y) p->clip_y = *clip_y;
	if (clip_w) p->clip_w = *clip_w;
	if (clip_h) p->clip_h = *clip_h;
	if (rotation) p->rotation = *rotation;
	if (scale_x) p->scale_x = *scale_x;
	if (scale_y) p->scale_y = *scale_y;
	if (alpha) p->alpha = *alpha;
	return 0;
}
int CVirtualFeed::GetFeedGeometry(u_int32_t id, u_int32_t* w, u_int32_t* h)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (w) *w = m_feeds[id]->width;
	if (h) *h = m_feeds[id]->height;
	return 0;
}
int CVirtualFeed::SetFeedCoordinates(u_int32_t id, int32_t x, int32_t y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->x = x;
	m_feeds[id]->y = y;
	return 0;
}
int CVirtualFeed::SetFeedOffset(u_int32_t id, int32_t offset_x, int32_t offset_y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->offset_x = offset_x;
	m_feeds[id]->offset_y = offset_y;
	return 0;
}
int CVirtualFeed::GetFeedOffset(u_int32_t id, int32_t* offset_x, int32_t* offset_y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (offset_x) *offset_x = m_feeds[id]->offset_x;
	if (offset_y) *offset_y = m_feeds[id]->offset_y;
	return 0;
}
int CVirtualFeed::GetFeedCoordinates(u_int32_t id, int32_t* x, int32_t* y)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (x) *x = m_feeds[id]->x;
	if (y) *y = m_feeds[id]->y;
	return 0;
}

int CVirtualFeed::SetFeedCoordinatesDelta(u_int32_t id, int32_t delta_x, int32_t delta_y,
	int32_t steps_x, int32_t steps_y )
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_x = delta_x;
	m_feeds[id]->delta_y = delta_y;
	m_feeds[id]->delta_x_steps = steps_x;
	m_feeds[id]->delta_y_steps = steps_y;
	if (delta_x || delta_y) m_feeds[id]->update = true;
	return 0;
}

int CVirtualFeed::SetFeedOffsetDelta(u_int32_t id, int32_t delta_offset_x, int32_t delta_offset_y,
	int32_t steps_offset_x, int32_t steps_offset_y )
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_offset_x = delta_offset_x;
	m_feeds[id]->delta_offset_y = delta_offset_y;
	m_feeds[id]->delta_offset_x_steps = steps_offset_x;
	m_feeds[id]->delta_offset_y_steps = steps_offset_y;
	if (delta_offset_x || delta_offset_y) m_feeds[id]->update = true;
	return 0;
}

int CVirtualFeed::SetFeedClip(u_int32_t id, u_int32_t clip_x, u_int32_t clip_y,
	u_int32_t clip_w, u_int32_t clip_h)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] ||
		!clip_w || !clip_h) return -1;
	if (clip_w + clip_x > m_feeds[id]->width ||
		clip_h + clip_y > m_feeds[id]->height) return -1;
	m_feeds[id]->clip_x = clip_x;
	m_feeds[id]->clip_y = clip_y;
	m_feeds[id]->clip_w = clip_w;
	m_feeds[id]->clip_h = clip_h;
	return 0;
}

int CVirtualFeed::SetFeedGeometry(u_int32_t id, u_int32_t w, u_int32_t h)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id] ||
		!w || !h) return -1;
	m_feeds[id]->width = w;
	m_feeds[id]->height = h;
	return 0;
}
int CVirtualFeed::GetFeedClip(u_int32_t id,
	u_int32_t* clip_x, u_int32_t* clip_y,
	u_int32_t* clip_w, u_int32_t* clip_h)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (clip_x) *clip_x = m_feeds[id]->clip_x;
	if (clip_y) *clip_y = m_feeds[id]->clip_y;
	if (clip_w) *clip_w = m_feeds[id]->clip_w;
	if (clip_h) *clip_h = m_feeds[id]->clip_h;
	return 0;
}

int CVirtualFeed::SetFeedMatrix(u_int32_t id, double xx, double xy, double yx, double yy, double x0, double y0)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (!m_feeds[id]->pMatrix)
		m_feeds[id]->pMatrix = (transform_matrix_t*)
			calloc(sizeof(transform_matrix_t), 1);
	if (!m_feeds[id]->pMatrix) return 1;			// No memory leak here.
	m_feeds[id]->pMatrix->matrix_xx = xx;			// Matrix is freed when deleting feed.
	m_feeds[id]->pMatrix->matrix_xy = xy;			// Deconstructor deletes all feeds.
	m_feeds[id]->pMatrix->matrix_yx = yx;
	m_feeds[id]->pMatrix->matrix_yy = yy;
	m_feeds[id]->pMatrix->matrix_x0 = x0;
	m_feeds[id]->pMatrix->matrix_y0 = y0;
	return 0;
}

int CVirtualFeed::SetFeedFilter(u_int32_t id, cairo_filter_t filter)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->filter = filter;
	return 0;
}

int CVirtualFeed::SetFeedClipDelta(u_int32_t id,
	int32_t delta_clip_x, int32_t delta_clip_y,
	int32_t delta_clip_w, int32_t delta_clip_h,
	u_int32_t delta_clip_x_steps, u_int32_t delta_clip_y_steps,
	u_int32_t delta_clip_w_steps, u_int32_t delta_clip_h_steps)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	m_feeds[id]->delta_clip_x = delta_clip_x;
	m_feeds[id]->delta_clip_y = delta_clip_y;
	m_feeds[id]->delta_clip_w = delta_clip_w;
	m_feeds[id]->delta_clip_h = delta_clip_h;
	m_feeds[id]->delta_clip_x_steps = delta_clip_x_steps;
	m_feeds[id]->delta_clip_y_steps = delta_clip_y_steps;
	m_feeds[id]->delta_clip_w_steps = delta_clip_w_steps;
	m_feeds[id]->delta_clip_h_steps = delta_clip_h_steps;
	if (delta_clip_x_steps || delta_clip_y_steps || delta_clip_w_steps || delta_clip_h_steps)
		m_feeds[id]->update = true;
	return 0;
}


// overlay vfeed (<id> | <id>..<id> | all | end | <id>..end) [ (<id> | <id>..<id> | all | end | <id>..end) ] ....
int CVirtualFeed::OverlayFeed(CCairoGraphic* pCairoGraphic, const char* str)
{
	//if (!str || !m_overlay) return 1;
	if (!str || !m_pVideoMixer) return 1;
	while (isspace(*str)) str++;
	if (!(*str)) return 1;
	u_int32_t maxid = m_max_feeds ? m_max_feeds - 1 : 0;
	while (*str) {
		u_int32_t from, to;
		const char* nextstr = GetIdRange(str, &from, &to, 0, maxid);
		if (!nextstr) break;
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
		if (from > to) return 1;
		for ( ; from <= to ; from++) {
			// If we don't have a feed, we skip to next
			if (from > maxid || !m_feeds[from]) continue;

			virtual_feed_enum_t feed_type = GetFeedSourceType(from);
			u_int8_t* src_buf = NULL;
			int real_feed_no = GetFeedSourceId(from);
			if (feed_type == VIRTUAL_FEED_FEED) {
				if (!m_pVideoMixer->m_pVideoFeed) {
					fprintf(stderr, "pVideoFeed was NULL for OverlayVirtualFeed\n");
					return 1;
				}
				src_buf = m_pVideoMixer->m_pVideoFeed->GetFeedFrame(real_feed_no);
			} else if (feed_type == VIRTUAL_FEED_IMAGE) {
				if (!m_pVideoMixer->m_pVideoImage) {
					fprintf(stderr, "pVideoImage was NULL for OverlayVirtualFeed\n");
					return 1;
				}
				image_item_t* pImage = m_pVideoMixer->m_pVideoImage->GetImageItem(real_feed_no);
				if (pImage) src_buf = pImage->pImageData;
			}
			if (!src_buf) return 1;
			if (pCairoGraphic) {
				virtual_feed_t* p = m_feeds[from];
				if (!p->mode) {
					pCairoGraphic->OverlayFrame(src_buf,
						p->width, p->height,
		       				p->x + p->anchor_x, p->y + p->anchor_y,
						p->offset_x, p->offset_y,
						p->clip_w, p->clip_h, p->clip_x, p->clip_y,
						p->align, p->scale_x , p->scale_y,
						p->rotation, p->alpha,
						p->filter, p->pMatrix);
				} else pCairoGraphic->OverlayVirtualFeed(m_pVideoMixer, m_feeds[from]);
		 		UpdateMove(from);
			}
		}
	}
	return 0;
}

int CVirtualFeed::UpdateMove(u_int32_t id)
{
	if (id >= m_max_feeds || !m_feeds || !m_feeds[id]) return -1;
	if (!m_feeds[id]->update) return 0;
        bool changed = false;

	if (m_feeds[id]->delta_x_steps) {
		m_feeds[id]->delta_x_steps--;
		m_feeds[id]->x += m_feeds[id]->delta_x;
		changed = true;
	}
	if (m_feeds[id]->delta_y_steps) {
		m_feeds[id]->delta_y_steps--;
		m_feeds[id]->y += m_feeds[id]->delta_y;
		changed = true;
	}
	if (m_feeds[id]->delta_offset_x_steps) {
		m_feeds[id]->delta_offset_x_steps--;
		m_feeds[id]->offset_x += m_feeds[id]->delta_offset_x;
		changed = true;
	}
	if (m_feeds[id]->delta_offset_y_steps) {
		m_feeds[id]->delta_offset_y_steps--;
		m_feeds[id]->offset_y += m_feeds[id]->delta_offset_y;
		changed = true;
	}
	if (m_feeds[id]->delta_clip_x_steps) {
		m_feeds[id]->delta_clip_x_steps--;
		int32_t x = m_feeds[id]->clip_x + m_feeds[id]->delta_clip_x;
		if (x + m_feeds[id]->clip_w <= m_feeds[id]->width) {
			x >= 0 ? m_feeds[id]->clip_x = x : m_feeds[id]->clip_x = 0;
		}
		changed = true;
	}
	if (m_feeds[id]->delta_clip_y_steps) {
		m_feeds[id]->delta_clip_y_steps--;
		int32_t y = m_feeds[id]->clip_y + m_feeds[id]->delta_clip_y;
		if (y + m_feeds[id]->clip_h <= m_feeds[id]->height) {
			y >= 0 ? m_feeds[id]->clip_y = y : m_feeds[id]->clip_y = 0;
		}
		changed = true;
	}
	if (m_feeds[id]->delta_clip_w_steps) {
		m_feeds[id]->delta_clip_w_steps--;
		int32_t w = m_feeds[id]->clip_w + m_feeds[id]->delta_clip_w;
		if (w + m_feeds[id]->clip_x < m_feeds[id]->width) {
			w >= 0 ? m_feeds[id]->clip_w = w : m_feeds[id]->clip_w = 0;
		}
		changed = true;
	}
	if (m_feeds[id]->delta_clip_h_steps) {
		m_feeds[id]->delta_clip_h_steps--;
		int32_t h = m_feeds[id]->clip_h + m_feeds[id]->delta_clip_h;
		if (h + m_feeds[id]->clip_y < m_feeds[id]->height) {
			h >= 0 ? m_feeds[id]->clip_h = h : m_feeds[id]->clip_h = 0;
		}
		changed = true;
	}
	if (m_feeds[id]->delta_scale_x_steps) {
		m_feeds[id]->delta_scale_x_steps--;
		double scale = m_feeds[id]->scale_x + m_feeds[id]->delta_scale_x;
		if (scale > 0.0) m_feeds[id]->scale_x = scale;
		changed = true;
	}
	if (m_feeds[id]->delta_scale_y_steps) {
		m_feeds[id]->delta_scale_y_steps--;
		double scale = m_feeds[id]->scale_y + m_feeds[id]->delta_scale_y;
		if (scale > 0.0) m_feeds[id]->scale_y = scale;
		changed = true;
	}
	if (m_feeds[id]->delta_rotation_steps) {
		m_feeds[id]->delta_rotation_steps--;
		m_feeds[id]->rotation += m_feeds[id]->delta_rotation;
		if (m_feeds[id]->rotation > 2*M_PI) m_feeds[id]->rotation -= 2*M_PI;
		if (m_feeds[id]->rotation < -2*M_PI) m_feeds[id]->rotation += 2*M_PI;
		changed = true;
	}
	if (m_feeds[id]->delta_alpha_steps) {
		m_feeds[id]->delta_alpha_steps--;
		double alpha = m_feeds[id]->alpha + m_feeds[id]->delta_alpha;
		if (alpha < 0.0) m_feeds[id]->alpha = 0.0;
		else if (alpha > 1.0) m_feeds[id]->alpha = 1.0;
		else m_feeds[id]->alpha = alpha;
		changed = true;
	}
	// If nothing changed, we set update to false to save future update runs
	// until a change is made to the move parameters again.
	m_feeds[id]->update = changed;
    	return 0;
}
