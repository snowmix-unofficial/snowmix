/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <ctype.h>
#include <stdio.h>
//#include <limits.h>
#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <stdlib.h>
#ifdef HAVE_PNG16
#include <libpng16/png.h>
#else
#ifdef HAVE_PNG15
#include <libpng12/png.h>
#else
#ifdef HAVE_PNG12
#include <libpng12/png.h>
#else
#include <png.h>
#endif
#endif
#endif

#ifdef WIN32
#define _USE_MATH_DEFINES
#include <io.h>
#include "windows_util.h"
#endif
#include <math.h>
//#include <string.h>


#include "snowmix.h"
#include "snowmix_util.h"
#include "video_image.h"
#include "cairo_graphic.h"

CVideoImage::CVideoImage(CVideoMixer* pVideoMixer, u_int32_t max_images, u_int32_t max_images_places)
{
	m_image_items = NULL;
	m_image_places = NULL;
	m_verbose = 0;
	m_image_items = (struct image_item_t**) calloc(max_images, sizeof(struct image_item_t*) + sizeof(u_int32_t));
	m_image_places = (struct image_place_t**) calloc(max_images_places, sizeof(struct image_place_t*) + sizeof(u_int32_t));
	m_max_images = m_image_items ? max_images : 0;
	m_max_image_places = m_image_places ? max_images_places : 0;
	m_seqno_images = (u_int32_t*) (m_image_items ? &(m_image_items[m_max_images]) : NULL);
	m_seqno_image_places = (u_int32_t*) (m_image_places ? &(m_image_places[m_max_image_places]) : NULL);
	m_pVideoMixer = pVideoMixer;
	//m_width = m_pVideoMixer ? m_pVideoMixer->GetSystemWidth() : 0;
	//m_height = m_pVideoMixer ? m_pVideoMixer->GetSystemHeight() : 0;
	if (!m_pVideoMixer) fprintf(stderr, "Class CVideoImage created with pVideoMixer NULL. Will fail\n");
}

CVideoImage::~CVideoImage()
{
	if (m_image_items) {
		for (unsigned int id=0; id < m_max_images; id++) DeleteImageItem(id);
		free(m_image_items);
		m_image_items = NULL;
	}
	if (m_image_places) {
		for (unsigned int id=0; id < m_max_image_places; id++) {
			if (m_image_places[id] && m_image_places[id]->pMatrix)
				free(m_image_places[id]->pMatrix);
			if (m_image_places[id]) free(m_image_places[id]);
			m_image_places[id] = NULL;
		}
		free(m_image_places);
		m_image_places = NULL;
	}
}

int CVideoImage::ImageItemSeqNo(u_int32_t image_id) {
	return m_seqno_images && image_id < m_max_images ? m_seqno_images[image_id] : 0;
}

int CVideoImage::ImagePlacedSeqNo(u_int32_t place_id) {
	return m_seqno_image_places && place_id < m_max_image_places ? m_seqno_image_places[place_id] : 0;
}

void CVideoImage::DeleteImageItem(u_int32_t image_id) {
	if (!m_image_items || !m_image_items[image_id]) return;
	if (m_image_items[image_id]->name) free(m_image_items[image_id]->name);
	if (m_image_items[image_id]->row_pointers) {
		for (unsigned int y=0; y<m_image_items[image_id]->height; y++)
			if (m_image_items[image_id]->row_pointers[y])
				free(m_image_items[image_id]->row_pointers[y]);
		free(m_image_items[image_id]->row_pointers);
	}
	if (m_image_items[image_id]->png_ptr) {
fprintf(stderr, "Destroy image %u\n", image_id);
		png_destroy_read_struct(&(m_image_items[image_id]->png_ptr),
			m_image_items[image_id]->info_ptr ? &(m_image_items[image_id]->info_ptr) : NULL, NULL);
	}
	if (m_image_items[image_id]->pImageData) free(m_image_items[image_id]->pImageData);
	free(m_image_items[image_id]);
	m_image_items[image_id] = NULL;
}
//PMM13
/*
void CVideoImage::DeleteImageItem(u_int32_t image_id) {
	if (!m_image_items || !m_image_items[image_id]) return;
	if (m_image_items[image_id]->name) free(m_image_items[image_id]->name);
	if (m_image_items[image_id]->row_pointers) {
		for (unsigned int y=0; y<m_image_items[image_id]->height; y++)
			if (m_image_items[image_id]->row_pointers[y])
				free(m_image_items[image_id]->row_pointers[y]);
		free(m_image_items[image_id]->row_pointers);
	}
	if (m_image_items[image_id]->pImageData) free(m_image_items[image_id]->pImageData);
	free(m_image_items[image_id]);
	m_image_items[image_id] = NULL;
}
*/

int CVideoImage::ParseCommand(CController* pController, struct controller_type* ctr,
	const char* str)
{
	while (isspace(*str)) str++;
	if (!strncasecmp (str, "overlay ", 8)) {
		if (m_pVideoMixer) return m_pVideoMixer->OverlayImage(str + 8);
		else return 1;
	}
	// image align [<id no> (left | center | right) (top | middle | bottom)]
	else if (!strncasecmp (str, "align ", 6)) {
		return set_image_place_align(ctr, pController, str+6);
	}
	// image alpha [<id no> <alpha>]
	else if (!strncasecmp (str, "alpha ", 6)) {
		return set_image_place_alpha(ctr, pController, str+6);
	}
	// image anchor [<id no> (n | s | e | w | c | ne | nw | se | sw)]
	else if (!strncasecmp (str, "anchor ", 7)) {
		return set_image_place_anchor(ctr, pController, str+7);
	}
	// image clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
	else if (!strncasecmp (str, "clip ", 5)) {
	 	return set_image_place_clip (ctr, pController, str+5);
	}
	// image coor [<place id>]
	else if (!strncasecmp (str, "coor ", 5)) {
		return set_image_place(ctr, pController, str);	// The length of 'coor ' is not added to str.
	}
	// image filter [<place id> (fast | good | best | nearest | bilinear | gaussian)]
	else if (!strncasecmp (str, "filter ", 7)) {
	 	return set_image_place_filter (ctr, pController, str+7);
	}
	// image geometry [<place id>]
	else if (!strncasecmp (str, "geometry ", 9)) {
		return set_image_geometry(ctr, pController, str+9);
	}
	// image image [<place id> <image id>]
	else if (!strncasecmp (str, "image ", 6)) {
		return set_image_place_image(ctr, pController, str+6);
	}
	// image place matrix [<id no> <xx> <xy> <yx> <yy> <x0> <y0>]
	else if (!strncasecmp (str, "matrix ", 7)) {
		return set_image_place_matrix(ctr, pController, str+7);
	}
	else if (!strncasecmp (str, "move ", 5)) {
		str += 5;
		while (isspace(*str)) str++;
		// image move alpha [<place id> <delta_alpha> <step_alpha>]
		if (!strncasecmp (str, "alpha ", 6)) {
			return set_image_place_move_alpha (ctr, pController, str+6);
		}
		// image place move clip [<id no> <delta left> <delta right> <delta top>
		//    <delta bottom> <left steps> <right steps> <top steps> <bottom steps>]
		else if (!strncasecmp (str, "clip ", 5)) {
			return set_image_place_move_clip (ctr, pController, str+5);
		}
		// image place move coor [<place id> <delta_x> <delta_y> <step_x> <step_y>]
		else if (!strncasecmp (str, "coor ", 5)) {
			return set_image_place_move_coor (ctr, pController, str+5);
		}
		// image place move offset [<place id> <delta_offset_x> <delta_offset_y> <step_offset_x> <step_offset_y>]
		else if (!strncasecmp (str, "offset ", 7)) {
			return set_image_place_move_offset (ctr, pController, str+7);
		}
		// image place move rotation [<place id> <delta_rotation> <step_rotation>]
		else if (!strncasecmp (str, "rotation ", 9)) {
			return set_image_place_move_rotation (ctr, pController, str+9);
		}
		// image place move scale [<place id> <delta_scale_x> <delta_scale_y>
		//      <step_x> <step_y>]
		else if (!strncasecmp (str, "scale ", 6)) {
			return set_image_place_move_scale (ctr, pController, str+6);
		}
		return 1;
	}
	// image place offset [<place id> <offset x> <offset y>]
	else if (!strncasecmp (str, "offset ", 7)) {
		return set_image_place_offset (ctr, pController, str+7);
	}
	else if (!strncasecmp (str, "place ", 6)) {
		str += 6;
		while (isspace(*str)) str++;
		// image place align [<id no> (left | center | right) (top | middle | bottom)]
//		if (!strncasecmp (str, "align ", 6)) {
//			return set_image_place_align(ctr, pController, str+6);
//		}
//		// image place alpha [<id no> <alpha>]
//		else if (!strncasecmp (str, "alpha ", 6)) {
//			return set_image_place_alpha(ctr, pController, str+6);
//		}
//		// image place anchor [<id no> (n | s | e | w | c | ne | nw | se | sw)]
//		else if (!strncasecmp (str, "anchor ", 7)) {
//			return set_image_place_anchor(ctr, pController, str+7);
//		}
//		// image place clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
//		else if (!strncasecmp (str, "clip ", 5)) {
//		 	return set_image_place_clip (ctr, pController, str+5);
//		}
//		// image place filter [<place id> (fast | good | best | nearest | bilinear | gaussian)]
//		else if (!strncasecmp (str, "filter ", 7)) {
//		 	return set_image_place_filter (ctr, pController, str+7);
//		}
//		else if (!strncasecmp (str, "move ", 5)) {
//			str += 5;
//			while (isspace(*str)) str++;
//			// image place move alpha [<place id> <delta_alpha> <step_alpha>]
//			if (!strncasecmp (str, "alpha ", 6)) {
//				return set_image_place_move_alpha (ctr, pController, str+6);
//			}
//			// image place move clip [<id no> <delta left> <delta right> <delta top>
//			//    <delta bottom> <left steps> <right steps> <top steps> <bottom steps>]
//			else if (!strncasecmp (str, "clip ", 5)) {
//				return set_image_place_move_clip (ctr, pController, str+5);
//			}
//			// image place move scale [<place id> <delta_scale_x> <delta_scale_y>
//			//      <step_x> <step_y>]
//			else if (!strncasecmp (str, "scale ", 6)) {
//				return set_image_place_move_scale (ctr, pController, str+6);
//			}
//			// image place move coor [<place id> <delta_x> <delta_y> <step_x> <step_y>]
//			else if (!strncasecmp (str, "coor ", 5)) {
//				return set_image_place_move_coor (ctr, pController, str+5);
//			}
//			// image place move offset [<place id> <delta_offset_x> <delta_offset_y> <step_offset_x> <step_offset_y>]
//			else if (!strncasecmp (str, "offset ", 7)) {
//				return set_image_place_move_offset (ctr, pController, str+7);
//			}
//			// image place move rotation [<place id> <delta_rotation> <step_rotation>]
//			else if (!strncasecmp (str, "rotation ", 9)) {
//				return set_image_place_move_rotation (ctr, pController, str+9);
//			}
//			return 1;
//		}
//		// image place offset [<place id> <offset x> <offset y>]
//		else if (!strncasecmp (str, "offset ", 7)) {
//			return set_image_place_offset (ctr, pController, str+7);
//		}
//		// image place image [<place id> <image id>]
//		else if (!strncasecmp (str, "image ", 6)) {
//			return set_image_place_image(ctr, pController, str+6);
//		}
//		// image place coor [<id no> <>]
//		else if (!strncasecmp (str, "coor ", 5)) {
//			return set_image_place(ctr, pController, str);
//		}
//		// image place rotation [<id no> <rotation>]
//		else if (!strncasecmp (str, "rotation ", 9)) {
//			return set_image_place_rotation(ctr, pController, str+9);
//		}
//		// image place scale [<id no> <scale_x> <scale_y>]
//		else if (!strncasecmp (str, "scale ", 6)) {
//			return set_image_place_scale(ctr, pController, str+6);
//		}
//		// image place matrix [<id no> <xx> <xy> <yx> <yy> <x0> <y0>]
//		else if (!strncasecmp (str, "matrix ", 7)) {
//			return set_image_place_matrix(ctr, pController, str+7);
//		}
		// image place [<id no> [<image id> <x> <y> [(n | s | e | w | ne | nw | se | sw)] [(left | center | right) (top | middle | bottom)]
		//else
			return set_image_place (ctr, pController, str);
	}
	// image rotation [<id no> <rotation>]
	else if (!strncasecmp (str, "rotation ", 9)) {
		return set_image_place_rotation(ctr, pController, str+9);
	}
	// image scale [<id no> <scale_x> <scale_y>]
	else if (!strncasecmp (str, "scale ", 6)) {
		return set_image_place_scale(ctr, pController, str+6);
	}
	// image source [(feed | image) <image id> (<feed id> | <image id>) [ <offset x> <offset y> <width> <height> [<scale_x> <scale_y> [ <rotation> <alpha> [(fast | good | best | nearest | bilinear | gaussian)]]]]]
	else if (!strncasecmp (str, "source ", 7)) {
		return set_image_source(ctr, pController, str+7);
	}
	// image load [<image id> [<file name>]]  // empty file name deletes entry
	else if (!strncasecmp (str, "load ", 5)) {
		return set_image_load (ctr, pController, str+5);
	}
	// image name [<id no> [<name>]]  // empty name deletes name
	else if (!strncasecmp (str, "name ", 5)) {
		return set_image_name (ctr, pController, str+5);
	}
	// image write <id no> <file name>
	else if (!strncasecmp (str, "write ", 6)) {
		return set_image_write (ctr, pController, str+6);
	}
	// image help
	else if (!strncasecmp (str, "help ", 5)) {
		return set_image_help (ctr, pController, str+5);
	}
	// image verbose
	else if (!strncasecmp (str, "verbose ", 8)) {
		return set_verbose (ctr, pController, str+8);
	} else return 1;
	return 0;
}


// vfeed verbose [<level>] 
int CVideoImage::set_verbose(struct controller_type* ctr, CController* pController, const char* str)
{
	int n;
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_verbose) m_verbose = 0;
		else m_verbose = 1;
		if (pController)
			pController->controller_write_msg (ctr,
				STATUS"image verbose %s\n", m_verbose ? "on" : "off");
		return 0;
	}
	while (isspace(*str)) str++;
	n = sscanf(str, "%u", &m_verbose);
	if (n != 1) return -1;
	if (m_verbose && pController)
		pController->controller_write_msg (ctr,
			MESSAGE"image verbose level set to %u\n",
			m_verbose);
	return 0;
}



int CVideoImage::set_image_help(struct controller_type* ctr, CController* pController, const char* str)
{
	if (!pController || !str) return -1;
	pController->controller_write_msg (ctr, "MSG: Commands:\n"
			"MSG:  image align [<place id> (left | center | right) "
				"(top | middle | bottom)]\n"
			"MSG:  image alpha [<place id> <alpha>]\n"
			"MSG:  image anchor [<place id> "ANCHOR_SYNTAX"]\n"
			"MSG:  image clip [<place id> <clip left> <clip right> "
				"<clip top> <clip bottom>]\n"
			"MSG:  image coor [<place id> <coor x> <coor y>]\n"
			"MSG:  image filter [<place id> (fast | good | best | nearest | "
				"bilinear | gaussian)]\n"
			"MSG:  image geometry [<place id>]\n"
			"MSG:  image image [<place id> <image id>]\n"
			"MSG:  image load [<image id> [<file name>]]  // empty string "
				"deletes entry\n"
			"MSG:  image matrix [<place id> <xx> <xy> <yx> <yy> <x0> <y0>]\n"
			"MSG:  image move alpha [<place id> <delta_alpha> <step_alpha>]\n"
			"MSG:  image move clip [<place id> <delta left> <delta right> "
				"<delta top> <delta bottom> <left steps> <right steps> "
				"<top steps> <bottom steps>]\n"
			"MSG:  image move coor [<place id> <delta_x> <delta_y> "
				"<step_x> <step_y>]\n"
			"MSG:  image move offset [<place id> <delta_offset_x> "
				"<delta_offset_y> <step_offset_x> <step_offset_y>]\n"
			"MSG:  image move rotation [<place id> <delta_rotation> "
				"<step_rotation>]\n"
			"MSG:  image move scale [<place id> <delta_scale_x> <delta_scale_y> "
				"<step_x> <step_y>]\n"
			"MSG:  image name [<image id> [<name>]]  // empty string deletes name\n"
			"MSG:  image offset [<place id> <offset x> <offset y>]\n"
			"MSG:  image overlay (<place id> | <place id>..<place id> | all | end "
				"| <place id>..end) [ ... ] \n"
			"MSG:  image place [<place id> [<image id> <x> <y> ["ANCHOR_SYNTAX"] "
				"[(left | center | right) (top | middle | bottom)]]]\n"
			"MSG:  image rotation [<place id> <rotation>]\n"
			"MSG:  image scale [<place id> <scale_x> <scale_y>]\n"
			"MSG:  image source [(feed | image | frame) <image id> <source id> [ <offset x> <offset y> <width> height> [<scale_x> <scale_y> [<rotation> <alpha> [(fast | good | best | nearest | bilinear | gaussian)]]]]]\n"
			"MSG:  image verbose <level>\n"
			"MSG:  image write <image id> <file name>\n"
			"MSG:  image help	// this list\n"
			"MSG:\n"
			);
	return 0;
}


// Query type 0=load 1=places
// format	: format 
// ids		: list of IDs
// maxid	: max number of ID
// nextavail	: Next available ID
// id or ids	: listing each ID
char* CVideoImage::Query(const char* query, bool tcllist)
{
	u_int32_t type, maxid;

	if (!query) return NULL;
	while (isspace(*query)) query++;
	if (!(strncasecmp("load ", query, 5))) {
		query += 5; type = 0;
	}
       	else if (!(strncasecmp("place ", query, 6))) {
		query += 6; type = 1;
	}
       	else if (!(strncasecmp("move ", query, 5))) {
		query += 5; type = 2;
	}
	else if (!(strncasecmp("extended ", query, 9))) {
		query += 9; type = 3;
	}
       	else if (!(strncasecmp("syntax", query, 6))) {
		return strdup("image (load | place | move | extended | syntax) "
			"(format | ids | maxid | nextavail | <id_list>)");
	} else return NULL;

	while (isspace(*query)) query++;
	maxid = (type == 0 ? MaxImages() : MaxImagesPlace());
	if (!strncasecmp(query, "format", 6)) {
		char* str;
		if (type == 0) {
			if (tcllist) str = strdup("id geometry bits color name file_name");
			else str = strdup("image load <width> <height> <bits> <color> <<name>> <<file_name>>");
		}
		else if (type == 1) {
			// image place - place_id image_id coor offset align anchor scale rotate alpha clip
			// image place - image place <place_id> <image_id> <coor> <offset> <align> <anchor> <scale> <rotate> <alpha> <clip>
			if (tcllist) str = strdup("place_id image_id coor offset align anchor "
				"scale rotate alpha clip filter");
			else str = strdup("image place <place_id> <image_id> <coor> "
				"<offset> <align> <anchor> <scale> <rotate> <alpha> <clip> <filter>");
		}
		else if (type == 2) {
			// image move - place_id coor_move scale_move rotate_move alpha_move clip_move
			// image move - image move place_id <coor_move> <scale_move> <rotate_move> <alpha_move> <clip_move>
			if (tcllist) str = strdup("place_id coor_move scale_move rotate_move alpha_move clip_move");
			else str = strdup("image move place_id <coor_move> <scale_move> <rotate_move> <alpha_move> <clip_move>");
		}
		else {	// type == 3
			// image extended - place_id matrix
			// image extended - place_id <matrix>
			if (tcllist) str = strdup("place_id matrix");
			else str = strdup("image extended <matrix>");
		}
		if (!str) fprintf(stderr, "CVideoImage::Query failed to allocate memory\n");
		return str;
	}
	else if (!strncasecmp(query, "maxid", 5)) {
		int len = (int)(log10((double)maxid)) + 3;	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) fprintf(stderr, "CVideoImage::Query failed to allocate memory\n");
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
			fprintf(stderr, "CVideoImage::Query failed to allocate memory\n");
			return NULL;
		}
		*str = '\0';
		for (u_int32_t i = from; i < maxid ; i++) {
			if ((type == 0 && !m_image_items[i]) ||
				(type == 1 && !m_image_places[i])) {
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
		int len = (int)(0.5+log10((double)maxid)) + 1;
		len *= maxid;
		char* str = (char*) malloc(len+1);
		if (!str) fprintf(stderr, "CVideoImage::Query failed to allocate memory\n");
		else {
			*str = '\0';
			char* p = str;
			for (u_int32_t i = 0; i < maxid ; i++) {
				if ((type == 0 && m_image_items[i]) || (type == 1 && m_image_places[i])) {
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
			if ((type == 0 && !m_image_items[from]) ||
				(type == 1 && !m_image_places[from])) { from++ ; continue; }
			if (type < 1)
				len += (50 + (m_image_items[from]->name ?
					  strlen(m_image_items[from]->name) : 0) +
					strlen(m_image_items[from]->file_name));
			else len += 266;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	if (len < 2) {
		fprintf(stderr, "CVideoImage::Query malformed query or no images loaded yet.\n");
		return NULL;
	}
	char* retstr = (char*) malloc(len);
	if (!retstr) {
		fprintf(stderr, "CVideoImage::Query failed to allocate memory\n");
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

			// id geometry bits color file_name
			// image load <width> <height> <bits> <color> <<file_name>>
			if ((type == 0 && m_image_items[from])) {
				image_item_t* p = m_image_items[from];
				if (tcllist) sprintf(pstr, "{%u {%u %u} %hhu %hhu {%s} {%s}} ", 
					from, p->width, p->height, (u_int8_t) p->bit_depth,
					p->color_type, p->name ? p->name : "", p->file_name);
				else sprintf(pstr, "image load %u %u %u %hhu %hhu <%s> <%s>\n",
					from, p->width, p->height, (u_int8_t) p->bit_depth,
					p->color_type, p->name ? p->name : "", p->file_name);
			}
			else if (type == 1 && m_image_places[from]) {
				image_place_t* pp = m_image_places[from];

				// place_id image_id coor offset align anchor scale rotate alpha clip");
				// image place <place_id> <image_id> <coor> <offset> <align> <anchor> <scale> <rotate> <alpha> <clip>
				if (type == 1) sprintf(pstr, (tcllist ?
					  "{%u %u {%d %d} {%d %d} {%s %s} {%d %d} {%.9lf %0.9lf} %.9lf "
					  "%.9lf {%.5lf %.5lf %.5lf %.5lf} %s} " :
					  "image place %u %u %d %d %d,%d %s,%s %d,%d %.9lf,%0.9lf %.9lf "
					  "%.9lf %.5lf,%.5lf,%.5lf,%.5lf %s\n"),
					from, pp->image_id, pp->x, pp->y, pp->offset_x, pp->offset_y,
					pp->align & SNOWMIX_ALIGN_LEFT ? "left" :
				  	pp->align & SNOWMIX_ALIGN_CENTER ? "center" : "right",
					pp->align & SNOWMIX_ALIGN_TOP ? "top" :
				  	pp->align & SNOWMIX_ALIGN_MIDDLE ? "middle" : "bottom",
					pp->anchor_x, pp->anchor_y,
					pp->scale_x, pp->scale_y, pp->rotation, pp->alpha,
					pp->clip_left, pp->clip_right, pp->clip_top, pp->clip_bottom,
					CairoFilter2String(pp->filter));

				// image move - place_id coor_move scale_move rotate_move alpha_move clip_move
				// image move - image move place_id <coor_move> <scale_move> <rotate_move> <alpha_move> <clip_move>
				else if (type == 2) sprintf(pstr, (tcllist ?
					  "{%u {%d %d %u %u} {%.9lf %9lf %u %u} {%.9lf %u} {%.9lf %u} {%.5lf %.5lf %.5lf %.5lf %u %u %u %u}} " :
					  "image move %u %d,%d,%u,%u %.9lf,%9lf,%u,%u %.9lf,%u %.9lf,%u %.5lf,%.5lf,%.5lf,%.5lf,%u,%u,%u,%u\n"),
					from,
					pp->delta_x, pp->delta_y, pp->delta_x_steps, pp->delta_y_steps,
					pp->delta_scale_x, pp->delta_scale_y,
					pp->delta_scale_x_steps, pp->delta_scale_y_steps,
					pp->delta_rotation, pp->delta_rotation_steps,
					pp->delta_alpha, pp->delta_alpha_steps,
					pp->delta_clip_left, pp->delta_clip_right,
					pp->delta_clip_top, pp->delta_clip_bottom,
					pp->delta_clip_left_steps, pp->delta_clip_right_steps,
					pp->delta_clip_top_steps, pp->delta_clip_bottom_steps);

				// image extended - place_id matrix
				// image extended - place_id <matrix>
				else sprintf(pstr, (tcllist ? "{ %u {%.9lf %.9lf %.9lf %.9lf %.9lf %.9lf}} " :
					  "image extended %u %.9lf,%.9lf,%.9lf,%.9lf,%.9lf,%.9lf\n"),
					from,
					pp->pMatrix ? pp->pMatrix->matrix_xx : 0.0,
					pp->pMatrix ? pp->pMatrix->matrix_xy : 0.0,
					pp->pMatrix ? pp->pMatrix->matrix_yx : 0.0,
					pp->pMatrix ? pp->pMatrix->matrix_yy : 0.0,
					pp->pMatrix ? pp->pMatrix->matrix_x0 : 0.0,
					pp->pMatrix ? pp->pMatrix->matrix_y0 : 0.0);

			}
			while (*pstr) pstr++;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	return retstr;
}

// image source [(feed | image) <image id> (<feed id> | <image id>) [ <offset x> <offset y> <width> height> [<scale_x> <scale_y> [<rotation> <alpha> [(fast | good | best | nearest | bilinear | gaussian)]]]]]
int CVideoImage::set_image_source (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;		// check that ci is not null
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		return 0;
	}
	source_enum_t source_type;
	u_int32_t image_id, source_id, offset_x = 0, offset_y = 0, width = 0, height = 0;
	double scale_x = 1.0, scale_y = 1.0, rotation = 0.0, alpha = 1.0;
	cairo_filter_t filter = CAIRO_FILTER_FAST;
	if (!strncmp(ci, "feed ", 5)) {
		source_type = SNOWMIX_SOURCE_FEED;
		ci += 5;
	} else if (!strncmp(ci, "image ", 6)) {
		source_type = SNOWMIX_SOURCE_IMAGE;
		ci += 6;
	} else if (!strncmp(ci, "frame ", 6)) {
		source_type = SNOWMIX_SOURCE_FRAME;
		ci += 6;
	} else return -1;
	while (isspace(*ci)) ci++;
	int n = sscanf(ci, "%u %u %u %u %u %u %lf %lf %lf %lf", &image_id, &source_id,
		&offset_x, &offset_y, &width, &height, &scale_x, &scale_y, &rotation, &alpha);
	if (n != 2 && n != 6 && n != 8 && n != 10) return -1;
	if (n == 10) {
		for (n=0 ; n < 9; n++) {
			while (!isspace(*ci)) ci++;
			while (isspace(*ci)) ci++;
		}
		while (*ci && !isspace(*ci)) ci++;
		while (isspace(*ci)) ci++;
		if (*ci) {
fprintf(stderr, "FILTER %s\n",ci);
			if (!strncasecmp (ci, "fast ", 5)) filter=CAIRO_FILTER_FAST;
			else if (!strncasecmp (ci, "good ", 5)) filter=CAIRO_FILTER_GOOD;
			else if (!strncasecmp (ci, "best ", 5)) filter=CAIRO_FILTER_BEST;
			else if (!strncasecmp (ci, "nearest ", 8)) filter=CAIRO_FILTER_NEAREST;
			else if (!strncasecmp (ci, "bilinear ", 9)) filter=CAIRO_FILTER_BILINEAR;
			else if (!strncasecmp (ci, "gaussian ", 9)) filter=CAIRO_FILTER_GAUSSIAN;
			else return -1;
		}
	}

	n = SetImageSource(image_id, source_type, source_id,
		offset_x, offset_y, width, height, scale_x, scale_y, rotation, alpha, filter);
	if (m_verbose && !n && pController) pController->controller_write_msg (ctr,
		"MSG: image %u source %s %u clip %u %u %u %u scale %.4lf %.4lf\n",
		image_id, source_type == SNOWMIX_SOURCE_FEED ? "feed" :
		  source_type == SNOWMIX_SOURCE_IMAGE ? "image" : "unknown",
		source_id, offset_x, offset_y, width, height, scale_x, scale_y);

	return 0;
}



// image load [<id no> [<file name>]]  // empty string deletes entry
int CVideoImage::set_image_load (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;		// check that ci is not null

	if (m_verbose) fprintf(stderr, "image load <%s>\n", ci ? ci : "nil");
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		int max_id = MaxImages();
		for (int i=0; i < max_id; i++) {
			struct image_item_t* pImageItem = GetImageItem(i);
			//controller_write_msg (ctr, "%d %s\n", i, pImageItem ? "ptr" : "NULL");
			if (pImageItem) {
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image load %d <%s> %ux%u "
					"bit depth %d type %s seqno %u\n", i, pImageItem->file_name,
					pImageItem->width, pImageItem->height,
					(int)(pImageItem->bit_depth),
					pImageItem->color_type == PNG_COLOR_TYPE_RGB ? "RGB" :
					  (pImageItem->color_type == PNG_COLOR_TYPE_RGBA ? "RGBA" :
					    "Unsupported"),
					  m_seqno_images ? m_seqno_images[i] : 0);
			}
//else fprintf(stderr, "Skipping %d\n", i);
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
	} else {
		if (m_verbose) fprintf(stderr, " - image load <%s>\n", ci ? ci : "nil");
		char* str = (char*) calloc(1,strlen(ci)-1);
		if (!str) {
			if (pController) pController->controller_write_msg (ctr,
				"MSG: Failed to allocate memory for file name %s\n", ci);
			return 0;
		}
		*str = '\0';
		int id;
		int n = sscanf(ci, "%u %[^\n]", &id, str);
		trim_string(str);
		if (n == 1) n = LoadImage(id, NULL);	// Deletes the entry
		else if (n == 2) n = LoadImage(id, str);	// Adds the entry
		else { free(str); return 1; }

		if (!n && m_seqno_images) m_seqno_images[id]++;

		free(str);
		if (m_verbose && pController) pController->controller_write_msg (ctr,
				"MSG: Image %u %s.\n", id, n ? "failed to load" : "loaded");
		return n;
	}
	return 0;
}

// image name [<id no> [<file name>]]  // empty string deletes name
int CVideoImage::set_image_name (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;		// check that ci is not null

	if (m_verbose) fprintf(stderr, "image name <%s>\n", ci ? ci : "nil");
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		int max_id = MaxImages();
		for (int i=0; i < max_id; i++) {
			struct image_item_t* pImageItem = GetImageItem(i);
			//controller_write_msg (ctr, "%d %s\n", i, pImageItem ? "ptr" : "NULL");
			if (pImageItem && pImageItem->name)
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image name %u <%s>\n", i,
					pImageItem->name ? pImageItem->name : "");
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	char* name = NULL;
	int id;
	int n = sscanf(ci, "%u", &id);
	if (n != 1) return -1;
	while (*ci && isdigit(*ci)) ci++;
	while (*ci && isspace(*ci)) ci++;
	if ((n = SetImageName(id, (*ci ? ci : NULL)))) {
		if (m_verbose) fprintf(stderr,
			"Failed to set image name for image id %u to <%s>\n", id, name);
		if (name) free(name);
	} else if (m_verbose && pController) pController->controller_write_msg (ctr,
		"MSG: Setting image %u name %s\n", id, name);
	return n;
}

// image write <id no> <file name>
int CVideoImage::set_image_write (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	if (m_verbose) fprintf(stderr, "image write <%s>\n", ci ? ci : "nil");
	while (isspace(*ci)) ci++;
	if (!(*ci)) return -1;
	char* str = (char*) calloc(1,strlen(ci)-1);
	if (!str) {
		fprintf(stderr, "Failed to allocate memory for image write for file name %s\n", ci);
		return 1;
	}
	*str = '\0';
	int id;
	int n = sscanf(ci, "%u %[^\n]", &id, str);
	if (n != 2) { free(str); return 1; }
	trim_string(str);
	n = WriteImage(id, str);
	if (n) fprintf(stderr, "Image %u failed to write to %s.\n", id, str);
	else if (m_verbose && pController) pController->controller_write_msg (ctr,
		"MSG: Image %u written to %s.\n", id, str);
    free(str);
	return n;
}

// image place matrix [<id no> <xx> <xy> <yx> <yy> <x0> <y0>]
int CVideoImage::set_image_place_matrix (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: place scale matrix "
					"%2d (%.4lf,%.4lf, %.4lf,%.4lf, %.4lf,%.4lf)\n", id,
					pImage->pMatrix ? pImage->pMatrix->matrix_xx : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_yx : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_xy : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_yy : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_x0 : 0.0,
					pImage->pMatrix ? pImage->pMatrix->matrix_y0 : 0.0);
			}
		}
		return 0;
	}

	u_int32_t place_id;
	double xx, xy, yx, yy, x0, y0;
	if (sscanf(ci, "%u %lf %lf %lf %lf %lf %lf", &place_id, &xx, &xy, &yx, &yy, &x0, &y0) != 7)
		return 1;
	int n = SetImageMatrix(place_id, xx, xy, yx, yy, x0, y0);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u matrix "
			"%.5lf,%.5lf %.5lf,%.5lf %.5lf,%.5lf\n",
			place_id, xx, xy, yx, yy, x0, y0);
	return n;

}

// image place scale [<id no> <scale_x> <scale_y>]
int CVideoImage::set_image_place_scale (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				unsigned int w = 0;
				unsigned int h = 0;
				if (m_image_items && m_image_items[pImage->image_id]) {
					w =  m_image_items[pImage->image_id]->width;
					h =  m_image_items[pImage->image_id]->height;
				}
				pController->controller_write_msg (ctr, "MSG: place scale "
					"%2d %.4lf %.4lf placed at %d,%d wxh (%4ux%-3u)\n", id,
					pImage->scale_x, pImage->scale_y,
					pImage->x, pImage->y, w, h);
			}
		}
		return 0;
	}

	u_int32_t place_id;
	double scale_x, scale_y;
	if (sscanf(ci, "%u %lf %lf", &place_id, &scale_x, &scale_y) != 3) return 1;
	int n = SetImageScale(place_id, scale_x, scale_y);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u scale %.5lf,%.5lf\n",
			place_id, scale_x, scale_y);
	return n;
}

// image place alpha [<id no> <alpha>]
int CVideoImage::set_image_place_alpha (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: place alpha "
					"%2d %.5lf\n", id, pImage->alpha);
			}
		}
		return 0;
	}
	u_int32_t place_id;
	int32_t n;
	double alpha;
	if (sscanf(ci, "%u %lf", &place_id, &alpha) != 2) return 1;
	n = SetImageAlpha(place_id, alpha);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u alpha %.5lf\n",
			place_id, alpha);
	return n;
}

// image place rotation [<id no> <rotation>]
int CVideoImage::set_image_place_rotation (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				unsigned int w = 0;
				unsigned int h = 0;
				if (m_image_items && m_image_items[pImage->image_id]) {
					w =  m_image_items[pImage->image_id]->width;
					h =  m_image_items[pImage->image_id]->height;
				}
				pController->controller_write_msg (ctr, "MSG: place rotation "
					"%2d %.5lf placed at %5d,%-5d wxh %4ux%-3u\n", id,
					pImage->rotation,
					pImage->x, pImage->y, w, h);
			}
		}
		return 0;
	}
	u_int32_t place_id;
	double rotation;
	if (sscanf(ci, "%u", &place_id) != 1) return -1;
	while (isdigit(*ci)) ci++;
	while (isspace(*ci)) ci++;
	if (!NumberWithPI(ci, &rotation)) return -1;
	int n = SetImageRotation(place_id, rotation);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u rotation %.5lf\n",
			place_id, rotation);
	return n;
}

// image place anchor [<id no> (n | s | e | w | ne | nw | se | sw)]
// Set image place  anchor
int CVideoImage::set_image_place_anchor (struct controller_type *ctr, CController* pController, const char* str) {
	if (!str) return -1;
	while (isspace(*str)) str++;

	if (!(*str)) {
		if (!m_image_places || !pController) return 1;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if (m_image_places[id]) {
			pController->controller_write_msg (ctr, "MSG: image place id %2d "
				"anchor %d,%d\n", id,
				m_image_places[id]->anchor_x, m_image_places[id]->anchor_y);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	u_int32_t id;
	char anchor[3];
	anchor[0] = '\0';
	if (sscanf(str, "%u %2["ANCHOR_TAGS"]", &id, &anchor[0]) != 2) return -1;
	int n = SetImageAnchor(id, anchor);
	if (m_verbose && !n)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"image place %u anchor %s\n", id, anchor);
	return n;

}


//image place align [<id no> (left | center | right) (top | middle | bottom)] "
int CVideoImage::set_image_place_align (struct controller_type *ctr, CController* pController, const char* str)
{
	while (isspace(*str)) str++;
	if (!(*str)) {
		struct image_place_t* pPlacedImage = NULL;
		unsigned int max = MaxImagesPlace();
		for (unsigned int id = 0; id < max; id++) {
			if ((pPlacedImage = GetImagePlaced(id))) {
				unsigned int w = 0;
				unsigned int h = 0;
				if (m_image_items && m_image_items[pPlacedImage->image_id]) {
					w =  m_image_items[pPlacedImage->image_id]->width;
					h =  m_image_items[pPlacedImage->image_id]->height;
				}
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image place id %2d x,y %5d,%-5d "
					"wxh %4dx%-4d rot %.4lf %s %s\n",
					id, pPlacedImage->x, pPlacedImage->y,
					w, h, pPlacedImage->rotation,
					pPlacedImage->align & SNOWMIX_ALIGN_CENTER ? "center" :
					  (pPlacedImage->align & SNOWMIX_ALIGN_RIGHT ? "right" : "left"),
					pPlacedImage->align & SNOWMIX_ALIGN_MIDDLE ? "middle" :
					  (pPlacedImage->align & SNOWMIX_ALIGN_BOTTOM ? "bottom" : "top"));
			}
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	u_int32_t place_id;
	int n = 0;
	//u_int32_t align = SNOWMIX_ALIGN_LEFT | SNOWMIX_ALIGN_TOP;
	if (sscanf(str, "%u", &place_id) != 1) return 1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	while (*str && !n) {
		int align = 0;
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
		} else {
			return 1;		// parameter error
		}
 		if ((n = SetImageAlign(place_id, align))) break;
		while (isspace(*str)) str++;
	}
	if (!n && m_verbose && pController) pController->controller_write_msg (ctr,
		"MSG: Image Align Set.\n");
	return n;
}

// image geometry [<place id>]
int CVideoImage::set_image_geometry (struct controller_type *ctr, CController* pController, const char* str)
{
	struct image_place_t* pPlacedImage = NULL;
	unsigned int max = MaxImagesPlace();

	while (isspace(*str)) str++;
	if (!(*str)) {
		for (unsigned int id = 0; id < max; id++) {
			if ((pPlacedImage = GetImagePlaced(id))) {
				unsigned int w = 0;
				unsigned int h = 0;
				if (m_image_items && m_image_items[pPlacedImage->image_id]) {
					w =  m_image_items[pPlacedImage->image_id]->width;
					h =  m_image_items[pPlacedImage->image_id]->height;
				}
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image place id %2d geometry %dx%d\n", id, w, h);
			}
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}
	u_int32_t id;
	if (sscanf(str, "%u", &id) != 1) return -1;
	if ((pPlacedImage = GetImagePlaced(id))) {
		unsigned int w = 0;
		unsigned int h = 0;
		if (m_image_items && m_image_items[pPlacedImage->image_id]) {
			w =  m_image_items[pPlacedImage->image_id]->width;
			h =  m_image_items[pPlacedImage->image_id]->height;
		}
		if (pController) pController->controller_write_msg (ctr,
			"MSG: image place id %2d geometry %dx%d\n", id, w, h);
	} else return -1;
	return 0;
}


// image place coor [<place id no> [<x> <y> [(n | s | e | w | c | ne | nw | se | sw)] [(left | center | right) (top | middle | bottom)]
// image place [<place id no> [<image id> <x> <y> [(n | s | e | w | c | ne | nw | se | sw)] [(left | center | right) (top | middle | bottom)]
int CVideoImage::set_image_place (struct controller_type *ctr, CController* pController, const char* str)
{
	while (isspace(*str)) str++;
	if (!(*str) || strcasecmp(str, "coor ") == 0) {
		struct image_place_t* pPlacedImage = NULL;
		unsigned int max = MaxImagesPlace();
		for (unsigned int id = 0; id < max; id++) {
			if ((pPlacedImage = GetImagePlaced(id))) {
				unsigned int w = 0;
				unsigned int h = 0;
				if (m_image_items && m_image_items[pPlacedImage->image_id]) {
					w =  m_image_items[pPlacedImage->image_id]->width;
					h =  m_image_items[pPlacedImage->image_id]->height;
				}
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image place id %2d image_id %2d x,y %5d,%-5d "
					"wxh %4dx%-4d rot %.4lf alpha %.3lf anchor %u,%u %s %s %s\n",
					id, pPlacedImage->image_id, pPlacedImage->x, pPlacedImage->y,
					w, h, pPlacedImage->rotation, pPlacedImage->alpha,
					pPlacedImage->anchor_x, pPlacedImage->anchor_y,
					pPlacedImage->align & SNOWMIX_ALIGN_CENTER ? "center" :
					  (pPlacedImage->align & SNOWMIX_ALIGN_RIGHT ? "right" : "left"),
					pPlacedImage->align & SNOWMIX_ALIGN_MIDDLE ? "middle" :
					  (pPlacedImage->align & SNOWMIX_ALIGN_BOTTOM ? "bottom" : "top"),
					  
					pPlacedImage->display ? "on" : "off");
			}
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
	} else {
		u_int32_t place_id, image_id;
		int32_t x, y;
		char anchor[3];
		bool coor = false;
		//u_int32_t align = SNOWMIX_ALIGN_LEFT | SNOWMIX_ALIGN_TOP;
		anchor[0] = '\0';
		if (!strncasecmp(str, "coor ", 5)) {
			coor = true;
			str += 5;
			while (isspace(*str)) str++;
		}
		int n = 0;
		if (coor) n = sscanf(str, "%u %d %d %2["ANCHOR_TAGS"]", &place_id, &x, &y, &anchor[0]);
		else n = sscanf(str, "%u %u %d %d %2["ANCHOR_TAGS"]", &place_id, &image_id, &x, &y, &anchor[0]);
		// Anchor tags would match "ce" as in center so we need to check
		if (!strncmp(anchor,"ce",2)) {
			anchor[0] = '\0';
			n--;
		}
		// Should we delete the entry or place it
		if (n == 1) {
			RemoveImagePlaced(place_id);
			if (m_verbose && pController) pController->controller_write_msg (ctr,
				"MSG: Image %u removed.\n");
		}
		else if (n == 4 || (coor && n == 3) || (!coor && n == 5)) {
			int i = SetImagePlace(place_id, image_id, x, y, true, coor, anchor);
			if (i) return i;
			for (i=0; i < (coor ? 3 : 4) ; i++) {
				while (isspace(*str)) str++;
				if (*str == '-' || *str == '+') str++;
				while (isdigit(*str)) str++;
			}
			if ((coor && n == 4) || (!coor && n ==5)) {
				while (isspace(*str)) str++;
				while (*str == 'n' || *str == 's' || *str == 'e' || *str == 'w') str++;
			}
			while (isspace(*str)) str++;
//fprintf(stderr, " - image place %u <%s>\n", place_id, str);
			while (*str) {
				int align = 0;
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
				} else {
					return 1;		// parameter error
				}
	 			SetImageAlign(place_id, align);
				while (isspace(*str)) str++;
			}
			if (m_verbose && pController) {
				if (coor) pController->controller_write_msg (ctr,
					"MSG: image place %u coor %u,%u.\n", place_id, x, y);
				else pController->controller_write_msg (ctr,
					"MSG: image place %u image %u coor %u,%u .\n",
					place_id, image_id, x, y);
			}
		} else return 1;
	}
	return 0;
}

// image place image [<place id no> <image id>]
int CVideoImage::set_image_place_image (struct controller_type *ctr, CController* pController, const char* str)
{
	while (isspace(*str)) str++;
	if (!(*str) || strcasecmp(str, "coor ") == 0) {
		struct image_place_t* pPlacedImage = NULL;
		unsigned int max = MaxImagesPlace();
		for (unsigned int id = 0; id < max; id++) {
			if ((pPlacedImage = GetImagePlaced(id))) {
				char* file_name = NULL;
				if (m_image_items && m_image_items[pPlacedImage->image_id])
					file_name = m_image_items[pPlacedImage->image_id]->file_name;
				if (pController) pController->controller_write_msg (ctr,
					"MSG: image place id %2d image_id %2d %s\n",
					id, pPlacedImage->image_id, file_name ? file_name : "null");
			}
		}
		if (pController) pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}
	u_int32_t place_id, image_id;
	if (sscanf(str, "%u %u", &place_id, &image_id) != 2) return 1;
	int n = SetImagePlaceImage(place_id, image_id);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u image %u\n",
			place_id, image_id);
	return n;
}


// image place move clip [<id no> <delta left> <delta right> <delta top> <delta bottom>
//		<left steps> <right steps> <top steps> <bottom steps>]
int CVideoImage::set_image_place_move_clip (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move "
					"clip %2d %.3lf %.3lf %.3lf %.3lf %u %u %u %u\n", id,
					pImage->delta_clip_left, pImage->delta_clip_right,
					pImage->delta_clip_top, pImage->delta_clip_bottom,
					pImage->delta_clip_left_steps, pImage->delta_clip_right_steps,
					pImage->delta_clip_top_steps, pImage->delta_clip_bottom_steps
					);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, step_left, step_right, step_top, step_bottom;
	double delta_clip_left, delta_clip_right, delta_clip_top, delta_clip_bottom;
	if (sscanf(ci, "%u %lf %lf %lf %lf %u %u %u %u",
		&place_id, &delta_clip_left, &delta_clip_right, &delta_clip_top, &delta_clip_bottom,
		&step_left, &step_right, &step_top, &step_bottom) != 9) return 1;
	int n = SetImageMoveClip(place_id, delta_clip_left, delta_clip_right, delta_clip_top,
		delta_clip_bottom, step_left, step_right, step_top, step_bottom);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u move "
			"clip %.5lf,%.5lf,%.5lf,%.5lf steps %u,%u,%u,%u\n",
			place_id, delta_clip_left, delta_clip_right, delta_clip_top,
			delta_clip_bottom, step_left, step_right, step_top, step_bottom);
	return n;
}

// image place clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
int CVideoImage::set_image_place_clip (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place "
					"clip %2d %.3lf %.3lf %.3lf %.3lf\n", id,
					pImage->clip_left, pImage->clip_right,
					pImage->clip_top, pImage->clip_bottom);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	double clip_left, clip_right, clip_top, clip_bottom;
	if (sscanf(ci, "%u %lf %lf %lf %lf",
		&place_id, &clip_left, &clip_right, &clip_top, &clip_bottom) != 5) return 1;
	int n = SetImageClip(place_id, clip_left, clip_right, clip_top, clip_bottom);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr,
			"MSG: image place %u clip %.2lf,%.2lf,%.2lf,%.2lf\n",
			place_id, clip_left, clip_right, clip_top, clip_bottom);
	return n;
}

// image place filter [<place id> (fast | good | best | nearest | bilinear | gaussian)]
int CVideoImage::set_image_place_filter (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImage = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImage = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place "
					"filter %2d %s\n", id,
					CairoFilter2String(pImage->filter));
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	cairo_filter_t filter;
	if (sscanf(ci, "%u", &place_id) != 1) return -1;
	while (isdigit(*ci)) ci++;
	while (isspace(*ci)) ci++;
	if (!strncasecmp (ci, "fast ", 5)) filter=CAIRO_FILTER_FAST;
	else if (!strncasecmp (ci, "good ", 5)) filter=CAIRO_FILTER_GOOD;
	else if (!strncasecmp (ci, "best ", 5)) filter=CAIRO_FILTER_BEST;
	else if (!strncasecmp (ci, "nearest ", 8)) filter=CAIRO_FILTER_NEAREST;
	else if (!strncasecmp (ci, "bilinear ", 9)) filter=CAIRO_FILTER_BILINEAR;
	else if (!strncasecmp (ci, "gaussian ", 9)) filter=CAIRO_FILTER_GAUSSIAN;
	else return 1;
	int n = SetImageFilter(place_id,filter);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr,
			"MSG: image place %u filter %s\n",
			place_id, CairoFilter2String(filter));
	return n;
}

// image place move scale [<id no> [<delta_scale_x> <delta_scale_y> <steps_x> <steps_y>]]
int CVideoImage::set_image_place_move_scale (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move "
					"scale %2d %.3lf %.3lf %u %u\n", id,
					pImagePlaced->delta_scale_x, pImagePlaced->delta_scale_y,
					pImagePlaced->delta_scale_x_steps,
					pImagePlaced->delta_scale_y_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, dxs, dys;
	double delta_scale_x, delta_scale_y;
	if (sscanf(ci, "%u %lf %lf %u %u", &place_id, &delta_scale_x, &delta_scale_y, &dxs, &dys) != 5) return 1;
	int n = SetImageMoveScale(place_id, delta_scale_x, delta_scale_y, dxs, dys);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u move "
			"scale %.5lf,%.5lf steps %u,%u\n",
			place_id, delta_scale_x, delta_scale_y, dxs, dys);
	return n;
}

// image place offset [<id no> <offset x> <offset y>]
int CVideoImage::set_image_place_offset (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place offset"
					" %2d at %d,%d offset %d,%d\n", id,
					pImagePlaced->x, pImagePlaced->y,
					pImagePlaced->offset_x, pImagePlaced->offset_y);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	int32_t offset_x, offset_y;
	if (sscanf(ci, "%u %d %d", &place_id, &offset_x, &offset_y) != 3) return 1;
	int n = SetImageOffset(place_id, offset_x, offset_y);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u offset %u,%u\n",
			place_id, offset_x, offset_y);
	return n;
}

// image place move coor [<id no> [<delta_x> <delta_y> <steps_x> <steps_y>]]
int CVideoImage::set_image_place_move_coor (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move coor"
					" %2d %d %d %u %u\n", id,
					pImagePlaced->delta_x, pImagePlaced->delta_y,
					pImagePlaced->delta_x_steps, pImagePlaced->delta_y_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, dxs, dys;
	int32_t dx, dy;
	if (sscanf(ci, "%u %d %d %u %u", &place_id, &dx, &dy, &dxs, &dys) != 5) return 1;
	int n = SetImageMoveCoor(place_id, dx, dy, dxs, dys);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u move "
			"coor %d,%d steps %u,%u\n",
			place_id, dx, dy, dxs, dys);
	return n;
}

// image place move offset [<id no> [<delta_offset_x> <delta_offset_y> <steps_offset_x> <steps_offset_y>]]
int CVideoImage::set_image_place_move_offset (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move offset"
					" %2d %d %d %u %u\n", id,
					pImagePlaced->delta_offset_x, pImagePlaced->delta_offset_y,
					pImagePlaced->delta_offset_x_steps, pImagePlaced->delta_offset_y_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, dxs, dys;
	int32_t dx, dy;
	if (sscanf(ci, "%u %d %d %u %u", &place_id, &dx, &dy, &dxs, &dys) != 5) return 1;
	int n = SetImageMoveOffset(place_id, dx, dy, dxs, dys);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u move "
			"offset %d,%d steps %u,%u\n",
			place_id, dx, dy, dxs, dys);
	return n;
}

// image place move alpha [<place id> <delta_alpha> <step_alpha>]
int CVideoImage::set_image_place_move_alpha (struct controller_type *ctr,
	CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move "
					"alpha %2d %.6lf %u\n", id,
					pImagePlaced->delta_alpha, 
					pImagePlaced->delta_alpha_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, delta_alpha_steps;
	double delta_alpha;
	if (sscanf(ci, "%u %lf %u", &place_id, &delta_alpha, &delta_alpha_steps) != 3) return -1;
	int n = SetImageMoveAlpha(place_id, delta_alpha, delta_alpha_steps);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr,
			"MSG: image place %u move alpha %.5lf steps %u\n",
			place_id, delta_alpha, delta_alpha_steps);
	return n;
}

// image place move rotation [<place id> <delta_rotation> <step_rotation>]
int CVideoImage::set_image_place_move_rotation (struct controller_type *ctr,
	CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		image_place_t* pImagePlaced = NULL;
		for (unsigned int id = 0; id < MaxImagesPlace(); id++) {
			if ((pImagePlaced = GetImagePlaced(id))) {
				pController->controller_write_msg (ctr, "MSG: image place move "
					"rotation %2d %.6lf %u\n", id,
					pImagePlaced->delta_rotation, 
					pImagePlaced->delta_rotation_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, delta_rotation_steps;
	double delta_rotation;
	if (sscanf(ci, "%u", &place_id) != 1) return -1;
	while (isdigit(*ci)) ci++;
	while (isspace(*ci)) ci++;
	if (!NumberWithPI(ci, &delta_rotation)) return -1;
	while (*ci && !isspace(*ci)) ci++;
	while (isspace(*ci)) ci++;
	if (sscanf(ci, "%u", &delta_rotation_steps) != 1) return -1;
	int n = SetImageMoveRotation(place_id, delta_rotation, delta_rotation_steps);
	if (!n && m_verbose && pController)
		pController->controller_write_msg (ctr, "MSG: image place %u move "
			"rotation %.5lf steps %u\n",
			place_id, delta_rotation, delta_rotation_steps);
	return n;
}

int CVideoImage::SetImageMatrix(u_int32_t place_id, double xx, double xy, double yx, double yy, double x0, double y0)
{
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];
		if (!pImage->pMatrix)
			pImage->pMatrix = (transform_matrix_t*)
				calloc(sizeof(transform_matrix_t), 1);
		if (!pImage->pMatrix) return 1;
		pImage->pMatrix->matrix_xx = xx;
		pImage->pMatrix->matrix_xy = xy;
		pImage->pMatrix->matrix_yx = yx;
		pImage->pMatrix->matrix_yy = yy;
		pImage->pMatrix->matrix_x0 = x0;
		pImage->pMatrix->matrix_y0 = y0;
	} else return -1;
	return 0;
}

int CVideoImage::SetImageClip(u_int32_t place_id, double clip_left, double clip_right,
	double clip_top, double clip_bottom) {
	if (place_id < m_max_image_places && m_image_places[place_id]) {
		image_place_t* pImage = m_image_places[place_id];

		if (clip_left < 0.0) clip_left = 0.0;
		else if (clip_left > 1.0) clip_left = 1.0;
		if (clip_right < 0.0) clip_right = 0.0;
		else if (clip_right > 1.0) clip_right = 1.0;
		if (clip_top < 0.0) clip_top = 0.0;
		else if (clip_top > 1.0) clip_top = 1.0;
		if (clip_bottom < 0.0) clip_bottom = 0.0;
		else if (clip_bottom > 1.0) clip_bottom = 1.0;
		pImage->clip_left = clip_left;
		pImage->clip_right = clip_right;
		pImage->clip_top = clip_top;
		pImage->clip_bottom = clip_bottom;
		return 0;
	} else return -1;
}

int CVideoImage::SetImageFilter(u_int32_t place_id, cairo_filter_t filter) {
	if (place_id < m_max_image_places && m_image_places[place_id])
		m_image_places[place_id]->filter = filter;
	else return -1;
	return 0;
}

int CVideoImage::SetImageMoveClip(u_int32_t place_id, double delta_clip_left,
	double delta_clip_right, double delta_clip_top, double delta_clip_bottom,
	u_int32_t step_left, u_int32_t step_right, u_int32_t step_top, u_int32_t step_bottom) {
	if (place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	image_place_t* pImage = m_image_places[place_id];

	if (delta_clip_left < -1.0) delta_clip_left = -1.0;
	else if (delta_clip_left > 1.0) delta_clip_left = 1.0;
	if (delta_clip_right < -1.0) delta_clip_right = -1.0;
	else if (delta_clip_right > 1.0) delta_clip_right = 1.0;
	if (delta_clip_top < -1.0) delta_clip_top = -1.0;
	else if (delta_clip_top > 1.0) delta_clip_top = 1.0;
	if (delta_clip_bottom < -1.0) delta_clip_bottom = -1.0;
	else if (delta_clip_bottom > 1.0) delta_clip_bottom = 1.0;

	pImage->delta_clip_left = delta_clip_left;
	pImage->delta_clip_right = delta_clip_right;
	pImage->delta_clip_top = delta_clip_top;
	pImage->delta_clip_bottom = delta_clip_bottom;
	pImage->delta_clip_left_steps = step_left;
	pImage->delta_clip_right_steps = step_right;
	pImage->delta_clip_top_steps = step_top;
	pImage->delta_clip_bottom_steps = step_bottom;
	if (step_left || step_right || step_top || step_bottom) pImage->update = true;
	return 0;
}

int CVideoImage::SetImageMoveAlpha(u_int32_t place_id, double delta_alpha,
	u_int32_t delta_alpha_steps) {
	if (place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	image_place_t* pImage = m_image_places[place_id];
	pImage->delta_alpha = delta_alpha;
	pImage->delta_alpha_steps = delta_alpha_steps;
	if (delta_alpha_steps) pImage->update = true;
	return 0;
}

int CVideoImage::SetImageMoveRotation(u_int32_t place_id, double delta_rotation,
	u_int32_t delta_rotation_steps) {
	if (place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	image_place_t* pImage = m_image_places[place_id];
	pImage->delta_rotation = delta_rotation;
	pImage->delta_rotation_steps = delta_rotation_steps;
	if (delta_rotation_steps) pImage->update = true;
	return 0;
}

int CVideoImage::SetImageMoveScale(u_int32_t place_id, double delta_scale_x,
	double delta_scale_y, u_int32_t delta_scale_x_steps, u_int32_t delta_scale_y_steps) {
	if (place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	image_place_t* pImage = m_image_places[place_id];
	pImage->delta_scale_x = delta_scale_x;
	pImage->delta_scale_y = delta_scale_y;
	pImage->delta_scale_x_steps = delta_scale_x_steps;
	pImage->delta_scale_y_steps = delta_scale_y_steps;
	if (delta_scale_x_steps || delta_scale_y_steps) pImage->update = true;
	return 0;
}

int CVideoImage::SetImageOffset(u_int32_t place_id, int32_t offset_x, int32_t offset_y)
{
	if (place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	image_place_t* pImage = m_image_places[place_id];
	pImage->offset_x = offset_x;
	pImage->offset_y = offset_y;
	return 0;
}

int CVideoImage::SetImageMoveCoor(u_int32_t place_id, int32_t delta_x, int32_t delta_y,
	u_int32_t delta_x_steps, u_int32_t delta_y_steps) {
	if (place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	image_place_t* pImage = m_image_places[place_id];
	pImage->delta_x = delta_x;
	pImage->delta_y = delta_y;
	pImage->delta_x_steps = delta_x_steps;
	pImage->delta_y_steps = delta_y_steps;
	if (delta_x_steps || delta_y_steps) pImage->update = true;
	return 0;
}

int CVideoImage::SetImageMoveOffset(u_int32_t place_id, int32_t delta_offset_x,
	int32_t delta_offset_y, u_int32_t delta_offset_x_steps, u_int32_t delta_offset_y_steps) {
	if (place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	image_place_t* pImage = m_image_places[place_id];
	pImage->delta_offset_x = delta_offset_x;
	pImage->delta_offset_y = delta_offset_y;
	pImage->delta_offset_x_steps = delta_offset_x_steps;
	pImage->delta_offset_y_steps = delta_offset_y_steps;
	if (delta_offset_x_steps || delta_offset_y_steps) pImage->update = true;
	return 0;
}

int CVideoImage::SetImageScale(u_int32_t place_id, double scale_x, double scale_y) {
	if (place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	m_image_places[place_id]->scale_x = scale_x;
	m_image_places[place_id]->scale_y = scale_y;
	return 0;
}

int CVideoImage::SetImageAlpha(u_int32_t place_id, double alpha) {
	if (place_id < m_max_image_places && m_image_places[place_id])
		m_image_places[place_id]->alpha = alpha;
	else return -1;
	return 0;
}

int CVideoImage::SetImageRotation(u_int32_t place_id, double rotation) {
	if (place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	m_image_places[place_id]->rotation = rotation;
	return 0;
}

// Max places image possible
unsigned int CVideoImage::MaxImagesPlace()
{
	return m_max_image_places;
}

//
int CVideoImage::SetImageAlign(u_int32_t place_id, int align) {
	if (place_id >= m_max_image_places || !m_image_places ||
		!m_image_places[place_id]) return 1;
	if (align & (SNOWMIX_ALIGN_LEFT | SNOWMIX_ALIGN_CENTER | SNOWMIX_ALIGN_RIGHT)) {
		m_image_places[place_id]->align = (m_image_places[place_id]->align &
			(~(SNOWMIX_ALIGN_LEFT | SNOWMIX_ALIGN_CENTER | SNOWMIX_ALIGN_RIGHT))) | align;
	} else if  (align & (SNOWMIX_ALIGN_TOP | SNOWMIX_ALIGN_MIDDLE | SNOWMIX_ALIGN_BOTTOM)) {
		m_image_places[place_id]->align = (m_image_places[place_id]->align &
			(~(SNOWMIX_ALIGN_TOP  | SNOWMIX_ALIGN_MIDDLE | SNOWMIX_ALIGN_BOTTOM))) | align;
	}
	return 0;
}

// Change the image of a placed image
int CVideoImage::SetImagePlaceImage(u_int32_t place_id, u_int32_t image_id)
{
	if (place_id >= m_max_image_places || !m_image_places || !m_image_places[place_id])
		return 1;
	m_image_places[place_id]->image_id = image_id;
	return 0;
}

// Create a image placed and add to image place list
int CVideoImage::SetImagePlace(u_int32_t place_id, u_int32_t image_id, int32_t x, int32_t y, bool display, bool coor, const char* anchor)
{
	if (place_id >= m_max_image_places || !m_image_places ||
		(coor && !m_image_places[place_id]))
		return 1;
	struct image_place_t* pImagePlaced = NULL;
	if (!m_image_places[place_id]) {
		if (!(m_image_places[place_id] =
			(struct image_place_t*) calloc(sizeof(struct image_place_t), 1)))
			return 1;
		if ((pImagePlaced =m_image_places[place_id])) {
			pImagePlaced->align = SNOWMIX_ALIGN_TOP | SNOWMIX_ALIGN_LEFT;
			pImagePlaced->anchor_x = 0;
			pImagePlaced->anchor_y = 0;
			pImagePlaced->offset_x = 0;
			pImagePlaced->offset_y = 0;
			pImagePlaced->scale_x = 1.0;
			pImagePlaced->scale_y = 1.0;
			pImagePlaced->display = true;
			pImagePlaced->alpha = 1.0;
			pImagePlaced->filter = CAIRO_FILTER_FAST;
			pImagePlaced->rotation = 0.0;
			pImagePlaced->pMatrix = NULL;
			pImagePlaced->delta_x			= 0;
			pImagePlaced->delta_x_steps		= 0;
			pImagePlaced->delta_y			= 0;
			pImagePlaced->delta_y_steps		= 0;
			pImagePlaced->delta_offset_x		= 0;
			pImagePlaced->delta_offset_x_steps	= 0;
			pImagePlaced->delta_offset_y		= 0;
			pImagePlaced->delta_offset_y_steps	= 0;
			pImagePlaced->delta_scale_x		= 0.0;
			pImagePlaced->delta_scale_x_steps	= 0;
			pImagePlaced->delta_scale_y		= 0.0;
			pImagePlaced->delta_scale_y_steps	= 0;
			pImagePlaced->delta_rotation		= 0.0;
			pImagePlaced->delta_rotation_steps	= 0;
			pImagePlaced->delta_alpha		= 0.0;
			pImagePlaced->delta_alpha_steps		= 0;
			pImagePlaced->clip_left			= 0.0;
			pImagePlaced->clip_right		= 1.0;
			pImagePlaced->clip_top			= 0.0;
			pImagePlaced->clip_bottom		= 1.0;
			pImagePlaced->update			= false;
		}
	} 
	pImagePlaced = m_image_places[place_id];
	if (!coor) pImagePlaced->image_id = image_id;
	pImagePlaced->x = x;
	pImagePlaced->y = y;

	// default anchor nw
	if (anchor && *anchor) return SetImageAnchor(place_id, anchor);

	return 0;
}


int CVideoImage::SetImageAnchor(u_int32_t place_id, const char* anchor)
{
	//if (!anchor || place_id >= m_max_image_places || !m_image_places[place_id]) return -1;
	if (place_id >= m_max_image_places || !m_image_places[place_id] || !m_pVideoMixer) return -1;
	//image_place_t* pImage = m_image_places[place_id];
	return SetAnchor(&(m_image_places[place_id]->anchor_x),
		&(m_image_places[place_id]->anchor_y),
		anchor, m_pVideoMixer->GetSystemWidth(), m_pVideoMixer->GetSystemHeight());
/*
	if (strcasecmp("ne", anchor) == 0) {
		pImage->anchor_x = m_width;
		pImage->anchor_y = 0;
	} else if (strcasecmp("se", anchor) == 0) {
		pImage->anchor_x = m_width;
		pImage->anchor_y = m_height;
	} else if (strcasecmp("sw", anchor) == 0) {
		pImage->anchor_x = 0;
		pImage->anchor_y = m_height;
	} else if (strcasecmp("n", anchor) == 0) {
		pImage->anchor_x = m_width>>1;
		pImage->anchor_y = 0;
	} else if (strcasecmp("w", anchor) == 0) {
		pImage->anchor_x = m_width;
		pImage->anchor_y = m_height>>1;
	} else if (strcasecmp("s", anchor) == 0) {
		pImage->anchor_x = m_width>>1;
		pImage->anchor_y = m_height;
	} else if (strcasecmp("e", anchor) == 0) {
		pImage->anchor_x = m_width;
		pImage->anchor_y = m_height>>1;
	} else {
		pImage->anchor_x = 0;
		pImage->anchor_y = 0;
	}
	return 0;
*/
}

// Remove a placed image
int CVideoImage::RemoveImagePlaced(u_int32_t place_id)
{
	if (place_id >= m_max_image_places || !m_image_places)
		return 1;
	if (m_image_places[place_id]) free(m_image_places[place_id]);
	m_image_places[place_id] = NULL;
	return 0;
}

// Get a placed image
struct image_place_t* CVideoImage::GetImagePlaced(unsigned int id)
{
	if (id >= m_max_image_places || !m_image_places) return NULL;
	return m_image_places[id];
}

// Max images to load from files
unsigned int CVideoImage::MaxImages()
{
	return m_max_images;
}

// Image item loaded from file
struct image_item_t* CVideoImage::GetImageItem(unsigned int i)
{
	return (i >= m_max_images ? NULL : m_image_items[i]);
}


void CVideoImage::UpdateMove(u_int32_t place_id)
{
	image_place_t* p = GetImagePlaced(place_id);
	if (!p || !p->update) return;
	bool changed = false;

	// check for 'image place move coor'
	if (p->delta_x_steps > 0) {
		p->delta_x_steps--;
		p->x += p->delta_x;
		changed = true;
	}
	if (p->delta_y_steps > 0) {
		p->delta_y_steps--;
		p->y += p->delta_y;
		changed = true;
	}

	// check for 'image place move offset'
	if (p->delta_offset_x_steps > 0) {
		p->delta_offset_x_steps--;
		p->offset_x += p->delta_offset_x;
		changed = true;
	}
	if (p->delta_offset_y_steps > 0) {
		p->delta_offset_y_steps--;
		p->offset_y += p->delta_offset_y;
		changed = true;
	}

	// check for 'image place move scale'
	if (p->delta_scale_x_steps > 0) {
		p->delta_scale_x_steps--;
		double scale = p->scale_x + p->delta_scale_x;
		if (scale > 10000.0) scale = 10000.0;
		else if (scale <= 0.0) scale = p->scale_x;
		p->scale_x = scale;
		changed = true;
	}
	// check for 'image place move scale'
	if (p->delta_scale_y_steps > 0) {
		p->delta_scale_y_steps--;
		double scale = p->scale_y + p->delta_scale_y;
		if (scale > 10000.0) scale = 10000.0;
		else if (scale <= 0.0) scale = p->scale_y;
		p->scale_y = scale;
		changed = true;
	}

	// check for 'image place move alpha'
	if (p->delta_alpha_steps > 0) {
		p->delta_alpha_steps--;
		p->alpha += p->delta_alpha;
		if (p->alpha > 1.0) p->alpha = 1.0;
		else if (p->alpha < 0.0) p->alpha = 0.0;
		changed = true;
	}

	// check for 'image place move rotation'
	if (p->delta_rotation_steps > 0) {
		p->delta_rotation_steps--;
		p->rotation += p->delta_rotation;
//fprintf(stderr, "Rotation %.3lf\n", p->rotation);
		if (p->rotation > 2*M_PI)
			p->rotation -= (2*M_PI);
		else if (p->rotation < -2*M_PI)
			p->rotation += (2*M_PI);
		changed = true;
	}
	// check for 'image place move clip'
	if (p->delta_clip_left_steps > 0) {
		p->delta_clip_left_steps--;
		p->clip_left += p->delta_clip_left;
		if (p->clip_left > 1.0) p->clip_left = 1.0;
		else if (p->clip_left < 0.0) p->clip_left = 0.0;
		changed = true;
	}
	// check for 'image place move clip'
	if (p->delta_clip_right_steps > 0) {
		p->delta_clip_right_steps--;
		p->clip_right += p->delta_clip_right;
		if (p->clip_right > 1.0) p->clip_right = 1.0;
		else if (p->clip_right < 0.0) p->clip_right = 0.0;
		changed = true;
	}
	// check for 'image place move clip'
	if (p->delta_clip_top_steps > 0) {
		p->delta_clip_top_steps--;
		p->clip_top += p->delta_clip_top;
		if (p->clip_top > 1.0) p->clip_top = 1.0;
		else if (p->clip_top < 0.0) p->clip_top = 0.0;
		changed = true;
	}
	// check for 'image place move clip'
	if (p->delta_clip_bottom_steps > 0) {
		p->delta_clip_bottom_steps--;
		p->clip_bottom += p->delta_clip_bottom;
		if (p->clip_bottom > 1.0) p->clip_bottom = 1.0;
		else if (p->clip_bottom < 0.0) p->clip_bottom = 0.0;
		changed = true;
	}
	// If nothing changed, further update runs are prevented until
	// move paramaters are set again with a step != 0.
	p->update = changed;
}

int CVideoImage::WriteImage(u_int32_t image_id, const char* file_name)
{
fprintf(stderr, "IMAGE WRITE\n");
	if (image_id >= m_max_images) {
		fprintf(stderr, "Image id %u larger than configured max %u\n", image_id, m_max_images-1);
		return -1;
	}
	if (!m_image_items[image_id] || !m_image_items[image_id]->pImageData) {
		fprintf(stderr, "Image id %u does not exist or has no image data\n", image_id);
		return -1;
	}
	if (!file_name || !(*file_name)) {
		fprintf(stderr, "File name error\n");
		return -1;
	}
	struct image_item_t* pImage = m_image_items[image_id];
#ifndef WIN32
	int fd = open(file_name, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
#else
	int fd = open(file_name, O_RDWR | O_TRUNC | O_CREAT | O_BINARY, S_IREAD | _S_IWRITE);
#endif
	if (fd < 0) {
		perror("Opening file error");
		fprintf(stderr, "File opening error for file %s\n", file_name);
		return -1;
	}
	int written = 0;
	int to_write = 4 * pImage->width * pImage->height;
	while (written < to_write) {
		int n = write(fd, pImage->pImageData, to_write - written);
		if (n < 1) {
			fprintf(stderr, "File write error for file %s\n", file_name);
			close(fd);
			return -1;
		}
		written += n;
	}
	close(fd);
	return 0;
}

int CVideoImage::SetImageName(u_int32_t id, const char* name)
{
	if (id >= m_max_images || !m_image_items[id] || (!name && !m_image_items[id]->name)) return -1;
	char* newname;

	// if name == NULL we delete the name
	if (!name) {
		free (m_image_items[id]->name);
		m_image_items[id]->name = NULL;
		return 0;
	}
	newname = strdup(name);
	if (!newname) return 1;
	trim_string(newname);
	// Free previous name if any
	if (m_image_items[id]->name) free (m_image_items[id]->name);
	m_image_items[id]->name = newname;
	return 0;
}

int CVideoImage::CreateImage(u_int32_t image_id, u_int8_t *source, u_int32_t width, u_int32_t height, const char* name) {
	image_item_t* pImage = NULL;
	int len = name ? strlen(name) + 1 : 1;

	// Check for valid image_id, image_items and scale
	if (image_id >= m_max_images || !m_image_items) return -1;

	// Delete destination image if it exist
	if (m_image_items[image_id]) DeleteImageItem(image_id);

	// Allocate place holder for destination image
	if (!(pImage = (struct image_item_t*) calloc(sizeof(struct image_item_t)+len,1))) {
		fprintf(stderr, "failed to allocate memory for image for image create.\n");
		return 1;
	}
	m_image_items[image_id] = pImage;
	pImage->file_name = ((char*) pImage) + sizeof(struct image_item_t);
	*(pImage->file_name) = '\0';
	if (name) strcpy(pImage->file_name, name);
	pImage->id = image_id;
	// pImage->name) = NULL;;	// implicit for calloc
	pImage->width = width;
	pImage->height = height;
	pImage->color_type = PNG_COLOR_TYPE_RGBA;
	pImage->bit_depth = 8;

	if (source) pImage->pImageData = source;
	else {
		// Allocate memory for image
		pImage->pImageData =
			(u_int8_t*) calloc(pImage->width*pImage->height*4,1);
		if (!pImage->pImageData) {
			fprintf(stderr, "Failed to allocate data for image.\n");
			goto free_and_return;
		}
		// Set alpha channel to 1.0 = 255
		u_int8_t* p = pImage->pImageData+3;	// 0=B 1=G 2=R 3=A
		for (u_int32_t row=0 ; row < pImage->height ; row++)
			for (u_int32_t col=0 ; col < pImage->width ; col++) {
				*p = 255;
				//if (row % 10 == 0) *(p-3) = 255;
				p += 4;
		}
	}
	return 0;
free_and_return:
	DeleteImageItem(image_id);
	return -1;
}

int CVideoImage::SetImageSource(u_int32_t image_id, source_enum_t source_type,
	u_int32_t source_id, u_int32_t offset_x, u_int32_t offset_y,
	u_int32_t width, u_int32_t height, double scale_x, double scale_y,
	double rotation, double alpha, cairo_filter_t filter) {
	image_item_t* pImage = NULL;
	u_int8_t *p = NULL, *src_data = NULL;
	u_int32_t src_width = 0, src_height = 0;

	CCairoGraphic* pCairoGraphic = NULL;

	// Check for valid image_id, image_items and scale
	if (image_id >= m_max_images || !m_image_items || scale_x <= 0.0 ||
		scale_y <= 0.0) return -1;

	// If source is image, check if it exist and that it is not copying itself
	// Also check that offset is within image
	if (source_type == SNOWMIX_SOURCE_IMAGE &&
		(source_id >= m_max_images || !m_image_items[source_id] ||
			source_id == image_id ||
			offset_x >= m_image_items[source_id]->width ||
			offset_y >= m_image_items[source_id]->height))
		return -1;

	// Delete destination image if it exist
	if (m_image_items[image_id]) DeleteImageItem(image_id);

	// Allocate place holder for destination image
	if (!(pImage = (struct image_item_t*) calloc(sizeof(struct image_item_t)+1,1))) {
		fprintf(stderr, "failed to allocate memory for image for source.\n");
		return 1;
	}
	pImage->file_name = ((char*) pImage) + sizeof(struct image_item_t);
	*(pImage->file_name) = '\0';
	pImage->id = image_id;
	// pImage->name) = NULL;;	// implicit for calloc


	// Is source an image?
	if (source_type == SNOWMIX_SOURCE_IMAGE) {
//fprintf(stderr, "Image Source %u image source %u\n", image_id, source_id);

		// Set source and source geometry
		image_item_t* pSource = m_image_items[source_id];
		src_data = pSource->pImageData;
		src_width = pSource->width;
		src_height = pSource->height;

	} else if (source_type == SNOWMIX_SOURCE_FEED) {
fprintf(stderr, "IMAGE SOURCE FEED\n");
		if (!m_pVideoMixer || !m_pVideoMixer->m_pVideoFeed) {
			fprintf(stderr, "Can not access m_pVideoFeed in CVideoImage::SetImageSource.\n");
			goto free_and_return;
		}
		src_data = m_pVideoMixer->m_pVideoFeed->GetFeedFrame(source_id,
			&src_width, &src_height);;
		if (!src_data || !src_width || !src_height) {
			fprintf(stderr, "Feed %u as source is not available in CVideoImage::SetImageSource.\n", source_id);
			goto free_and_return;
		}
	} else if (source_type == SNOWMIX_SOURCE_FRAME) {
		if (source_id != 0) {
			fprintf(stderr, "Only frame 0 can be copied from in CVideoImage::SetImageSource.\n");
			goto free_and_return;
		}
		if (!m_pVideoMixer || !m_pVideoMixer->m_pCairoGraphic || !m_pVideoMixer->m_overlay) {
			if (m_pVideoMixer) fprintf(stderr, "No videomixer frame available in CVideoImage::SetImageSource.\n");
			else fprintf(stderr, "No videomixer available in CVideoImage::SetImageSource.\n");
			goto free_and_return;
		}
fprintf(stderr, "Copying from frame\n");
		src_data = m_pVideoMixer->m_overlay;
		src_width = m_pVideoMixer->GetSystemWidth();
		src_height = m_pVideoMixer->GetSystemHeight();
	} else {
fprintf(stderr, "Deleting allocated image item\n");
		DeleteImageItem(image_id);
		return 1;
	}
	// Set destination geometry
	if (!width) width = src_width;
	if (!height) height = src_height;

	// Check if offset will reduce produced image
	if (offset_x + width > src_width)
		width = src_width - offset_x;
	if (offset_y + height > src_height)
		height = src_height - offset_y;

	// Check if scale changes the produced image size
	pImage->width = (typeof(pImage->width))(scale_x * width);
	pImage->height = (typeof(pImage->height))(scale_y * height);
	if (!(pImage->width) || !(pImage->height)) goto free_and_return;

	// Allocate memory for image
	pImage->pImageData =
		(u_int8_t*) calloc(pImage->width*pImage->height*4,1);
	if (!pImage->pImageData) {
		fprintf(stderr, "Failed to allocate data for image.\n");
		goto free_and_return;
	}
	pImage->color_type = PNG_COLOR_TYPE_RGBA;
	pImage->bit_depth = 8;

	// Set alpha channel to 1.0 = 255
	p = pImage->pImageData+3;	// 0=B 1=G 2=R 3=A
	for (u_int32_t row=0 ; row < pImage->height ; row++)
		for (u_int32_t col=0 ; col < pImage->width ; col++) {
			*p = 255;
			//if (row % 10 == 0) *(p-3) = 255;
			p += 4;
	}

	if (pImage && pImage->width && pImage->height) {
		// Create a cairo context for image creation
		if ((pCairoGraphic = new CCairoGraphic(pImage->width,
			pImage->height, pImage->pImageData))) {
			pCairoGraphic->OverlayFrame(src_data,
				src_width, src_height,	// src geometry
				0, 0,					// clip placement
				offset_x, offset_y,			// clip offset
				width, height,				// clip geometry
				0, 0,					// clip start
				0,							// align
				scale_x, scale_y,			// clip scale
				rotation, alpha, filter);		// rot, alpha, filter
			m_image_items[image_id] = pImage;
			delete pCairoGraphic;
		}
	}
	return m_image_items[image_id] ? 0 : 1;
free_and_return:
	DeleteImageItem(image_id);
	return -1;
}

FILE* CVideoImage::FileOpen(const char* file_name, const char* mode, char** open_name) {
	if (!file_name || !(*file_name)) return NULL;
	if (*file_name == '/') {
		if (m_verbose > 1) fprintf(stderr, "Image load opening absolut file name %s\n", file_name);
		return fopen(file_name, mode);
	}
	if (!m_pVideoMixer) return NULL;
	char *longname;
	const char *path;
	FILE* fp;
	int index = 0;
	int len = strlen(file_name);
	while ((path = m_pVideoMixer->GetSearchPath(index++))) {
		longname = (char*) malloc(strlen(path)+1+len+1);
		if (!longname) {
			fprintf(stderr, "Warning. Failed to allocate memory for longname.\n");
			return NULL;
		}
		sprintf(longname, "%s/%s", path, file_name);
		TrimFileName(longname);
if (m_verbose > 1) fprintf(stderr, "Trying to open %s for image load\n", longname);
		if ((fp = fopen(longname, mode))) {
			if (open_name) *open_name = longname;
			else free(longname);
			return fp;
		}
		free (longname);
	}
	return NULL;
}

		

// load image into image item list
int CVideoImage::LoadImage(u_int32_t image_id, const char* file_name)
{
	png_byte header[8];    // 8 is the maximum size that can be checked
	struct image_item_t* pImage = NULL;
	FILE* fp = NULL;
	char* longname = NULL;
	//int number_of_passes = 0;
	int n = 0, len;
	if (m_verbose)
		fprintf(stderr, "Load Image %u <%s>\n", image_id, file_name ?
			file_name : "no file name given");

	if (image_id >= m_max_images) {
		fprintf(stderr, "Image id %u larger than configured max %u\n", image_id, m_max_images-1);
		return -1;
	}

	if (!file_name) {
		DeleteImageItem(image_id);
		return 0;
	}

	// Open file using search path
	fp = FileOpen(file_name, "rb", &longname);
	if (!fp) {
		fprintf(stderr, "File %s could not be opened for reading for png\n", file_name);
		goto error_free_and_return;
	}

	// Test for PNG file
	n = fread(header, 1, 8, fp);
	if (n != 8 || png_sig_cmp(header, 0, 8)) {
		fprintf(stderr, "File %s is not recognized as a PNG file\n", file_name);
		goto error_free_and_return;
	}

	// Allocate image stuct for holding image
	len = strlen(longname ? longname : file_name);
	if (!(pImage = (struct image_item_t*) calloc(sizeof(struct image_item_t)+len+1,1))) {
		fprintf(stderr, "failed to allocate memory for image for png.\n");
		goto error_free_and_return;
	}
	pImage->id = image_id;
	pImage->file_name = ((char*) pImage) + sizeof(struct image_item_t);
	strcpy(pImage->file_name, longname ? longname : file_name);
	if (longname) {
		free(longname);
		longname=NULL;
	}

	/* open file and test for it being a png */
/*
	fp = fopen(file_name, "rb");
	if (!fp) {
		fprintf(stderr, "File %s could not be opened for reading for png\n", file_name);
		goto error_free_and_return;
	}
	n = fread(header, 1, 8, fp);
	if (n != 8 || png_sig_cmp(header, 0, 8)) {
		fprintf(stderr, "File %s is not recognized as a PNG file\n", file_name);
		goto error_free_and_return;
	}
*/


	/* initialize stuff */
	if (!(pImage->png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))) {
		fprintf(stderr, "Failed to create read structure for png file %s\n", file_name);
		goto error_free_and_return;
	}

	if (!(pImage->info_ptr = png_create_info_struct(pImage->png_ptr))) {
		fprintf(stderr, "Failed to create info structure for png file %s\n", file_name);
		goto error_free_and_return;
	}

	if (setjmp(png_jmpbuf(pImage->png_ptr))) {
		fprintf(stderr, "Error while init for png file %s\n", file_name);
		goto error_free_and_return;
	}

	png_init_io(pImage->png_ptr, fp);
	png_set_sig_bytes(pImage->png_ptr, 8);

	png_read_info(pImage->png_ptr, pImage->info_ptr);

	pImage->width = png_get_image_width(pImage->png_ptr, pImage->info_ptr);
	pImage->height = png_get_image_height(pImage->png_ptr, pImage->info_ptr);
	pImage->color_type = png_get_color_type(pImage->png_ptr, pImage->info_ptr);
	pImage->bit_depth = png_get_bit_depth(pImage->png_ptr, pImage->info_ptr);

	//number_of_passes = png_set_interlace_handling(pImage->png_ptr);
	png_set_interlace_handling(pImage->png_ptr);
	png_read_update_info(pImage->png_ptr, pImage->info_ptr);

	// Check Image size
	if (pImage->width == 0 || pImage->height == 0) {
		fprintf(stderr, "Image geometry for file %s is illegal %ux%u\n", file_name, pImage->width, pImage->height);
		goto error_free_and_return;
	}


	/* read file */
	if (setjmp(png_jmpbuf(pImage->png_ptr))) {
		fprintf(stderr, "Error during read_image for png file %s\n", file_name);
		goto error_free_and_return;
	}

	pImage->row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * pImage->height);
	for (unsigned int y=0; y<pImage->height; y++)
		pImage->row_pointers[y] = (png_byte*)
			malloc(png_get_rowbytes(pImage->png_ptr,pImage->info_ptr));

	png_read_image(pImage->png_ptr, pImage->row_pointers);

	pImage->pImageData =
			(u_int8_t*) malloc(pImage->width*pImage->height*4);
	if (pImage->pImageData) {
		u_int8_t* pTmp = pImage->pImageData;
		if (pImage->color_type == PNG_COLOR_TYPE_RGBA) {
			for (unsigned int iy=0; iy < pImage->height; iy++) {
				png_byte* row = pImage->row_pointers[iy];
				for (unsigned int ix=0; ix < pImage->width; ix++) {
					png_byte* ptr = &(row[ix*4]);
					*pImage->pImageData++ = ptr[2];
					*pImage->pImageData++ = ptr[1];
					*pImage->pImageData++ = ptr[0];
					*pImage->pImageData++ = ptr[3];
				}
				//free(pImage->row_pointers[iy]);
			}
		} else {	// Assume RGB
			for (unsigned int iy=0; iy < pImage->height; iy++) {
				png_byte* row = pImage->row_pointers[iy];
				for (unsigned int ix=0; ix < pImage->width; ix++) {
					png_byte* ptr = &(row[ix*3]);
					*pImage->pImageData++ = ptr[2];
					*pImage->pImageData++ = ptr[1];
					*pImage->pImageData++ = ptr[0];
					*pImage->pImageData++ = 0xff;
				}
				//free(pImage->row_pointers[iy]);
			}
		}
			pImage->pImageData = pTmp;
	}

	fclose(fp);
	if (pImage->row_pointers) {
		//free(pImage->row_pointers);
		//pImage->row_pointers = NULL;
	}
	if (m_verbose) fprintf(stderr, "Image %u created\n", image_id);

	if (m_image_items[image_id]) DeleteImageItem(image_id);
	m_image_items[image_id] = pImage;
	return 0;

error_free_and_return:
	if (pImage) {
		if (pImage->png_ptr) {
			png_destroy_read_struct(&(pImage->png_ptr), pImage->info_ptr ? &(pImage->info_ptr) : NULL, NULL);
		}
		free(pImage);
	}
	if (fp) fclose(fp);
	if (longname) free(longname);
	return -1;
}

int CVideoImage::OverlayImage(const u_int32_t place_id, u_int8_t* pY, const u_int32_t width,
	const u_int32_t height)
{
	struct image_place_t* pImagePlaced = GetImagePlaced(place_id);

	// look up image placed and check we have an image item
	if (!pImagePlaced || pImagePlaced->image_id >= m_max_images ||
		!m_image_items[pImagePlaced->image_id] || !pImagePlaced->display) return 0;

	int x = pImagePlaced->x;
	int y = pImagePlaced->y;
	u_int32_t image_id = pImagePlaced->image_id;	// Should be the same id

	// See if align is different from align left and top
	if (pImagePlaced->align &(~(SNOWMIX_ALIGN_LEFT | SNOWMIX_ALIGN_TOP))) {
		if (pImagePlaced->align & SNOWMIX_ALIGN_CENTER)
			x -= m_image_items[image_id]->width/2;
		else if (pImagePlaced->align & SNOWMIX_ALIGN_RIGHT)
			x -= m_image_items[image_id]->width;
		if (pImagePlaced->align & SNOWMIX_ALIGN_MIDDLE)
			y -= m_image_items[image_id]->height/2;
		else if (pImagePlaced->align & SNOWMIX_ALIGN_BOTTOM)
			y -= m_image_items[image_id]->height;
	}
fprintf(stderr, "Place image at %d,%d -> %d,%d\n", pImagePlaced->x, pImagePlaced->y, x,y);

	pY = pY + 4*(y*width+x);
	if (m_image_items[image_id]->pImageData) {
		u_int8_t* ptr = m_image_items[image_id]->pImageData;
		for (unsigned int iy=0; iy<m_image_items[image_id]->height; iy++) {
			u_int8_t* pY_tmp = pY + 4*width;
			for (unsigned int ix=0; ix<m_image_items[image_id]->width; ix++) {
				if (ptr[3]) {
					if (ptr[3] >= 255) {
						*pY++ = ptr[0];
						*pY++ = ptr[1];
						*pY++ = ptr[2];
						pY++;
					} else {
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[0]))*ptr[3]))/255;
						pY++;
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[1]))*ptr[3]))/255;
						pY++;
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[2]))*ptr[3]))/255;
						pY++;
						pY++;
					}
				} else pY += 4;
				ptr += 4;
			}
			pY = pY_tmp;
		}
	} else {
		fprintf(stderr, "Image without image data ???\n");
/*
		struct image_item_t* pImage = m_image_items[image_id];
		u_int8_t* p = pImage->pImageData =
			(u_int8_t*) malloc(pImage->width*pImage->height*4);
		for (unsigned int iy=0; iy < pImage->height; iy++) {
			//png_byte* row = m_image_items[image_id]->row_pointers[iy];
			png_byte* row = pImage->row_pointers[iy];
			u_int8_t* pY_tmp = pY + 4*width;
			for (unsigned int ix=0; ix < pImage->width; ix++) {
				png_byte* ptr = &(row[ix*4]);
				//printf("Pixel at position [ %d - %d ] has RGBA values: %d - %d - %d - %d\n",
				// x, y, ptr[0], ptr[1], ptr[2], ptr[3]);

				*p++ = ptr[2];
				*p++ = ptr[1];
				*p++ = ptr[0];
				*p++ = ptr[3];
				if (ptr[3]) {
					if (ptr[3] >= 255) {
						*pY++ = ptr[2];
						*pY++ = ptr[1];
						*pY++ = ptr[0];
						pY++;
					} else {
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[2]))*ptr[3]))/255; pY++;
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[1]))*ptr[3]))/255; pY++;
						*pY = (((u_int32_t)(*pY))*(255-ptr[3]))/255 +
							((((u_int32_t)(ptr[0]))*ptr[3]))/255; pY++;
						pY++;
					}
				} else pY += 4;
			}
			pY = pY_tmp;
		}
*/
	}
	return 0;
}
