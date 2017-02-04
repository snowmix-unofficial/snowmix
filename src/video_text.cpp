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
//#include <limits.h>
#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
//#include <sys/types.h>
#ifdef WIN32
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#ifndef M_PI
#error M_PI not defined
#endif

#include <string.h>

#include <cairo/cairo.h>

#include "snowmix.h"
#include "video_mixer.h"
#include "video_text.h"
#include "snowmix_util.h"

//#include "video_scaler.h"
//#include "add_by.h"
#include <pango/pangocairo.h>

CTextItems::CTextItems(CVideoMixer* pVideoMixer, u_int32_t max_strings, u_int32_t max_fonts, u_int32_t max_places) {
	m_pVideoMixer = pVideoMixer;
	m_text_strings = NULL;
	m_text_strings_multibyte = NULL;
	m_list_text_item_head = NULL;
	m_list_text_item_tail = NULL;
	m_max_strings = 0;
	m_max_fonts = 0;
	m_max_places = 0;
	//m_width = m_pVideoMixer ? m_pVideoMixer->GetSystemWidth() : 0;
	//m_height = m_pVideoMixer ? m_pVideoMixer->GetSystemHeight() : 0;
	if (max_strings < 1 || max_fonts < 1 || max_places < 1) return;
	m_text_strings = (char**) calloc(sizeof(char*), max_strings);
	m_text_strings_multibyte = (bool*) calloc(sizeof(bool), max_strings);
	m_text_strings_seqno = (u_int16_t*) calloc(sizeof(u_int16_t), max_strings);
	m_fonts = (char**) calloc(sizeof(char*), max_fonts);
	m_pDesc = (PangoFontDescription**) calloc(sizeof(PangoFontDescription*), max_fonts);
	m_fonts_seqno = (u_int16_t*) calloc(sizeof(u_int16_t), max_fonts);
	m_text_places = (text_item_t**) calloc(sizeof(text_item_t*), max_places);

	m_max_strings = m_text_strings ? max_strings : 0;
	m_max_fonts = m_fonts ? max_fonts : 0;
	m_max_places = m_text_places ? max_places : 0;
	m_string_count = 0;
	m_font_count = 0;
	m_placed_count = 0;
	m_verbose = 0;
	if (!m_pVideoMixer) fprintf(stderr, "Class CTextItems created with pVideoMixer NULL. Will fail\n");
}

CTextItems::~CTextItems() {
	if (m_text_strings) {
		for (u_int32_t i=0;  i < m_max_strings; i++) {
			if (m_text_strings[i]) free (m_text_strings[i]);
			m_text_strings[i] = NULL;		// just for safe keeping
		}
		free(m_text_strings);
	}
	if (m_fonts) {
		for (u_int32_t i=0;  i < m_max_fonts; i++) {
			if (m_fonts[i]) free (m_fonts[i]);
			m_fonts[i] = NULL;			// just for safe keeping
		}
		free(m_fonts);
	}
	if (m_fonts_seqno) free(m_fonts_seqno);
	m_fonts_seqno = NULL;
	if (m_pDesc) {
		for (u_int32_t i=0;  i < m_max_fonts; i++) {
			if (m_pDesc[i]) pango_font_description_free(m_pDesc[i]);
			m_pDesc[i] = NULL;			// just for safe keeping
		}
		free(m_pDesc);
	}
	if (m_text_places) {
		for (u_int32_t i=0;  i < m_max_places; i++) {
			DeleteTextRoller(i);
			if (m_text_places[i]) free (m_text_places[i]);
			m_text_places[i] = NULL;			// just for safe keeping
		}
		free(m_text_places);
	}
}

int CTextItems::ParseCommand(CController* pController, struct controller_type* ctr, const char* str)
{
	if (!pController) return 0;
	while (isspace(*str)) str++;
	if (!strncasecmp (str, "overlay ", 8)) {
		if (m_pVideoMixer->OverlayText(str + 8, m_pVideoMixer->m_pCairoGraphic))
			goto wrong_param_no;
	}
	// text align [<id no> (left | center | right) (top | middle | bottom)]
	else if (!strncasecmp (str, "align ", 6)) {
		if (set_text_place_align(ctr, pController, str+6)) goto wrong_param_no;
	}
	// text alpha [<id no> <alpha>]
	else if (!strncasecmp (str, "alpha ", 6)) {
		if (set_text_place_alpha(ctr, pController, str+6)) goto wrong_param_no;
	}
	// text anchor [<id no> (n | s | e | w | c | ne | nw | se | sw)]
	else if (!strncasecmp (str, "anchor ", 7)) {
		if (set_text_place_anchor(ctr, pController, str+7)) goto wrong_param_no;
	}
	// text backgr
	else if (!strncasecmp (str, "backgr ", 7)) {
		str += 7;
		while (isspace(*str)) str++;
		// text backgr alpha [<place id> <alpha>]
		if (!strncasecmp (str, "alpha ", 6)) {
			if (set_text_place_backgr_alpha (ctr, pController, str+6))
				goto wrong_param_no;
		}
		// text backgr clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
		else if (!strncasecmp (str, "clip ", 5)) {
			if (set_text_place_backgr_clip (ctr, pController, str+5))
				goto wrong_param_no;
		}
		// text backgr round [<place id> <left top> <right top> <left bottom> <right bottom>]
		else if (!strncasecmp (str, "round ", 6)) {
			if (set_text_place_backgr_round (ctr, pController, str+5))
				goto wrong_param_no;
		}
		// text backgr linpat <place_id> [<fraction> <red> <green> <blue> <alpha>]"
		else if (!strncasecmp (str, "linpat ", 7)) {
			if (set_text_place_backgr_linpat(ctr, pController, str + 7))
				goto wrong_param_no;
		}
		// text backgr move alpha [<id no> <delta alpha> <alpha steps> ] 
		else if (!strncasecmp (str, "move alpha ", 11)) {
			if (set_text_place_backgr_move_alpha (ctr, pController, str+11))
				goto wrong_param_no;
			}
		// text backgr move clip [<place id> <delta left> <delta right> <delta top>
		//	<delta bottom>  <left steps> <right steps> <top steps> <bottom steps>]
		else if (!strncasecmp (str, "move clip ", 10)) {
			if (set_text_place_backgr_move_clip (ctr, pController, str+10))
				goto wrong_param_no;
		}
		// text backgr rgb [<place id> [ <red> <green> blue> [ <alpha> ] ]
		else if (!strncasecmp (str, "rgb ", 4)) {
			if (set_text_place_backgr_rgb (ctr, pController, str+4))
				goto wrong_param_no;
		}
		// text backgr [<id no> [ <l_pad> <r_pad> <t_pad> <b_pad> [<red> <green> 
		//			<blue> <alpha>]]]
		else {
			if (set_text_place_background (ctr, pController, str)) goto wrong_param_no;
		}
	}
	// text clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
	else if (!strncasecmp (str, "clip ", 5)) {
		if (set_text_place_clip (ctr, pController, str+5)) goto wrong_param_no;
	}
	// text clipabs [<place id> <clip x> <clip y> <clip width> <clip height>]
	else if (!strncasecmp (str, "clipabs ", 8)) {
		if (set_text_place_clipabs (ctr, pController, str+8)) goto wrong_param_no;
	}
	// text coor <place_id> <x> <y>
	else if (!strncasecmp (str, "coor ", 5)) {
		if (set_text_place_coor (ctr, pController, str+5)) goto wrong_param_no;
	}
	// text grow <place id> <grow> [<grow delta> [variable]]
	else if (!strncasecmp (str, "grow ", 5)) {
		if (set_text_place_grow(ctr, pController, str+5)) goto wrong_param_no;
	}
	// text move
	else if (!strncasecmp (str, "move ", 5)) {
		str += 5;
		// text move alpha [<place id> <delta_alpha> <step_alpha>]
		if (!strncasecmp (str, "alpha ", 6)) {
			if (set_text_place_move_alpha (ctr, pController, str+6))
				goto wrong_param_no;
		}
		// text move clip [<id no> <delta left> <delta right> <delta top> <delta bottom>
        	//                <left steps> <right steps> <top steps> <bottom steps>]
		else if (!strncasecmp (str, "clip ", 5)) {
			if (set_text_place_move_clip (ctr, pController, str+5))
				goto wrong_param_no;
		}
		// text move scale [<place id> <delta_scale_x> <delta_scale_y> <step_x> <step_y>]
		else if (!strncasecmp (str, "scale ", 6)) {
			if (set_text_place_move_scale (ctr, pController, str+6))
				goto wrong_param_no;
		}
		// text move coor [<place id> <delta_x> <delta_y> <step_x> <step_y>]
		else if (!strncasecmp (str, "coor ", 5)) {
			if (set_text_place_move_coor (ctr, pController, str+5))
				goto wrong_param_no;
		}
		// text move rotation [<place id> <delta_rotation> <step_rotation>]
		else if (!strncasecmp (str, "rotation ", 9)) {
			if (set_text_place_move_rotation (ctr, pController, str+9))
				goto wrong_param_no;
		} else goto wrong_param_no;
	}
	// text offset <place id> <offset_x> <offset y>
	else if (!strncasecmp (str, "offset ", 7)) {
		if (set_text_place_offset(ctr, pController, str+7)) goto wrong_param_no;
	}
	// text repeat move <id no> <dx> <dy> <end x> <end y>
	else if (!strncasecmp (str, "repeat move ", 12)) {
		if (set_text_place_repeat_move(ctr, pController, str+12)) goto wrong_param_no;
	}
	// text rgb [<id no> <red> <green> <blue>]
	else if (!strncasecmp (str, "rgb ", 4)) {
		if (set_text_place_rgb(ctr, pController, str+4)) goto wrong_param_no;
		}
	// text rotation [<id no> <rotation>]
	else if (!strncasecmp (str, "rotation ", 9)) {
		if (set_text_place_rotation(ctr, pController, str+9)) goto wrong_param_no;
	}
	// text scale [<id no> <scale_x> <scale_y>]
	else if (!strncasecmp (str, "scale ", 6)) {
		if (set_text_place_scale(ctr, pController, str+6)) goto wrong_param_no;
	}
	// text place string [<id no> <string id>]
	else if (!strncasecmp (str, "place string ", 13)) {
		if (set_text_place_string(ctr, pController, str+13))
			goto wrong_param_no;
	}
	// text place font [<id no> <string id>]
	else if (!strncasecmp (str, "place font ", 11)) {
		if (set_text_place_font(ctr, pController, str+11))
			goto wrong_param_no;
	}
	else if (!strncasecmp (str, "info ", 5)) {
		if (set_text_place_info (ctr, pController, str+5)) goto wrong_param_no;
	}

	// text place >>>>> NOTE : text place some_command is obsolete in 0.5.0 except
	//    "text place string" "text place font" "text place info" and "text place"
	else if (!strncasecmp (str, "place ", 6)) {
		str += 6;
		while (isspace(*str)) str++;

//		// text place align [<id no> (left | center | right) (top | middle | bottom)]
//		if (!strncasecmp (str, "align ", 6)) {
//			if (set_text_place_align(ctr, pController, str+6)) goto wrong_param_no;
//		}
//		// text place alpha [<id no> <alpha>]
//		else if (!strncasecmp (str, "alpha ", 6)) {
//			if (set_text_place_alpha(ctr, pController, str+6)) goto wrong_param_no;
//		}
//		// text place anchor [<id no> (n | s | e | w | c | ne | nw | se | sw)]
//		else if (!strncasecmp (str, "anchor ", 7)) {
//			if (set_text_place_anchor(ctr, pController, str+7)) goto wrong_param_no;
//		}
//		// text place backgr
//		else if (!strncasecmp (str, "backgr ", 7)) {
//			str += 7;
//			while (isspace(*str)) str++;
//			// text place backgr alpha [<place id> <alpha>]
//			if (!strncasecmp (str, "alpha ", 6)) {
//				if (set_text_place_backgr_alpha (ctr, pController, str+6)) goto wrong_param_no;
//			}
//			// text place backgr clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
//			else if (!strncasecmp (str, "clip ", 5)) {
//				if (set_text_place_backgr_clip (ctr, pController, str+5)) goto wrong_param_no;
//			}
//			// text place backgr round [<place id> <left top> <right top> <left bottom> <right bottom>]
//			else if (!strncasecmp (str, "round ", 6)) {
//				if (set_text_place_backgr_round (ctr, pController, str+5)) goto wrong_param_no;
//			}
//			// text place backgr linpat <place_id> [<fraction> <red> <green> <blue> <alpha>]"
//			else if (!strncasecmp (str, "linpat ", 7)) {
//				if (set_text_place_backgr_linpat(ctr, pController, str + 7)) goto wrong_param_no;
//			}
//			// text place backgr move alpha [<id no> <delta alpha> <alpha steps> ] 
//			else if (!strncasecmp (str, "move alpha ", 11)) {
//				if (set_text_place_backgr_move_alpha (ctr, pController, str+11)) goto wrong_param_no;
//			}
//			// text place backgr move clip [<place id> <delta left> <delta right> <delta top>
//			//	<delta bottom>  <left steps> <right steps> <top steps> <bottom steps>]
//			else if (!strncasecmp (str, "move clip ", 10)) {
//				if (set_text_place_backgr_move_clip (ctr, pController, str+10)) goto wrong_param_no;
//			}
//			// text place backgr rgb [<place id> [ <red> <green> blue> [ <alpha> ] ]
//			else if (!strncasecmp (str, "rgb ", 4)) {
//				if (set_text_place_backgr_rgb (ctr, pController, str+4)) goto wrong_param_no;
//			}
//			// text place backgr [<id no> [ <l_pad> <r_pad> <t_pad> <b_pad> [<red> <green> 
//			//			<blue> <alpha>]]]
//			else {
//				if (set_text_place_background (ctr, pController, str)) goto wrong_param_no;
//			}
//		}
//		// text place clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
//		else if (!strncasecmp (str, "clip ", 5)) {
//			if (set_text_place_clip (ctr, pController, str+5)) goto wrong_param_no;
//		}
//		// text place clipabs [<place id> <clip x> <clip y> <clip width> <clip height>]
//		else if (!strncasecmp (str, "clipabs ", 8)) {
//			if (set_text_place_clipabs (ctr, pController, str+8)) goto wrong_param_no;
//		}
//		// text place coor <place_id> <x> <y>
//		else if (!strncasecmp (str, "coor ", 5)) {
//			if (set_text_place_coor (ctr, pController, str+5)) goto wrong_param_no;
//		}
//		// text place grow <place id> <grow> [<grow delta> [variable]]
//		else if (!strncasecmp (str, "grow ", 5)) {
//			if (set_text_place_grow(ctr, pController, str+5)) goto wrong_param_no;
//		}
//		// text place move
//		else if (!strncasecmp (str, "move ", 5)) {
//			str += 5;
//			// text place move alpha [<place id> <delta_alpha> <step_alpha>]
//			if (!strncasecmp (str, "alpha ", 6)) {
//				if (set_text_place_move_alpha (ctr, pController, str+6)) goto wrong_param_no;
//			}
//			// text place move clip [<id no> <delta left> <delta right> <delta top> <delta bottom>
//       		//                <left steps> <right steps> <top steps> <bottom steps>]
//			else if (!strncasecmp (str, "clip ", 5)) {
//				if (set_text_place_move_clip (ctr, pController, str+5)) goto wrong_param_no;
//			}
//			// text place move scale [<place id> <delta_scale_x> <delta_scale_y> <step_x> <step_y>]
//			else if (!strncasecmp (str, "scale ", 6)) {
//				if (set_text_place_move_scale (ctr, pController, str+6)) goto wrong_param_no;
//			}
//			// text place move coor [<place id> <delta_x> <delta_y> <step_x> <step_y>]
//			else if (!strncasecmp (str, "coor ", 5)) {
//				if (set_text_place_move_coor (ctr, pController, str+5)) goto wrong_param_no;
//			}
//			// text place move rotation [<place id> <delta_rotation> <step_rotation>]
//			else if (!strncasecmp (str, "rotation ", 9)) {
//				if (set_text_place_move_rotation (ctr, pController, str+9)) goto wrong_param_no;
//			} else goto wrong_param_no;
//		}
//		// text place offset <place id> <offset_x> <offset y>
//		else if (!strncasecmp (str, "offset ", 7)) {
//			if (set_text_place_offset(ctr, pController, str+7)) goto wrong_param_no;
//		}
//		// text place repeat move <id no> <dx> <dy> <end x> <end y>
//		else if (!strncasecmp (str, "repeat move ", 12)) {
//			if (set_text_place_repeat_move(ctr, pController, str+12)) goto wrong_param_no;
//		}
//		// text place rgb [<id no> <red> <green> <blue>]
//		else if (!strncasecmp (str, "rgb ", 4)) {
//			if (set_text_place_rgb(ctr, pController, str+4)) goto wrong_param_no;
//		}
//		// text place rotation [<id no> <rotation>]
//		else if (!strncasecmp (str, "rotation ", 9)) {
//			if (set_text_place_rotation(ctr, pController, str+9)) goto wrong_param_no;
//		}
//		// text place scale [<id no> <scale_x> <scale_y>]
//		else if (!strncasecmp (str, "scale ", 6)) {
//			if (set_text_place_scale(ctr, pController, str+6)) goto wrong_param_no;
//		}
		// text place string [<id no> <string id>]
		if (!strncasecmp (str, "string ", 7)) {
			if (set_text_place_string(ctr, pController, str+7)) goto wrong_param_no;
		}
		// text place font [<id no> <font id>]
		else if (!strncasecmp (str, "font ", 5)) {
			if (set_text_place_font(ctr, pController, str+5)) goto wrong_param_no;
		}
		// text place info
		else if (!strncasecmp (str, "info ", 5)) {
			if (set_text_place_info (ctr, pController, str+5)) goto wrong_param_no;
		}
		// text place [<place_id> [<text_id> <font_id> <x> <y> [<red> <green> <blue>
		//			  [ <alpha> [ nw | ne | se | sw | n | s | e | w | c ]]]]]
		else {
			if (set_text_place (ctr, pController, str)) goto wrong_param_no;
		}
	}
	// text roller [<roller id> [ <text place id> <w> <h> <delta x> <delta y> ]]
	else if (!strncasecmp (str, "roller ", 7)) {
		if (set_text_roller(ctr, pController, str+7)) goto wrong_param_no;
		// text roller place [<roller id> [ <text place id> <w> <h> <delta x> <delta y> ]]
	}
	// text string concat <dest string id> <string id> [<string id> ...]
	else if (!strncasecmp (str, "string concat ", 14)) {
		if (set_text_string_concat (ctr, pController, str+14)) goto wrong_param_no;
	}
	else if (!strncasecmp (str, "string ", 7)) {
		if (set_text_string (ctr, pController, str+7)) goto wrong_param_no;
	}
	else if (!strncasecmp (str, "font available ", 15)) {
		if (list_text_fonts_available (ctr, pController, str+15)) goto wrong_param_no;
	}
	else if (!strncasecmp (str, "font ", 5)) {
		if (set_text_font (ctr, pController, str+5)) goto wrong_param_no;
	}
	else if (!strncasecmp (str, "verbose ", 8)) {
		if (set_text_verbose (ctr, pController, str+8)) goto wrong_param_no;
	}
	else if (!strncasecmp (str, "help ", 5)) {
		if (list_help (ctr, pController, str+5)) goto wrong_param_no;
	} else return 1;
	return 0;
wrong_param_no:
	return -1;
}

// text help
int CTextItems::list_help(struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!pController) return -1;
	pController->controller_write_msg (ctr, "MSG: Commands:\n"
		"MSG:  text align [<place id> (left | center | right) "
			"(top | middle | bottom) [<rotation>]\n"
		"MSG:  text alpha [<place id> <alpha>]\n"
		"MSG:  text anchor [<place id> "ANCHOR_SYNTAX"]\n"
		"MSG:  text backgr alpha [<place id> <alpha>]\n"
		"MSG:  text backgr clip [<place id> <clip left> "
			"<clip right> <clip top> <clip bottom>]\n"
		"MSG:  text backgr linpat [<place_id> [<fraction> <red> <green> "
			"<blue> <alpha>] [ v | h]]\n"
		"MSG:  text backgr move alpha [<place id> <delta alpha> <alpha steps>]\n"
		"MSG:  text backgr move clip [<place id> <delta left> <delta right>"
			" <delta top> <delta bottom> <left steps> <right steps> "
			"<top steps> <bottom steps>]\n"
		"MSG:  text backgr rgb [<place id> [ <red> <green> blue> [ <alpha> ] ]\n"
		"MSG:  text backgr round [<place id> <left top> <right top> "
			"<left bottom> <right bottom>]\n"
		"MSG:  text backgr [<place id> [ <l_pad> <r_pad> <t_pad> "
			"<b_pad> [<red> <green> <blue> <alpha>]]]  "
			"// <place id> only deletes backgr entry\n"
		"MSG:  text clip [<place id> <clip left> <clip right> "
			"<clip top> <clip bottom>]\n"
		"MSG:  text clipabs [<place id> <clip x> <clip y> "
			"<clip width> <clip height>]\n");
	pController->controller_write_msg (ctr,
		"MSG:  text coor [<place_id> <x> <y>]\n"
		"MSG:  text font [<font id> [<font description>]]  // empty string deletes 2"
			"entry\n"
		"MSG:  text font available\n"
		"MSG:  text overlay (<place id> | <place id>..<place id> | all | end | <place id>..end) "
			"[ ... ]\n"
		"MSG:  text place font [<place id> <font id>]\n"
		"MSG:  text place string [<place id> <string id>]\n"
		"MSG:  text grow <place id> <start len> [<delta grow> [variable]]\n"
		"MSG:  text move alpha [<place id> <delta_alpha> <step_alpha>]\n"
		"MSG:  text move clip [<place id> <delta left> <delta right> "
			"<delta top> <delta bottom> <left steps> <right steps> "
			"<top steps> <bottom steps>]\n"
		"MSG:  text move coor [<place id> [<delta_x> <delta_y> "
			"<step_x> <step_y>]]  // id only removes move entry\n"
		"MSG:  text move rotation [<place id> <delta_rotation> "
			"<step_rotation>]\n"
		"MSG:  text move scale [<place id> <delta_scale_x> "
			"<delta_scale_y> <step_x> <step_y>]\n"
		"MSG:  text offset [<place id> <offset x> <offset y>]\n"
		"MSG:  text place [<place id> [<string_id> <font_id> <x> <y> "
			"[<red> <green> <blue> "
			"[<alpha> ["ANCHOR_SYNTAX"]]]]]  "
			"// <place id> only deletes entry\n"
		"MSG:  text repeat move [<place id> <dx> <dy> <end x> <end y>]\n"
		"MSG:  text rgb [<place id> <red> <green> <blue>]\n"
		"MSG:  text rotation [<place id> <rotation>]\n"
		"MSG:  text scale [<place id> <scale_x> <scale_y>]\n"
		"MSG:  text string [<string id> [<string>]]  // empty string "
			"deletes entry\n"
		"MSG:  text string concat <dest string id> <string id> [<string id> ...]\n"
		"MSG:  text verbose [<verbose level>]\n"
		"MSG:  text help     // This help list\n"
		);
	return 0;
}

// text verbose [<level>] 
int CTextItems::set_text_verbose(struct controller_type *ctr, CController* pController, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (m_verbose) m_verbose = 0;
		else m_verbose = 1;
		if (pController)
			pController->controller_write_msg (ctr,
				STATUS"text verbose %s\n", m_verbose ? "on" : "off");
		return 0;
	}
	while (isspace(*str)) str++;
	if (sscanf(str, "%u", &m_verbose) != 1) return -1;
	if (m_verbose && pController)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			MESSAGE"text verbose level set to %u for CTextItems\n",
			m_verbose);
	return 0;
}

// text place info
int CTextItems::set_text_place_info (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!pController || !ci) return -1;
	text_item_t* p = NULL;
	pController->controller_write_msg (ctr, STATUS" text place info\n"
		STATUS" placed texts     : %u\n"
		STATUS" max placed texts : %u\n"
		STATUS" verbose level    : %u\n"
		STATUS" text place id : str_id font_id pos offset wxh align anchor scale rot col clip pad colbg clipbg\n",
		m_placed_count, m_max_places, m_verbose);
	for (unsigned int id = 0; id < MaxPlaces(); id++) {
		if ((p = GetTextPlace(id))) {
			pController->controller_write_msg (ctr, STATUS"text place %u : "
				"%u %u %d,%d %d,%d %ux%u %s,%s %d,%d %.3lf,%.3lf "
				"%.3lf %.3lf,%.3lf,%.3lf,%.3lf "
				"%.3lf,%.3lf,%.3lf,%.3lf "
				"%d,%d,%d,%d "
				"%.3lf,%.3lf,%.3lf,%.3lf "
				"%.3lf,%.3lf,%.3lf,%.3lf + linpat\n",
				id, p->text_id, p->font_id, p->x, p->y, p->offset_x, p->offset_y, p->w, p->h,
				p->align & SNOWMIX_ALIGN_LEFT ? "left" :
				  p->align & SNOWMIX_ALIGN_RIGHT ? "right" :
				    p->align & SNOWMIX_ALIGN_CENTER ? "center" : "unknown",
				p->align & SNOWMIX_ALIGN_TOP ? "top" :
				  p->align & SNOWMIX_ALIGN_BOTTOM ? "bottom" :
				    p->align & SNOWMIX_ALIGN_MIDDLE ? "middle" : "unknown",
				p->anchor_x, p->anchor_y, p->scale_x, p->scale_y,
				p->rotate, p->red, p->green, p->blue, p->alpha,
				p->clip_left, p->clip_right, p->clip_top, p->clip_bottom,
				p->pad_left, p->pad_right, p->pad_top, p->pad_bottom,
				p->red_bg, p->green_bg, p->blue_bg, p->alpha_bg,
				p->clip_bg_left, p->clip_bg_right, p->clip_bg_top, p->clip_bg_bottom);
		}
	}
	pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}

int CTextItems::set_text_roller (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci || !m_text_places) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		for (u_int32_t id=0; id < m_max_places; id++) if (m_text_places[id]) {
			if (!m_text_places[id]->pRollerText) continue;
			pController->controller_write_msg (ctr, STATUS
				"text roller %u at %d,%d WxH %ux%u delta %d,%d\n",
				id, m_text_places[id]->x, m_text_places[id]->y,
				m_text_places[id]->pRollerText->width, m_text_places[id]->pRollerText->height,
				m_text_places[id]->pRollerText->delta_x, m_text_places[id]->pRollerText->delta_y);
		}
		pController->controller_write_msg (ctr, STATUS"\n");
		return 0;
	}
	int32_t delta_x, delta_y, n;
	u_int32_t id, width, height;
	n = sscanf(ci, "%u %u %u %d %d", &id, &width, &height, &delta_x, &delta_y);
	if ( n == 1) return DeleteTextRoller(id);
	if (n == 5) return AddTextRoller(id, width, height, delta_x, delta_y);
	return -1;
}

// Query type 0=string 1=font 2=places 3=move 4=backgr
// format	: format 
// ids		: list of IDs
// maxid	: max number of ID
// nextavail	: Next available ID
// id or ids	: listing each ID
char* CTextItems::Query(const char* query, bool tcllist)
{
	char *pstr, *retstr = NULL;
	char *linpatstr = NULL;
	const char *str = NULL;
	linpat_t* pLinpat;
	int type, len = 0;

	if (!query) return NULL;
	while (isspace(*query)) query++;
	if (!(strncasecmp("string ", query, 7))) {
		query += 7; type = 0;
	}
       	else if (!(strncasecmp("font ", query, 5))) {
		query += 5; type = 1;
	}
	else if (!(strncasecmp("place ", query, 6))) {
		query += 6; type = 2;
	}
       	else if (!(strncasecmp("move ", query, 5))) {
		query += 5; type = 3;
	}
       	else if (!(strncasecmp("backgr ", query, 7))) {
		query += 7; type = 4;
	}
       	else if (!(strncasecmp("linpat ", query, 7))) {
		query += 7; type = 5;
	}
       	else if (!(strncasecmp("syntax", query, 6))) {
		return strdup("text (string | font | place | move | "
                        "backgr | linpat | syntax) (format | ids | maxid | nextavail | <id_list>)");
	} else return NULL;

	u_int32_t maxid = (type == 0 ? MaxStrings() : type == 1 ? MaxFonts() : MaxPlaces());
	while (isspace(*query)) query++;

	if (!strncasecmp(query, "format", 6)) {
		if (type == 0) {
			if (tcllist) retstr = strdup("id text");
			else retstr = strdup("text string <id> <<string text>>");
		}
		else if (type == 1) {
			if (tcllist) retstr = strdup("id font");
			else retstr = strdup("text font <id> <<font>>");
		}
		else if (type == 2) {
			if (tcllist) retstr = strdup("place_id text_id font_id coor offset "
				"geometry align anchor scale rotate rgba clip clipabs grow");
			else retstr = strdup("text place <place_id> <text_id> <font_id> <coor> <offset> "
				"<geometry> <align> <anchor> <scale> <rotate> <rgba> <clip> <clipabs> <grow>");
		}
		else if (type == 3) {
			// text move - place_id coor_move scale_move rotate_move alpha_move clip_move
			if (tcllist) retstr = strdup("place_id ");
			else retstr = strdup("text move <place_id> ");
		}
		else if (type == 4) {
			// text backgr - place_id bg_enabled pad round rbga bg_grow bg_move
			if (tcllist) retstr = strdup("place_id bg_enabled pad round rbga bg_grow bg_move");
			else retstr = strdup("text backgr <place_id> <bg_enabled> <pad> <round> <rbga> <bg_grow> <bg_move>");
		}
		else if (type == 5) {
			// text linpat - place_id direction fraction red green blue alpha
			if (tcllist) retstr = strdup("place_id direction fraction red green blue alpha");
			else retstr = strdup("text backgr <place_id> <direction> <fraction <red> <green> <blue> <alpha>");
		}
		else {
		}
		if (!retstr) goto malloc_failed;
		return retstr;
	}
	else if (!strncasecmp(query, "maxid", 5)) {
		len = (int)(log10((double)maxid)) + 3;	// in case of rounding error
		char* retstr = (char*) malloc(len);
		if (!retstr) goto malloc_failed;
		if (maxid) sprintf(retstr, "%u", maxid-1);
		else {
			free(retstr);
			retstr=NULL;
		}
		return retstr;
	}
	else if (!strncasecmp(query, "nextavail", 9)) {
		u_int32_t from = 0;
		query += 9;
		while (isspace(*query)) query++;
		sscanf(query,"%u", &from);	// get "from" if it is there
		
		len = (int)(log10((double)maxid)) + 3;	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) goto malloc_failed;
		*str = '\0';
		for (u_int32_t i = from; i < maxid ; i++) {
			if ((type == 0 && !m_text_strings[i]) || (type == 1 && !m_fonts[i]) ||
				(type == 2 && !m_text_places[i])) {
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
		len = (int)(0.5+log10((double)maxid)) + 1;
		len *= maxid;
		char* str = (char*) malloc(len+1);
		if (!str) fprintf(stderr, "CTextItems::Query failed to allocate memory\n");
		else {
			*str = '\0';
			char* p = str;
			for (u_int32_t i = 0; i < maxid ; i++) {
				if ((type == 0 && !m_text_strings[i]) ||
				    (type == 1 && !m_fonts[i]) ||
				    (type > 1 && !m_text_places[i]) ||
				    (type == 4 && !m_text_places[i]->background) ||
				    (type == 5 && !m_text_places[i]->linpat_h && !m_text_places[i]->linpat_v))
					continue;
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
		while (from <= to) {
			// Check that we have type and a string
			if ((type == 0 && !m_text_strings[from]) ||
			    (type == 1 && !m_fonts[from]) ||
			    (type > 1 && !m_text_places[from]) ||
			    (type == 4 && !m_text_places[from]->background) ||
			    (type == 5 && !m_text_places[from]->linpat_h && !m_text_places[from]->linpat_v)) {
				from++;
				continue;
			}
			if (type < 2)
				len += (17 + ((int)log10((double)(from+1))) + strlen(type == 0 ?
					m_text_strings[from] : m_fonts[from]));
			else if (type < 5) len += 256;
			else {		// Check that we have linpats
				int linpats = 0;
				pLinpat = m_text_places[from]->linpat_h;
				while (pLinpat) { linpats++; pLinpat = pLinpat->next; }
				pLinpat = m_text_places[from]->linpat_v;
				while (pLinpat) { linpats++; pLinpat = pLinpat->next; }
				len +=  (25 + 40*linpats);
				// {{place_id direction fraction red green blue alpha} { ...}}
				// text linpat <place_id> <direction> <fraction> <red> <green> <blue> <alpha>
				// 123456789012 +9         +2          +8         +8     +8     +8     +8     +1
			}
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	if (len < 2) {
		fprintf(stderr, "CTextItems::Query malformed query\n");
		return NULL;
	}
	retstr = (char*) malloc(len+4);
	if (!retstr) goto malloc_failed;
	*retstr = '\0';
	str = query;
	pstr = retstr;
	while (*str) {
		u_int32_t from, to;
		const char* nextstr = GetIdRange(str, &from, &to, 0, maxid ? maxid-1 : 0);
		if (!nextstr) return NULL;
		while (from <= to) {
			if ((type == 0 && m_text_strings[from]) || (type == 1 && m_fonts[from])) {
				if (tcllist) sprintf(pstr, "{%u {%s}} ", from, type == 0 ?
					m_text_strings[from] : m_fonts[from]);
				else sprintf(pstr, "text %s %u <%s>\n", type == 0 ? "string" : "font",
						from, type == 0 ? m_text_strings[from] : m_fonts[from]);
			}
			else if (type > 1 && m_text_places[from]) {
				text_item_t* pt = m_text_places[from];

				// text place <place_id> <text_id> <font_id> <coor> <offset> <geometry>
				//	<align> <anchor> <scale> <rotate> <rgba> <clip>
				if (type == 2) sprintf(pstr, (tcllist ?
					  "{%u %u %u {%d %d} {%d %d} {%u %u} {%s %s} {%d %d} {%.9lf %0.9lf} %.9lf "
					    "{%.5lf %.5lf %.5lf %.5lf} {%.5lf %.5lf %.5lf %.5lf} {%d %d %d %d} {%d %u}} " :
					  "text place %u %u %u %d,%d %d,%d %ux%u %s,%s %d,%d %.9lf,%0.9lf %.9lf "
					    "%.9lf,%.9lf,%.9lf,%.9lf %.5lf,%.5lf,%.5lf,%.5lf %d,%d,%d,%d %d,%u\n"),
					from, pt->text_id, pt->font_id, pt->x, pt->y,
					pt->offset_x, pt->offset_y, pt->w, pt->h,
					pt->align & SNOWMIX_ALIGN_LEFT ? "left" :
				  	pt->align & SNOWMIX_ALIGN_CENTER ? "center" : "right",
					pt->align & SNOWMIX_ALIGN_TOP ? "top" :
				  	pt->align & SNOWMIX_ALIGN_MIDDLE ? "middle" : "bottom",
					pt->anchor_x, pt->anchor_y,
					pt->scale_x, pt->scale_y, pt->rotate,
					pt->red, pt->green, pt->blue, pt->alpha,
					pt->clip_left, pt->clip_right, pt->clip_top, pt->clip_bottom,
					pt->clip_abs_x, pt->clip_abs_y, pt->clip_abs_width, pt->clip_abs_height,
					pt->text_grow, pt->text_grow_delta);

				// text move - place_id coor_move scale_move rotate_move alpha_move clip_move
				else if (type == 3) sprintf(pstr, (tcllist ?
					  "{%u {%d %d %u %u} {%.9lf %.9lf %u %u} {%.9lf %u} {%.9lf %u} "
					    "{%.9lf %.9lf %.9lf %.9lf %u %u %u %u}} " :
					  "text move %u %d,%d,%u,%u %.9lf,%.9lf,%u,%u %.9lf,%u %.9lf,%u "
					    "%.9lf,%.9lf,%.9lf,%.9lf,%u,%u,%u,%u\n"),
					from,
					pt->delta_x, pt->delta_y, pt->delta_x_steps, pt->delta_y_steps,
					pt->delta_scale_x, pt->delta_scale_y,
					pt->delta_scale_x_steps, pt->delta_scale_y_steps,
					pt->delta_rotation, pt->delta_rotation_steps,
					pt->delta_alpha, pt->delta_alpha_steps,
					pt->delta_clip_left, pt->delta_clip_right,
					pt->delta_clip_top, pt->delta_clip_bottom,
					pt->delta_clip_left_steps, pt->delta_clip_right_steps,
					pt->delta_clip_top_steps, pt->delta_clip_bottom_steps );

				// text backgr - place_id bg_enabled pad round rbga bg_grow bg_move
				else if (type == 4 && pt->background) {
					sprintf(pstr, (tcllist ?
					  	"{%u %u {%d %d %d %d} {%u %u %u %u} {%.5lf %.5lf %.5lf "
						   "%.5lf} %d {%.5lf %.5lf %.5lf %.5lf %u %u %u %u}} " :
					  	"text backgr %u %u %d,%d,%d,%d %u,%u,%u,%u %.5lf,%.5lf,"
						  "%.5lf,%.5lf %d %.5lf,%.5lf,%.5lf,%.5lf,%u,%u,%u,%u\n"),
						from, pt->background ? 1 : 0,
						pt->pad_left, pt->pad_right, pt->pad_top, pt->pad_bottom,
						pt->round_left_top, pt->round_right_top,
						pt->round_left_bottom, pt->round_right_bottom,
						pt->red_bg, pt->green_bg, pt->blue_bg, pt->alpha_bg,
						pt->text_grow_bg_fixed ? 1 : 0,
						pt->delta_clip_bg_left, pt->delta_clip_bg_right,
						pt->delta_clip_bg_top, pt->delta_clip_bg_bottom,
						pt->delta_clip_bg_left_steps, pt->delta_clip_bg_right_steps,
						pt->delta_clip_bg_top_steps, pt->delta_clip_bg_bottom_steps);
				}
				else if (type == 5 && (pt->linpat_h || pt->linpat_v)) {
					if (tcllist) {
						sprintf(pstr, "{%u ", from);
						while (*pstr) pstr++;
					}
					pLinpat = m_text_places[from]->linpat_h;
					while (pLinpat) {
						if (tcllist) sprintf(pstr, "{h %.5lf %5.5lf %.5lf %5.5lf %.5lf} ",
								pLinpat->fraction, pLinpat->red, pLinpat->green,
								pLinpat->blue, pLinpat->alpha);
						else sprintf(pstr, "text linpat h,%.5lf,%5.5lf,%.5lf,%5.5lf,%.5lf\n",
								pLinpat->fraction, pLinpat->red, pLinpat->green,
								pLinpat->blue, pLinpat->alpha);
						while (*pstr) pstr++;
						pLinpat = pLinpat->next;
					}
					pLinpat = m_text_places[from]->linpat_v;
					while (pLinpat) {
						if (tcllist) sprintf(pstr, "{v %.5lf %5.5lf %.5lf %5.5lf %.5lf} ",
							pLinpat->fraction, pLinpat->red, pLinpat->green,
							pLinpat->blue, pLinpat->alpha);
						else sprintf(pstr, "text linpat h,%.5lf,%5.5lf,%.5lf,%5.5lf,%.5lf\n",
							pLinpat->fraction, pLinpat->red, pLinpat->green,
							pLinpat->blue, pLinpat->alpha);
						while (*pstr) pstr++;
						pLinpat = pLinpat->next;
					}
				}
			}
			while (*pstr) pstr++;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	if (linpatstr) free(linpatstr);
	return retstr;

malloc_failed:
	fprintf(stderr, "CTextItems::Query failed to allocate memory\n");
	return NULL;
}


// text string [<id no> [<string>]]  // empty string deletes entry
int CTextItems::set_text_string (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		int max_id = MaxStrings();
		for (int i=0; i < max_id; i++) {
			char* str = GetString(i);
			if (str) pController->controller_write_msg (ctr,
				"MSG: text string %d <%s>\n", i, str);
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}
	// First we find the id no
	u_int32_t id = 0;
	int x = sscanf(ci, "%u", &id);
	if (x != 1) return 1;
	while (isdigit(*ci)) ci++;
	while (isspace(*ci)) ci++;

	const char* str = ci;
	//while (isdigit(*str)) str++;
	//while (isspace(*str)) str++;

	// If there is no more, we delete the string
	if (!(*str)) {
		x = AddString(id, NULL);	// Deleting
		if (!x && pController->m_verbose)
			pController->controller_write_msg (ctr,
				"MSG: text string deleted id %d\n", id);
		return x;

	}
	int len = 0;
	const char* p = str;
	while (*str) {
		if (*str == '\\' && (*(str+1) == 'n' || *(str+1) == 't')) str++;
		len++;
		str++;
	}
	str = p;
	// Then we allocate space for it
	char* newstr = (char*) calloc(1,len+1);
	if (!newstr) return 1; // We failed to allocate memory
	p = newstr;

	// Now we parse/copy the string
	while (*str) {
		if (*str == '\\' && (*(str+1) == 'n' || *(str+1) == 't' ||
			*(str+1) == '\\' )) {
			str++;
			if (*str == 'n') *newstr++ = '\n';
			else if (*str == 't') *newstr++ = '\t';
			else *newstr++ = '\\';
			str++;
		} else *newstr++ = *str++;
	}
	*newstr = '\0';
	newstr = (char*) p;	// It's okay to cast const char* to char* here
	trim_string(newstr);	// remove trailing spaces
	x = AddString(id, newstr);
	if (!x && pController->m_verbose)
		pController->controller_write_msg (ctr,
			"MSG: text string added id %d <%s>\n", id, newstr);
	return x;
}

// text string concat <id no> <id no> [<id no> ...]
int CTextItems::set_text_string_concat (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	const char* str = ci;

	// First we find the id no
	u_int32_t id = 0;
	u_int32_t id2 = 0;
	u_int32_t len = 1;
	int32_t n;

	// get the destination text string id
	while (isspace(*str)) str++;
	if (sscanf(str, "%u", &id) != 1) return -1;
	while (isdigit(*str)) str++; while (isspace(*str)) str++;

	// Save the address of the source id list for second iteration
	const char* src_list = str;

	// Walk through the source id list and find the length of the strings
	while (sscanf(str, "%u", &id2) == 1) {
		// skipping the read id and trailing spaces
		while (isdigit(*str)) str++; while (isspace(*str)) str++;
		// Get the source string and add its length to len
		char* src_str = GetString(id2);
		// Check if the string exists
		if (!src_str) return -1;
		len += strlen(src_str);
	}
	// check for trailing garbage in source list
	if (*str) return -1;

	// Get a string large enough to hold all the source strings
	char* new_str = (char*) calloc(len,1);
	if (!new_str) return 1;

	str = src_list;
	// Walk through the source id list and concatenate the sour strings
	while (sscanf(str, "%u", &id2) == 1) {
		// skipping the read id and trailing spaces
		while (isdigit(*str)) str++; while (isspace(*str)) str++;
		// Get the source string and concatenate it to the new_str
		char* src_str = GetString(id2);
		if (!src_str) {
			free(new_str);
			return -1;
		}
		strcat(new_str, src_str);
	}
	AddString(id, NULL);				// First we delete the old
	if ((n = AddString(id, new_str)))		// Then we add the new
		free(new_str);				// and free it if we fail.
	
	if (!n && pController && pController->m_verbose)
		pController->controller_write_msg (ctr,
			"MSG: text string concat %d %s\n", id, src_list);
	return n;	// No potential memory leak. Will be freed upon deletion.
}

// text font available
int CTextItems::list_text_fonts_available (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	int i, n_families;
	PangoFontFamily** families = NULL;
	PangoFontMap* fontmap = NULL;
	const char* family_name = NULL;
	if (!(*ci)) {
		if (!pController) return 1;
		if (!(fontmap = pango_cairo_font_map_get_default())) return 1;
		pango_font_map_list_families (fontmap, & families, & n_families);
		pController->controller_write_msg (ctr, "MSG: Found %d font families available\n",
			n_families);
		for (i = 0; i < n_families; i++) {
			PangoFontFamily* family = families[i];
			family_name = pango_font_family_get_name (family);
			pController->controller_write_msg (ctr, "MSG: Font family %3d : %s\n",
				i, family_name);
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		g_free (families);
		return 0;
	}
	return -1;
}

// text font [<id no> [<string>]]  // empty string deletes entry
int CTextItems::set_text_font (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		int max_id = MaxFonts();
		for (int i=0; i < max_id; i++) {
			char* str = GetFont(i);
			if (str) pController->controller_write_msg (ctr,
				"MSG: text font %d <%s>\n", i, str);
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	// Get the id
	u_int32_t id, n;
	if (sscanf(ci, "%u", &id) != 1) return -1;
	while (isdigit(*ci)) ci++;
	while (isspace(*ci)) ci++;

	// If there are no more, we should delete the font if existing
	if (!(*ci)) {
		n = AddFont(id, NULL);
		if (!n && pController->m_verbose)
			pController->controller_write_msg (ctr,
				"MSG: text font deleted id %d\n", id);

		return n;
	}

	// There are more to read and we need copy
	char* str = strdup(ci);
	if (!str) return 1;
	trim_string(str);
	n = AddFont(id, str);
	if (!n && pController->m_verbose)
		pController->controller_write_msg (ctr,
			"MSG: text font added id %d <%s>\n", id, str);
	return n;
}

// text place backgr [<id no> [ <l_pad> <r_pad> <t_pad> <b_pad> [<red> <green> <blue> <alpha>]]]
int CTextItems::set_text_place_background (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place backgr"
					" %2d pad-lrtb %5d,%-5d %5d,%-5d "
					"alpha %.4lf rgb(%.3lf,%.3lf,%.3lf)%s%s\n", id,
					pTextItem->pad_left, pTextItem->pad_right,
					pTextItem->pad_top, pTextItem->pad_bottom,
					pTextItem->alpha_bg,
					pTextItem->red_bg, pTextItem->green_bg,
					pTextItem->blue_bg,
					pTextItem->linpat_h ? " h" : "",
					pTextItem->linpat_v ? " v" : "");
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}
	u_int32_t place_id;
	int32_t pl, pr, pt, pb;
	double red, green, blue, alpha;
	int n = sscanf(ci, "%u %d %d %d %d %lf %lf %lf %lf",
		&place_id, &pl, &pr, &pt, &pb, &red, &green, &blue, &alpha);
	if (n == 9)
		return AddBackgroundParameters(place_id, pl, pr, pt, pb, red, green, blue, alpha);
	else if (n == 5)
		return AddBackgroundParameters(place_id, pl, pr, pt, pb);
	else return 1;		// wrong parameter
}

// text place repeat move <id no> <dx> <dy> <end x> <end y>
int CTextItems::set_text_place_repeat_move (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				if (pTextItem->repeat_delta_x || pTextItem->repeat_delta_y) {
					pController->controller_write_msg (ctr, "MSG: text place move %2d "
						"from %5d,%-5d to %5d,%-5d delta %2d,%-2d\n", id,
						pTextItem->x, pTextItem->y,
						pTextItem->repeat_move_to_x, pTextItem->repeat_move_to_y,
						pTextItem->repeat_delta_x, pTextItem->repeat_delta_y);
				}
			}
		}
		return 0;
	}
	u_int32_t place_id;
	int32_t dx, dy, ex, ey;
	double dr = 0.0;
	int n = sscanf(ci, "%u %d %d %d %d %lf", &place_id, &dx, &dy, &ex, &ey, &dr);
	if (n == 5 || n == 6) return AddRepeatMove(place_id, dx, dy, ex, ey, dr);
	else if (n == 1) {
		return AddRepeatMove(place_id);
	} else return 1;		// wrong parameter
}

// text place backgr linpat <place_id> [<fraction> <red> <green> <blue> <alpha>]"
int CTextItems::set_text_place_backgr_linpat (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				if (pTextItem->linpat_h || pTextItem->linpat_v) {
					linpat_t* pLinpat = pTextItem->linpat_h;
					while (pLinpat) {
						pController->controller_write_msg (ctr, "MSG: text place linpat %2d "
							"%.2lf %.2lf %.2lf %.2lf %.2lf h\n",
							id, pLinpat->fraction, pLinpat->red, pLinpat->green,
							pLinpat->blue, pLinpat->alpha);
						pLinpat = pLinpat->next;
					}
					pLinpat = pTextItem->linpat_v;
					while (pLinpat) {
						pController->controller_write_msg (ctr, "MSG: text place linpat %2d "
							"%.2lf %.2lf %.2lf %.2lf %.2lf v\n",
							id, pLinpat->fraction, pLinpat->red, pLinpat->green,
							pLinpat->blue, pLinpat->alpha);
						pLinpat = pLinpat->next;
					}
				}
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	double fraction, red, green, blue, alpha;
	char orient = 'h';
	int n = sscanf(ci, "%u %lf %lf %lf %lf %lf %c", &place_id, &fraction,
		&red, &green, &blue, &alpha, &orient);
	if (n == 6 || n == 7) return SetTextLinpat(place_id, fraction, red, green, blue, alpha, orient);
	else if (n == 1) return DeleteTextLinpat(place_id);
	else return 1;		// parameter error
}


// text place align [<id no> (left | center | right) (top | middle | bottom)]
int CTextItems::set_text_place_align (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
		//		if (pController->m_verbose)
					pController->controller_write_msg (ctr, "MSG: text place "
						"align %2d %-6s %-6s at %5d,%-5d %4dx%-4d\n", id,
						pTextItem->align & SNOWMIX_ALIGN_LEFT ? "left" :
					  	(pTextItem->align & SNOWMIX_ALIGN_CENTER ? "center" :
							"right"),
						pTextItem->align & SNOWMIX_ALIGN_TOP ? "top" :
					  		(pTextItem->align & SNOWMIX_ALIGN_MIDDLE ?
								"middle" : "bottom"),
						pTextItem->repeat_move_x, pTextItem->repeat_move_y,
						pTextItem->w, pTextItem->h);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}
	char* str = (char*)ci;
//	while (*str == ' ' || *str == '\t') str++;
	u_int32_t place_id;
	int n = sscanf(str, "%u", &place_id);
	if (n != 1) return -1;
	while (isdigit(*str)) str++;
	while (isspace(*str)) str++;
	while (*str) {
		int align = 0;
		double rotate = 0.0;

		while (isspace(*str)) str++;
		if (sscanf(str,"%lf", &rotate) == 1) {
	 		if (SetTextRotation(place_id, rotate)) return -1;
			if (*str == '-') str++;
			while (isdigit(*str)) str++;
			if (*str == '.') str++;
			while (isdigit(*str)) str++;
			while (isspace(*str)) str++;
			continue;
		}
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
//fprintf(stderr, "ALIGN <%s> <%s>\n", ci,str);
			return 1;		// parameter error
		}
	 	int n = SetTextAlign(place_id, align);
		if (n) return -1;
		while (isspace(*str)) str++;
	}
	return 0;
}

// text place move scale [<id no> [<delta_scale_x> <delta_scale_y> <steps_x> <steps_y>]]
int CTextItems::set_text_place_move_scale (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place move "
					"scale %2d %.3lf %.3lf %u %u\n", id,
					pTextItem->delta_scale_x, pTextItem->delta_scale_y,
					pTextItem->delta_scale_x_steps,
					pTextItem->delta_scale_y_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, dxs, dys;
	double delta_scale_x, delta_scale_y;
	int n = sscanf(ci, "%u %lf %lf %u %u", &place_id, &delta_scale_x,
		&delta_scale_y, &dxs, &dys);
	if (n == 5) return SetTextMoveScale(place_id, delta_scale_x, delta_scale_y, dxs, dys);
	else return 1;		// parameter error
}

// text place move coor [<id no> [<delta_x> <delta_y> <steps_x> <steps_y>]]
int CTextItems::set_text_place_move_coor (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place move coor"
					" %2d %d %d %u %u\n", id,
					pTextItem->delta_x, pTextItem->delta_y,
					pTextItem->delta_x_steps, pTextItem->delta_y_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, dxs, dys;
	int32_t dx, dy;
	int n = sscanf(ci, "%u %d %d %u %u", &place_id, &dx, &dy, &dxs, &dys);
	if (n == 5) return SetTextMoveCoor(place_id, dx, dy, dxs, dys);
	else return 1;		// parameter error
}

// text place backgr move alpha [<id no> <delta alpha> <alpha steps> ] 
int CTextItems::set_text_place_backgr_move_alpha (struct controller_type *ctr,
	CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place backgr "
					"move alpha %2d %.6lf %u\n", id,
					pTextItem->delta_alpha_bg, 
					pTextItem->delta_alpha_bg_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, delta_alpha_bg_steps;
	double delta_alpha_bg;
	int n = sscanf(ci, "%u %lf %u", &place_id, &delta_alpha_bg, &delta_alpha_bg_steps);
	if (n == 3) return SetTextBackgroundMoveAlpha(place_id, delta_alpha_bg, delta_alpha_bg_steps);
	else return 1;		// parameter error
}

// text place move clip [<id no> <delta left> <delta right> <delta top> <delta bottom>
//                <left steps> <right steps> <top steps> <bottom steps>]
int CTextItems::set_text_place_move_clip (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pT = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pT = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place move "
					"clip %2d %.3lf %.3lf %.3lf %.3lf %u %u %u %u\n", id,
					pT->delta_clip_left, pT->delta_clip_right,
					pT->delta_clip_top, pT->delta_clip_bottom,
					pT->delta_clip_left_steps, pT->delta_clip_right_steps,
					pT->delta_clip_top_steps, pT->delta_clip_bottom_steps
					);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, step_left, step_right, step_top, step_bottom;
	double delta_clip_left, delta_clip_right, delta_clip_top, delta_clip_bottom;
	int n = sscanf(ci, "%u %lf %lf %lf %lf %u %u %u %u",
		&place_id, &delta_clip_left, &delta_clip_right, &delta_clip_top, &delta_clip_bottom,
		&step_left, &step_right, &step_top, &step_bottom);
	if (n == 9) return SetTextMoveClip(place_id, delta_clip_left, delta_clip_right, delta_clip_top,
		delta_clip_bottom, step_left, step_right, step_top, step_bottom);
	else return 1;		// parameter error
}

// text place backgr move clip [<id no> <delta left> <delta right> <delta top> <delta bottom>
//                <left steps> <right steps> <top steps> <bottom steps>]
int CTextItems::set_text_place_backgr_move_clip (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pT = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pT = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place move "
					"clip %2d %.3lf %.3lf %.3lf %.3lf %u %u %u %u\n", id,
					pT->delta_clip_bg_left, pT->delta_clip_bg_right,
					pT->delta_clip_bg_top, pT->delta_clip_bg_bottom,
					pT->delta_clip_bg_left_steps, pT->delta_clip_bg_right_steps,
					pT->delta_clip_bg_top_steps, pT->delta_clip_bg_bottom_steps
					);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, step_left, step_right, step_top, step_bottom;
	double delta_clip_bg_left, delta_clip_bg_right, delta_clip_bg_top, delta_clip_bg_bottom;
	int n = sscanf(ci, "%u %lf %lf %lf %lf %u %u %u %u", &place_id,
		&delta_clip_bg_left, &delta_clip_bg_right, &delta_clip_bg_top, &delta_clip_bg_bottom,
		&step_left, &step_right, &step_top, &step_bottom);
	if (n == 9) return SetTextBackgroundMoveClip(place_id, delta_clip_bg_left, delta_clip_bg_right,
		delta_clip_bg_top, delta_clip_bg_bottom, step_left, step_right, step_top, step_bottom);
	else return 1;		// parameter error
}

// text place move alpha [<place id> <delta_alpha> <step_alpha>]
int CTextItems::set_text_place_move_alpha (struct controller_type *ctr,
	CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place move "
					"alpha %2d %.6lf %u\n", id,
					pTextItem->delta_alpha, 
					pTextItem->delta_alpha_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, delta_alpha_steps;
	double delta_alpha;
	int n = sscanf(ci, "%u %lf %u", &place_id, &delta_alpha, &delta_alpha_steps);
	if (n == 3) return SetTextMoveAlpha(place_id, delta_alpha, delta_alpha_steps);
	else return 1;		// parameter error
}

// text place move rotation [<place id> <delta_rotation> <step_rotation>]
int CTextItems::set_text_place_move_rotation (struct controller_type *ctr,
	CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place move "
					"rotation %2d %.6lf %u\n", id,
					pTextItem->delta_rotation, 
					pTextItem->delta_rotation_steps);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, delta_rotation_steps;
	double delta_rotation;
	int n = sscanf(ci, "%u %lf %u", &place_id, &delta_rotation, &delta_rotation_steps);
	if (n == 3) return SetTextMoveRotation(place_id, delta_rotation, delta_rotation_steps);
	else return 1;		// parameter error
}

// text place coor [<id no> <x> <y>]
int CTextItems::set_text_place_coor (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place coor %2d "
					"%5d %-5d wxh %4ux%-3u\n",
					id, pTextItem->x, pTextItem->y,
					pTextItem->w, pTextItem->h);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	int32_t x, y;
	char anchor[3];
	anchor[0] = '\0';
	int n = sscanf(ci, "%u %d %d %2["ANCHOR_TAGS"]", &place_id, &x, &y, &anchor[0]);
	if (n == 3 || n == 4) return SetTextCoor(place_id, x, y, anchor);
	else return 1;		// parameter error
}

// text place offset [<place id <offset_x> <offset_y>]
int CTextItems::set_text_place_offset (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		if (!pController) return 1;
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr,
					"MSG: text place %2d offset %d,%d\n",
					id, pTextItem->offset_x, pTextItem->offset_y);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	int32_t offset_x, offset_y;
	if (sscanf(ci, "%u %d %d", &place_id, &offset_x, &offset_y) != 3) return -1;
	int n = SetTextOffset(place_id, offset_x, offset_y);
	if (m_verbose && !n && pController)
		pController->controller_write_msg (ctr,
			"text place %u offset %d,%d\n", place_id, offset_x, offset_y);
	return n;
}

// text place rgb [<id no> <red> <green> <blue>]
int CTextItems::set_text_place_rgb (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place rgb "
					"%2d %.3lf %.3lf %.3lf\n",
					id, pTextItem->red, pTextItem->green, pTextItem->blue);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	double red, green, blue;
	int n = sscanf(ci, "%u %lf %lf %lf", &place_id, &red, &green, &blue);
	if (n == 4) return SetTextColor(place_id, red, green, blue);
	else return 1;		// parameter error
}

// text place clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
int CTextItems::set_text_place_clip (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place "
					"clip %2d %.3lf %.3lf %.3lf %.3lf\n", id,
					pTextItem->clip_left, pTextItem->clip_right,
					pTextItem->clip_top, pTextItem->clip_bottom);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	double clip_left, clip_right, clip_top, clip_bottom;
	int n = sscanf(ci, "%u %lf %lf %lf %lf", &place_id, &clip_left,
		&clip_right, &clip_top, &clip_bottom);
	if (n == 5) return SetTextClip(place_id, clip_left, clip_right, clip_top, clip_bottom);
	else return 1;		// parameter error
}

// text place clipabs [<place id> <clip x> <clip y> <clip width> <clip height>]
int CTextItems::set_text_place_clipabs (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place "
					"clipabs %2d %d %d %d %d\n", id,
					pTextItem->clip_abs_x, pTextItem->clip_abs_y,
					pTextItem->clip_abs_width, pTextItem->clip_abs_height);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	int x, y, width, height;
	int n = sscanf(ci, "%u %d %d %d %d", &place_id, &x, &y, &width, &height);
	if (n == 5) return SetTextClipAbs(place_id, x, y, width, height);
	else return 1;		// parameter error
}

// text place backgr round [<place id> <left top> <right top> <left bottom> <right bottom>]
int CTextItems::set_text_place_backgr_round (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place backgr "
					"round %2d %u %u %u %u\n", id,
					pTextItem->round_left_top, pTextItem->round_right_top,
					pTextItem->round_left_bottom, pTextItem->round_right_bottom);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id, left_top, right_top, left_bottom, right_bottom;
	int n = sscanf(ci, "%u %u %u %u %u", &place_id, &left_top, &right_top, &left_bottom, &right_bottom);
	if (n == 5) return SetTextBackgroundRound(place_id, left_top, right_top, left_bottom, right_bottom);
	else return 1;		// parameter error
}

// text place backgr alpha [<place id> <alpha>]
int CTextItems::set_text_place_backgr_alpha (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place backgr "
					"alpha %2d %.4lf\n", id,
					pTextItem->alpha_bg);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	double alpha_bg;
	int n = sscanf(ci, "%u %lf", &place_id, &alpha_bg);
	if (n == 2) return SetTextBackgroundAlpha(place_id, alpha_bg);
	else return 1;		// parameter error
}

// text place backgr clip [<place id> <clip left> <clip right> <clip top> <clip bottom>]
int CTextItems::set_text_place_backgr_clip (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place backgr "
					"clip %2d %.3lf %.3lf %.3lf %.3lf\n", id,
					pTextItem->clip_bg_left, pTextItem->clip_bg_right,
					pTextItem->clip_bg_top, pTextItem->clip_bg_bottom);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	double clip_left, clip_right, clip_top, clip_bottom;
	int n = sscanf(ci, "%u %lf %lf %lf %lf", &place_id, &clip_left,
		&clip_right, &clip_top, &clip_bottom);
	if (n == 5) return SetTextBackgroundClip(place_id, clip_left, clip_right, clip_top, clip_bottom);
	else return 1;		// parameter error
}

// text place backgr rgb [<place id> <red> <green> <blue> [ <alpha> ]
int CTextItems::set_text_place_backgr_rgb (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place backgr "
					"rgb %2d %.4lf %.4lf %.4lf %.4lf\n", id,
					pTextItem->red_bg, pTextItem->green_bg,
					pTextItem->blue_bg, pTextItem->alpha_bg);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	double red_bg, green_bg, blue_bg, alpha_bg;
	int n = sscanf(ci, "%u %lf %lf %lf %lf", &place_id, &red_bg,
		&green_bg, &blue_bg, &alpha_bg);
	if (n > 3) {
		if (SetTextBackgroundRGB(place_id, red_bg, green_bg, blue_bg)) return -1;
		if ( n > 4) return SetTextBackgroundAlpha(place_id, alpha_bg);
		return 0;
	}
	else return 1;		// parameter error
}

// text place alpha [<id no> <alpha>]
int CTextItems::set_text_place_alpha (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place alpha "
					"%2d %.4lf \n", id, pTextItem->alpha);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	} 
	u_int32_t place_id;
	double alpha;
	int n = sscanf(ci, "%u %lf", &place_id, &alpha);
	if (n == 2) return SetTextAlpha(place_id, alpha);
	else return 1;		// parameter error
}

// text place anchor [<id no> (n | s | e | w | c | ne | nw | se | sw)]
int CTextItems::set_text_place_anchor (struct controller_type *ctr, CController* pController,
	const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		if (!pController) return 1;
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place %2d anchor "
					"%d,%d\n", id, pTextItem->anchor_x, pTextItem->anchor_y);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}
	u_int32_t place_id;
	char anchor[3];
	anchor[0] = '\0';
	if (sscanf(ci, "%u %2["ANCHOR_TAGS"]", &place_id, &anchor[0]) != 2) return -1;
	int n = SetTextAnchor(place_id, anchor);
	if (m_verbose && !n && pController)
		pController->controller_write_msg (ctr,
			"text place %u anchor %s\n", place_id, anchor);
	return n;
}

// text place rotation [<id no> <rotation>]
int CTextItems::set_text_place_rotation (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place "
					"rotation %2d %.4lf \n", id, pTextItem->rotate);
			}
		}
		return 0;
	} 
	u_int32_t place_id;
	double rotation;
	int n = sscanf(ci, "%u %lf", &place_id, &rotation);
	if (n == 2) return SetTextRotation(place_id, rotation);
	else return 1;		// parameter error
}

// text place string [<id no> <string id>]
int CTextItems::set_text_place_string (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place "
					"string %2d %.u \n", id, pTextItem->text_id);
			}
		}
		return 0;
	} 
	u_int32_t place_id, text_id;
	int n = sscanf(ci, "%u %u", &place_id, &text_id);
	if (n == 2) return SetTextString(place_id, text_id);
	else return 1;		// parameter error
}

// text place font [<id no> <font id>]
int CTextItems::set_text_place_font (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place "
					"font %2d %.u \n", id, pTextItem->font_id);
			}
		}
		return 0;
	} 
	u_int32_t place_id, font_id;
	int n = sscanf(ci, "%u %u", &place_id, &font_id);
	if (n == 2) return SetFontID(place_id, font_id);
	else return 1;		// parameter error
}

// text place scale [<id no> <scale_x> <scale_y>]
int CTextItems::set_text_place_scale (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id < MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: text place "
					"scale %2d %.4lf %.4lf wxh (%4ux%-3u)\n", id,
					pTextItem->scale_x, pTextItem->scale_y,
					pTextItem->w, pTextItem->h);
			}
		}
		return 0;
	} 
	u_int32_t place_id;
	double scale_x, scale_y;
	int n = sscanf(ci, "%u %lf %lf", &place_id, &scale_x, &scale_y);
	if (n == 3) return SetTextScale(place_id, scale_x, scale_y);
	else return 1;		// parameter error
}

// text place grow <place id> <grow> [<grow delta> [variable]]
int CTextItems::set_text_place_grow (struct controller_type *ctr, CController* pController, const char* ci)
{
	u_int32_t place_id, grow_delta = 0;
	int32_t grow;
	bool fixed = true;
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	int n = sscanf(ci, "%u %d %u", &place_id, &grow, &grow_delta);
	if (n != 2 && n != 3) return 1;
	while (isdigit(*ci)) ci++;	// place_id 
	while (isspace(*ci)) ci++;
	while (isdigit(*ci)) ci++;	// grow
	while (isspace(*ci)) ci++;
	if (n == 3) {
		while (isdigit(*ci)) ci++;	// grow_delta
		while (isspace(*ci)) ci++;
		if (!strncasecmp(ci, "var", 3)) fixed = false;
		else if (*ci) return -1;
	}
	return SetTextGrow(place_id, grow, grow_delta, fixed);
}

// text place [<id no> | <text_id> <font_id> <x> <y> [<red> <green> <blue>]]]
int CTextItems::set_text_place (struct controller_type *ctr, CController* pController, const char* ci)
{
	if (!pController) return -1;
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		text_item_t* pTextItem = NULL;
		for (unsigned int id = 0; id< MaxPlaces(); id++) {
			if ((pTextItem = GetTextPlace(id))) {
				pController->controller_write_msg (ctr, "MSG: Text place id %2d text id %2d "
					"font %2d at %4d,%-4d rgb=(%.3lf,%.3lf,%.3lf) "
					"wxh (%ux%u) anchor (%d,%d)\n",
					id, pTextItem->text_id, pTextItem->font_id,
					pTextItem->x, pTextItem->y, pTextItem->red,
					pTextItem->green, pTextItem->blue,
					pTextItem->w, pTextItem->h,
					pTextItem->anchor_x, pTextItem->anchor_y
					);
			}
		}
		pController->controller_write_msg (ctr, "MSG:\n");
		return 0;
	}

	u_int32_t place_id;
	u_int32_t text_id;
	u_int32_t font_id;
	u_int32_t x;
	u_int32_t y;
	double red = 0.0;
	double green = 0.0;
	double blue = 0.0;
	double alpha = 1.0;
	char anchor[3];
	anchor[0] = '\0';
	int n = sscanf(ci, "%u %u %u %u %u %lf %lf %lf %lf %2["ANCHOR_TAGS"]",
		&place_id, &text_id, &font_id, &x, &y, &red, &green,
		&blue, &alpha, &anchor[0]);

	// Check for the text place delete command
	if (n == 1) {
		n = RemovePlacedText(place_id);
		if (!n && pController->m_verbose)
			pController->controller_write_msg (ctr,
			"MSG: text place id %u removed.\n", place_id);
		return n;

	// Check for the text place command using default color
	} else {
		if (n != 5 && n != 8 && n != 9 && n != 10) return 1;

		return PlaceText(place_id, text_id, font_id, x, y, red, green, blue,
			alpha, anchor);
	}
}

int CTextItems::AddString(u_int32_t id, char* str) {
	if (id >= m_max_strings || !m_text_strings) return -1;
	if (m_text_strings[id]) {
		free(m_text_strings[id]);
		if (str) m_string_count--;
	} else {
		if (str) m_string_count++;
	}
	m_text_strings[id] = str;
	if (m_text_strings_multibyte) m_text_strings_multibyte[id] = FindUTF8Multibyte(str) ? true : false;
//fprintf(stderr, "Text has multibyte %s\n", m_text_strings_multibyte[id] ? "yes" : "no");
	if (m_text_strings_seqno) m_text_strings_seqno[id]++;
	return 0;
}

u_int16_t CTextItems::GetPlacedTextsStringSeqNo(u_int32_t place_id) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return 0;
	u_int32_t id = m_text_places[place_id]->text_id; 
	if (id >= m_max_strings || !m_text_strings_seqno) return 0;
	return m_text_strings_seqno[id];
}

u_int16_t CTextItems::GetPlacedTextsFontSeqNo(u_int32_t place_id) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return 0;
	u_int32_t id = m_text_places[place_id]->font_id; 
	if (id >= m_max_fonts || !m_fonts_seqno) return 0;
	return m_fonts_seqno[id];
}

PangoFontDescription* CTextItems::GetPangoFontDesc(u_int32_t id) {
	if (id >= m_max_fonts || !m_pDesc) return NULL;
	return m_pDesc[id];
}

int CTextItems::AddFont(u_int32_t id, char* str) {
	if (id >= m_max_fonts || !m_fonts) return -1;
	if (m_fonts[id]) {
		free(m_fonts[id]);
		m_fonts[id] = NULL;
		m_font_count--;
//fprintf(stderr, "Deleting font %u\n", id);
	}
	if (m_pDesc && m_pDesc[id]) {
//fprintf(stderr, "Deleting Pango font Desc %u\n", id);
		pango_font_description_free(m_pDesc[id]);
		m_pDesc[id] = NULL;
	}
	if (!str) return 0;

    if (m_pDesc) {
        m_pDesc[id] = pango_font_description_from_string(str);
        if (!m_pDesc[id]) return -1;
    }
//fprintf(stderr, "Setting font %u to %s\n", id, str);
	m_fonts[id] = str;		// Str was copied before calling this function
	if (m_fonts_seqno) m_fonts_seqno[id]++;
	m_font_count++;
	return 0;
}

bool CTextItems::StringHasMultibyte(u_int32_t id) {
	if (m_text_strings_multibyte && id < m_max_strings) return m_text_strings_multibyte[id];
	else return false;
}

char* CTextItems::GetString(u_int32_t id) {
	if (m_text_strings && id < m_max_strings) return m_text_strings[id];
	else return NULL;
}

char* CTextItems::GetFont(u_int32_t id) {
	if (m_fonts && id < m_max_fonts) return m_fonts[id];
	else return NULL;
}

text_item_t* CTextItems::GetTextPlace(u_int32_t id) {
	if (m_text_places && id < m_max_places) return m_text_places[id];
	else return NULL;
}

int CTextItems::SetTextGrow(u_int32_t place_id, int grow, u_int32_t grow_delta, bool fixed) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	m_text_places[place_id]->text_grow = grow;
	m_text_places[place_id]->text_grow_delta = grow_delta;
	m_text_places[place_id]->text_grow_bg_fixed = fixed;
	return 0;
}

int CTextItems::SetTextBackgroundMoveAlpha(u_int32_t place_id, double delta_alpha_bg,
	u_int32_t delta_alpha_bg_steps) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	text_item_t* pTextItem = m_text_places[place_id];
	pTextItem->delta_alpha_bg = delta_alpha_bg;
	pTextItem->delta_alpha_bg_steps = delta_alpha_bg_steps;
	if (delta_alpha_bg_steps) pTextItem->update = true;
	return 0;
}

int CTextItems::SetTextMoveAlpha(u_int32_t place_id, double delta_alpha,
	u_int32_t delta_alpha_steps) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	text_item_t* pTextItem = m_text_places[place_id];
	pTextItem->delta_alpha = delta_alpha;
	pTextItem->delta_alpha_steps = delta_alpha_steps;
	if (delta_alpha_steps) pTextItem->update = true;
	return 0;
}

int CTextItems::SetTextMoveRotation(u_int32_t place_id, double delta_rotation,
	u_int32_t delta_rotation_steps) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	text_item_t* pTextItem = m_text_places[place_id];
	pTextItem->delta_rotation = delta_rotation;
	pTextItem->delta_rotation_steps = delta_rotation_steps;
	if (delta_rotation_steps) pTextItem->update = true;
	return 0;
}

int CTextItems::SetTextMoveScale(u_int32_t place_id, double delta_scale_x,
	double delta_scale_y, u_int32_t delta_scale_x_steps, u_int32_t delta_scale_y_steps) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	text_item_t* pTextItem = m_text_places[place_id];
	pTextItem->delta_scale_x = delta_scale_x;
	pTextItem->delta_scale_y = delta_scale_y;
	pTextItem->delta_scale_x_steps = delta_scale_x_steps;
	pTextItem->delta_scale_y_steps = delta_scale_y_steps;
	if (delta_scale_y_steps || delta_scale_x_steps) pTextItem->update = true;
	return 0;
}

int CTextItems::SetTextMoveCoor(u_int32_t place_id, int32_t delta_x, int32_t delta_y,
	u_int32_t delta_x_steps, u_int32_t delta_y_steps) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	text_item_t* pTextItem = m_text_places[place_id];
	pTextItem->delta_x = delta_x;
	pTextItem->delta_y = delta_y;
	pTextItem->delta_x_steps = delta_x_steps;
	pTextItem->delta_y_steps = delta_y_steps;
	if (delta_y_steps || delta_x_steps) pTextItem->update = true;
	return 0;
}

int CTextItems::SetTextCoor(u_int32_t place_id, int32_t x, int32_t y, const char* anchor) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	if (anchor && *anchor) SetTextAnchor(place_id, anchor);
	text_item_t* pTextItem = m_text_places[place_id];
	pTextItem->x = x;
	pTextItem->y = y;

	// We reset repeat move parameters
	AddRepeatMove(place_id);
	pTextItem->repeat_move_x = pTextItem->repeat_move_to_x = x;
	pTextItem->repeat_move_y = pTextItem->repeat_move_to_x = y;
	return 0;
}

int CTextItems::SetTextOffset(u_int32_t place_id, int32_t offset_x, int32_t offset_y) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	text_item_t* pTextItem = m_text_places[place_id];
	pTextItem->offset_x = offset_x;
	pTextItem->offset_y = offset_y;
	return 0;
}

int CTextItems::SetTextAlign(u_int32_t place_id, int align) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	if (align & (SNOWMIX_ALIGN_LEFT | SNOWMIX_ALIGN_CENTER | SNOWMIX_ALIGN_RIGHT)) {
		m_text_places[place_id]->align = (m_text_places[place_id]->align &
			(~(SNOWMIX_ALIGN_LEFT | SNOWMIX_ALIGN_CENTER | SNOWMIX_ALIGN_RIGHT))) | align;
	} else if  (align & (SNOWMIX_ALIGN_TOP | SNOWMIX_ALIGN_MIDDLE | SNOWMIX_ALIGN_BOTTOM)) {
		m_text_places[place_id]->align = (m_text_places[place_id]->align &
			(~(SNOWMIX_ALIGN_TOP  | SNOWMIX_ALIGN_MIDDLE | SNOWMIX_ALIGN_BOTTOM))) | align;
	}
	return 0;
}

int CTextItems::DeleteTextLinpat(u_int32_t place_id)
{
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id] ||
		(!m_text_places[place_id]->linpat_h && m_text_places[place_id]->linpat_v)) return -1;
	linpat_t* p = m_text_places[place_id]->linpat_h;
	while (p) {
		linpat_t* p2 = p->next;
		free(p);
		p = p2;
	}
	m_text_places[place_id]->linpat_h = NULL;
	p = m_text_places[place_id]->linpat_v;
	while (p) {
		linpat_t* p2 = p->next;
		free(p);
		p = p2;
	}
	m_text_places[place_id]->linpat_v = NULL;
	return 0;
}

int CTextItems::SetTextLinpat(u_int32_t place_id, double fraction, double red,
	double green, double blue, double alpha, char orient)
{
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id] || (orient != 'h' && orient != 'v')) return -1;
	linpat_t* p = (linpat_t*) calloc(sizeof(linpat_t),1);
	if (!p) return 1;

	p->fraction = fraction;
	p->red = red;
	p->green = green;
	p->blue = blue;
	p->alpha = alpha;
	if (orient == 'h') {
		p->next = m_text_places[place_id]->linpat_h;
		m_text_places[place_id]->linpat_h = p;
	} else {
		p->next = m_text_places[place_id]->linpat_v;
		m_text_places[place_id]->linpat_v = p;
	}
	return 0;
}

int CTextItems::SetTextColor(u_int32_t place_id, double red, double green, double blue) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	m_text_places[place_id]->red = red;
	m_text_places[place_id]->green = green;
	m_text_places[place_id]->blue  = blue;
	return 0;
}

int CTextItems::SetTextAlpha(u_int32_t place_id, double alpha) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	if (alpha < 0) alpha = 0.0;
	else if (alpha > 1.0) alpha = 1.0;
	m_text_places[place_id]->alpha = alpha;
	return 0;
}

int CTextItems::SetTextRotation(u_int32_t place_id, double rotation) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	m_text_places[place_id]->rotate = rotation;
	return 0;
}

int CTextItems::SetTextString(u_int32_t place_id, u_int32_t text_id) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id] ||
		!m_text_strings || text_id >= m_max_strings || !m_text_strings[text_id])
		return -1;
	m_text_places[place_id]->text_id = text_id;
	m_text_places[place_id]->text_id_seqno = 0;
	return 0;
}

int CTextItems::SetFontID(u_int32_t place_id, u_int32_t font_id) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id] ||
		!m_fonts || font_id >= m_max_fonts || !m_fonts[font_id])
		return -1;
	m_text_places[place_id]->font_id = font_id;
	m_text_places[place_id]->font_id_seqno = 0;
	return 0;
}


int CTextItems::SetTextScale(u_int32_t place_id, double scale_x, double scale_y) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	m_text_places[place_id]->scale_x = scale_x;
	m_text_places[place_id]->scale_y = scale_y;
	return 0;
}

int CTextItems::SetTextClip(u_int32_t place_id, double clip_left, double clip_right,
	double clip_top, double clip_bottom)
{
	if (place_id >= m_max_places || !m_text_places[place_id]) return -1;
	if (clip_left < 0.0) clip_left = 0.0;
	else if (clip_left > 1.0) clip_left = 1.0;
	if (clip_right < 0.0) clip_right = 0.0;
	else if (clip_right > 1.0) clip_right = 1.0;
	if (clip_top < 0.0) clip_top = 0.0;
	else if (clip_top > 1.0) clip_top = 1.0;
	if (clip_bottom < 0.0) clip_bottom = 0.0;
	else if (clip_bottom > 1.0) clip_bottom = 1.0;
	m_text_places[place_id]->clip_left = clip_left;
	m_text_places[place_id]->clip_right = clip_right;
	m_text_places[place_id]->clip_top = clip_top;
	m_text_places[place_id]->clip_bottom = clip_bottom;
	return 0;
}

int CTextItems::SetTextClipAbs(u_int32_t place_id, int32_t x, int32_t y,
	int32_t width, int32_t height)
{
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	m_text_places[place_id]->clip_abs_x = x;
	m_text_places[place_id]->clip_abs_y = y;
	m_text_places[place_id]->clip_abs_width = width;
	m_text_places[place_id]->clip_abs_height = height;
	return 0;
}

int CTextItems::SetTextMoveClip(u_int32_t place_id, double delta_clip_left,
	double delta_clip_right, double delta_clip_top, double delta_clip_bottom,
	u_int32_t step_left, u_int32_t step_right, u_int32_t step_top, u_int32_t step_bottom) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	if (delta_clip_left < -1.0) delta_clip_left = -1.0;
	else if (delta_clip_left > 1.0) delta_clip_left = 1.0;
	if (delta_clip_right < -1.0) delta_clip_right = -1.0;
	else if (delta_clip_right > 1.0) delta_clip_right = 1.0;
	if (delta_clip_top < -1.0) delta_clip_top = -1.0;
	else if (delta_clip_top > 1.0) delta_clip_top = 1.0;
	if (delta_clip_bottom < -1.0) delta_clip_bottom = -1.0;
	else if (delta_clip_bottom > 1.0) delta_clip_bottom = 1.0;

	m_text_places[place_id]->delta_clip_left = delta_clip_left;
	m_text_places[place_id]->delta_clip_right = delta_clip_right;
	m_text_places[place_id]->delta_clip_top = delta_clip_top;
	m_text_places[place_id]->delta_clip_bottom = delta_clip_bottom;
	m_text_places[place_id]->delta_clip_left_steps = step_left;
	m_text_places[place_id]->delta_clip_right_steps = step_right;
	m_text_places[place_id]->delta_clip_top_steps = step_top;
	m_text_places[place_id]->delta_clip_bottom_steps = step_bottom;
	if (step_left || step_right || step_top || step_bottom) m_text_places[place_id]->update = true;
	return 0;
}

int CTextItems::SetTextBackgroundMoveClip(u_int32_t place_id, double delta_clip_bg_left,
	double delta_clip_bg_right, double delta_clip_bg_top, double delta_clip_bg_bottom,
	u_int32_t step_left, u_int32_t step_right, u_int32_t step_top, u_int32_t step_bottom) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	if (delta_clip_bg_left < -1.0) delta_clip_bg_left = -1.0;
	else if (delta_clip_bg_left > 1.0) delta_clip_bg_left = 1.0;
	if (delta_clip_bg_right < -1.0) delta_clip_bg_right = -1.0;
	else if (delta_clip_bg_right > 1.0) delta_clip_bg_right = 1.0;
	if (delta_clip_bg_top < -1.0) delta_clip_bg_top = -1.0;
	else if (delta_clip_bg_top > 1.0) delta_clip_bg_top = 1.0;
	if (delta_clip_bg_bottom < -1.0) delta_clip_bg_bottom = -1.0;
	else if (delta_clip_bg_bottom > 1.0) delta_clip_bg_bottom = 1.0;

	m_text_places[place_id]->delta_clip_bg_left = delta_clip_bg_left;
	m_text_places[place_id]->delta_clip_bg_right = delta_clip_bg_right;
	m_text_places[place_id]->delta_clip_bg_top = delta_clip_bg_top;
	m_text_places[place_id]->delta_clip_bg_bottom = delta_clip_bg_bottom;
	m_text_places[place_id]->delta_clip_bg_left_steps = step_left;
	m_text_places[place_id]->delta_clip_bg_right_steps = step_right;
	m_text_places[place_id]->delta_clip_bg_top_steps = step_top;
	m_text_places[place_id]->delta_clip_bg_bottom_steps = step_bottom;
	if (step_left || step_right || step_top || step_bottom) m_text_places[place_id]->update = true;
	return 0;
}

int CTextItems::SetTextBackgroundRound(u_int32_t place_id, u_int32_t left_top, u_int32_t right_top, u_int32_t left_bottom, u_int32_t right_bottom) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	m_text_places[place_id]->round_left_top = left_top;
	m_text_places[place_id]->round_right_top = right_top;
	m_text_places[place_id]->round_left_bottom = left_bottom;
	m_text_places[place_id]->round_right_bottom = right_bottom;
	return 0;
}

int CTextItems::SetTextBackgroundAlpha(u_int32_t place_id, double alpha_bg) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	if (alpha_bg < 0.0) alpha_bg = 0.0;
	else if (alpha_bg > 1.0) alpha_bg = 1.0;
	m_text_places[place_id]->alpha_bg = alpha_bg;
	return 0;
}

int CTextItems::SetTextBackgroundClip(u_int32_t place_id, double clip_bg_left, double clip_bg_right,
	double clip_bg_top, double clip_bg_bottom) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	if (clip_bg_left < 0.0) clip_bg_left = 0.0;
	else if (clip_bg_left > 1.0) clip_bg_left = 1.0;
	if (clip_bg_right < 0.0) clip_bg_right = 0.0;
	else if (clip_bg_right > 1.0) clip_bg_right = 1.0;
	if (clip_bg_top < 0.0) clip_bg_top = 0.0;
	else if (clip_bg_top > 1.0) clip_bg_top = 1.0;
	if (clip_bg_bottom < 0.0) clip_bg_bottom = 0.0;
	else if (clip_bg_bottom > 1.0) clip_bg_bottom = 1.0;
	m_text_places[place_id]->clip_bg_left = clip_bg_left;
	m_text_places[place_id]->clip_bg_right = clip_bg_right;
	m_text_places[place_id]->clip_bg_top = clip_bg_top;
	m_text_places[place_id]->clip_bg_bottom = clip_bg_bottom;
	return 0;
}

int CTextItems::SetTextBackgroundRGB(u_int32_t place_id, double red_bg, double green_bg,
	double blue_bg) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	if (red_bg < 0.0) red_bg = 0.0;
	else if (red_bg > 1.0) red_bg = 1.0;
	if (green_bg < 0.0) green_bg = 0.0;
	else if (green_bg > 1.0) green_bg = 1.0;
	if (blue_bg < 0.0) blue_bg = 0.0;
	else if (blue_bg > 1.0) blue_bg = 1.0;
	m_text_places[place_id]->red_bg = red_bg;
	m_text_places[place_id]->green_bg = green_bg;
	m_text_places[place_id]->blue_bg = blue_bg;
	return 0;
}


int CTextItems::RemovePlacedText(u_int32_t place_id) {
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	linpat_t* p = m_text_places[place_id]->linpat_h;
	while (p) {
		linpat_t* tmp = p->next;
		free(p);
		p = tmp;
	}
	p = m_text_places[place_id]->linpat_v;
	while (p) {
		linpat_t* tmp = p->next;
		free(p);
		p = tmp;
	}
	free(m_text_places[place_id]);
	m_text_places[place_id] = NULL;
	m_placed_count--;
	return 0;
}

int CTextItems::SetTextAnchor(u_int32_t place_id, const char* anchor)
{
	//if (!anchor || place_id >= m_max_places || !m_text_places[place_id]) return;
	if (place_id >= m_max_places || !m_text_places[place_id] || !m_pVideoMixer) return -1;
	//text_item_t* pTextItem = m_text_places[place_id];
	return SetAnchor(&(m_text_places[place_id]->anchor_x),
                &(m_text_places[place_id]->anchor_y),
                anchor, m_pVideoMixer->GetSystemWidth(), m_pVideoMixer->GetSystemHeight());
/*
	if (strcasecmp("ne", anchor) == 0) {
		pTextItem->anchor_x = m_width;
		pTextItem->anchor_y = 0;
	} else if (strcasecmp("se", anchor) == 0) {
		pTextItem->anchor_x = m_width;
		pTextItem->anchor_y = m_height;
	} else if (strcasecmp("sw", anchor) == 0) {
		pTextItem->anchor_x = 0;
		pTextItem->anchor_y = m_height;
	} else if (strcasecmp("n", anchor) == 0) {
		pTextItem->anchor_x = m_width>>1;
		pTextItem->anchor_y = 0;
	} else if (strcasecmp("w", anchor) == 0) {
		pTextItem->anchor_x = m_width;
		pTextItem->anchor_y = m_height>>1;
	} else if (strcasecmp("s", anchor) == 0) {
		pTextItem->anchor_x = m_width>>1;
		pTextItem->anchor_y = m_height;
	} else if (strcasecmp("e", anchor) == 0) {
		pTextItem->anchor_x = m_width;
		pTextItem->anchor_y = m_height>>1;
	} else {
		pTextItem->anchor_x = 0;
		pTextItem->anchor_y = 0;
	}
*/
}

int CTextItems::DeleteTextRoller(u_int32_t id)
{
	if (!m_text_places || id >= m_max_places || !m_text_places[id] ||
		!m_text_places[id]->pRollerText) return -1;
	text_roller_item_t* p = m_text_places[id]->pRollerText->unplaced;
	while (p) {
		text_roller_item_t* next = p->next;
		free(p);
		p = next;
	}
	p = m_text_places[id]->pRollerText->placed;
	while (p) {
		text_roller_item_t* next = p->next;
		free(p);
		p = next;
	}
	free(m_text_places[id]->pRollerText);
	m_text_places[id]->pRollerText = NULL;
	return 0;
}

int CTextItems::AddTextRoller(u_int32_t id, u_int32_t width, u_int32_t height, int32_t delta_x, int32_t delta_y)
{
	if (!m_text_places || id >= m_max_places || !m_text_places[id]) return -1;
	if (m_text_places[id]->pRollerText) DeleteTextRoller(id);
	m_text_places[id]->pRollerText = (text_roller_t*) calloc(sizeof(text_roller_t),1);
	return (m_text_places[id]->pRollerText ? 0 : 1); // No memory leak. Freed by deconstructor
}

int CTextItems::PlaceText(u_int32_t place_id, u_int32_t text_id, u_int32_t font_id,
	u_int32_t x, u_int32_t y, double red, double green, double blue, double alpha,
	const char* anchor)
{
	text_item_t* pTextItem = NULL;
	if (place_id >= m_max_places || !m_text_places || !m_text_strings ||
		text_id >= m_max_strings || !m_text_strings[text_id] || !m_fonts ||
		font_id > m_max_fonts || !m_fonts[font_id]) return -1;
	if ( m_text_places[place_id]) pTextItem = m_text_places[place_id];
	else {
		if (!(pTextItem = (text_item_t*)calloc(1,sizeof(text_item_t)))) return 1;
		pTextItem->offset_x		= 0;
		pTextItem->offset_y		= 0;
		pTextItem->w	 		= 0;
		pTextItem->h	 		= 0;
		pTextItem->align		= SNOWMIX_ALIGN_TOP | SNOWMIX_ALIGN_LEFT;
		pTextItem->pRollerText		= NULL;
		pTextItem->text_grow_delta	= 0;
		pTextItem->text_grow		= -1;
		pTextItem->text_grow_bg_fixed	= true;
		pTextItem->delta_x		= 0;
		pTextItem->delta_x_steps	= 0;
		pTextItem->delta_y		= 0;
		pTextItem->delta_y_steps	= 0;
		pTextItem->delta_scale_x	= 0.0;
		pTextItem->delta_scale_x_steps	= 0;
		pTextItem->delta_scale_y	= 0.0;
		pTextItem->delta_scale_y_steps	= 0;
		pTextItem->delta_rotation	= 0.0;
		pTextItem->delta_rotation_steps	= 0;
		pTextItem->delta_alpha		= 0.0;
		pTextItem->delta_alpha_steps	= 0;
		pTextItem->delta_alpha_bg	= 0.0;
		pTextItem->delta_alpha_bg_steps	= 0;
		pTextItem->delta_clip_left	= 0.0;
		pTextItem->delta_clip_left_steps= 0;
		pTextItem->delta_clip_right	= 0.0;
		pTextItem->delta_clip_right_steps= 0;
		pTextItem->delta_clip_top	= 0.0;
		pTextItem->delta_clip_top_steps	= 0;
		pTextItem->delta_clip_bottom	= 0.0;
		pTextItem->delta_clip_bottom_steps= 0;
		pTextItem->delta_clip_bg_left	= 0.0;
		pTextItem->delta_clip_bg_left_steps = 0;
		pTextItem->delta_clip_bg_right	= 0.0;
		pTextItem->delta_clip_bg_right_steps = 0;
		pTextItem->delta_clip_bg_top 	= 0.0;
		pTextItem->delta_clip_bg_top_steps = 0;
		pTextItem->delta_clip_bg_bottom	= 0.0;
		pTextItem->delta_clip_bg_bottom_steps = 0;
		pTextItem->clip_abs_x		= 0;
		pTextItem->clip_abs_y		= 0;
		pTextItem->clip_abs_width	= 0;
		pTextItem->clip_abs_height	= 0;
		pTextItem->anchor_x		= 0;
		pTextItem->anchor_y		= 0;
		pTextItem->scale_x		= 1.0;
		pTextItem->scale_y		= 1.0;
		pTextItem->update		= false;

		// Clip parameters
		pTextItem->clip_left		= 0.0;
		pTextItem->clip_right		= 1.0;
		pTextItem->clip_top		= 0.0;
		pTextItem->clip_bottom		= 1.0;
		pTextItem->clip_bg_left		= 0.0;
		pTextItem->clip_bg_right	= 1.0;
		pTextItem->clip_bg_top		= 0.0;
		pTextItem->clip_bg_bottom	= 1.0;

		// Background parameters
		pTextItem->background		= false;
		pTextItem->pad_left		= 0;
		pTextItem->pad_right		= 0;
		pTextItem->pad_top		= 0;
		pTextItem->pad_bottom		= 0;
		pTextItem->round_left_top	= 0;
		pTextItem->round_right_top	= 0;
		pTextItem->round_left_bottom	= 0;
		pTextItem->round_right_bottom	= 0;
		pTextItem->red_bg		= 0.0;
		pTextItem->green_bg		= 0.0;
		pTextItem->blue_bg		= 0.0;
		pTextItem->alpha_bg		= 0.0;
		pTextItem->linpat_h		= NULL;
		pTextItem->linpat_v		= NULL;

		m_placed_count++;
	}
	if (pTextItem) {
		m_text_places[place_id] = pTextItem;

		// Base parameters
		pTextItem->id	 = place_id;	// FIXME. Needs restructuring to use this
		pTextItem->text_id = text_id;
		pTextItem->font_id = font_id;
		pTextItem->x	 = x;
		pTextItem->y	 = y;
		pTextItem->red	 = red;
		pTextItem->green = green;
		pTextItem->blue  = blue;
		pTextItem->alpha = alpha;
		//pTextItem->blink = false;
		//pTextItem->display = true;
		// default anchor nw
		if (anchor && *anchor) SetTextAnchor(place_id, anchor);

	
		AddRepeatMove(place_id);
		// repeat move parameters
		pTextItem->repeat_move_x = pTextItem->repeat_move_to_x = x;
		pTextItem->repeat_move_y = pTextItem->repeat_move_to_x = y;
		//pTextItem->repeat_delta_rotation = 0.0;
		//pTextItem->repeat_delta_alpha = 0.0;

	} else return 1;
	return 0;
}

int CTextItems::AddBackgroundParameters(u_int32_t place_id, 
	int32_t pl, int32_t pr, int32_t pt, int32_t pb)
{
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	text_item_t* p = GetTextPlace(place_id);
	if (!p) return -1;
	p->background = true;
	p->pad_left = pl;
	p->pad_right = pr;
	p->pad_top = pt;
	p->pad_bottom = pb;
	return 0;
}
int CTextItems::AddBackgroundParameters(u_int32_t place_id, 
	int32_t pl, int32_t pr, int32_t pt, int32_t pb,
	double red, double green, double blue, double alpha)
{
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	text_item_t* p = GetTextPlace(place_id);
	if (!p) return -1;
	p->background = true;
	p->pad_left = pl;
	p->pad_right = pr;
	p->pad_top = pt;
	p->pad_bottom = pb;
	p->red_bg = red;
	p->green_bg = green;
	p->blue_bg = blue;
	p->alpha_bg = alpha;
	p->update = true;
	return 0;
}

int CTextItems::AddRepeatMove(u_int32_t place_id, int32_t dx, int32_t dy, int32_t ex, int32_t ey, double dr, double da)
{
	if (place_id >= m_max_places || !m_text_places || !m_text_places[place_id]) return -1;
	text_item_t* p = GetTextPlace(place_id);
	if (!p) return -1;
	p->repeat_move_x = p->x;
	p->repeat_move_y = p->y;
	p->repeat_move_to_x = ex;
	p->repeat_move_to_y = ey;
	p->repeat_delta_x = dx;
	p->repeat_delta_y = dy;
	p->repeat_delta_rotation = dr;
	p->repeat_delta_alpha = da;
	p->update = true;
	return 0;
}

void CTextItems::UpdateMoveAll()
{
	if (!m_text_places) return;
	for (u_int32_t id=0 ; id < m_max_places ; id++)
		if (m_text_places[id] && m_text_places[id]->update) UpdateMove(id);
}

void CTextItems::UpdateMove(u_int32_t place_id)
{
	text_item_t* p = GetTextPlace(place_id);
	if (!p || !p->update) return;
	bool changed = false;

	// check for 'text move coor'
	if (p->delta_x_steps > 0) {
		p->delta_x_steps--;
		p->x += p->delta_x;
		p->repeat_move_x += p->delta_x;
		p->repeat_move_to_x += p->delta_x;
		changed = true;
	}
	if (p->delta_y_steps > 0) {
		p->delta_y_steps--;
		p->y += p->delta_y;
		p->repeat_move_y += p->delta_y;
		p->repeat_move_to_y += p->delta_y;
		changed = true;
	}

	// check for 'text move scale'
	if (p->delta_scale_x_steps > 0) {
		p->delta_scale_x_steps--;
		p->scale_x += p->delta_scale_x;
		if (p->scale_x > 10000.0) p->scale_x = 10000.0;
		else if (p->scale_x < 0.0) p->scale_x = 0.0;
		changed = true;
		//p->text_id_seqno = 0;	// trigger new WxH calculation
	}
	// check for 'text move scale'
	if (p->delta_scale_y_steps > 0) {
		p->delta_scale_y_steps--;
		p->scale_y += p->delta_scale_y;
		if (p->scale_y > 10000.0) p->scale_y = 10000.0;
		else if (p->scale_y < 0.0) p->scale_y = 0.0;
		changed = true;
		//p->text_id_seqno = 0;	// trigger new WxH calculation
	}

	// check for 'text move alpha'
	if (p->delta_alpha_steps > 0) {
		p->delta_alpha_steps--;
		p->alpha += p->delta_alpha;
		if (p->alpha > 1.0) p->alpha = 1.0;
		else if (p->alpha < 0.0) p->alpha = 0.0;
		changed = true;
	}

	// check for 'text move rotation'
	if (p->delta_rotation_steps > 0) {
		p->delta_rotation_steps--;
		p->rotate += p->delta_rotation;
		if (p->rotate > 2*M_PI)
			p->rotate -= (2*M_PI);
		else if (p->rotate < 2*M_PI)
			p->rotate += (2*M_PI);
		changed = true;
	}

	// check for 'text backgr move alpha'
	if (p->delta_alpha_bg_steps > 0) {
		p->delta_alpha_bg_steps--;
		p->alpha_bg += p->delta_alpha_bg;
		if (p->alpha_bg > 1.0) p->alpha_bg = 1.0;
		else if (p->alpha_bg < 0.0) p->alpha_bg = 0.0;
		changed = true;
	}

	// check for 'text move clip'
	if (p->delta_clip_left_steps > 0) {
		p->delta_clip_left_steps--;
		p->clip_left += p->delta_clip_left;
		if (p->clip_left > 1.0) p->clip_left = 1.0;
		else if (p->clip_left < 0.0) p->clip_left = 0.0;
		changed = true;
	}
	// check for 'text move clip'
	if (p->delta_clip_right_steps > 0) {
		p->delta_clip_right_steps--;
		p->clip_right += p->delta_clip_right;
		if (p->clip_right > 1.0) p->clip_right = 1.0;
		else if (p->clip_right < 0.0) p->clip_right = 0.0;
		changed = true;
	}
	// check for 'text move clip'
	if (p->delta_clip_top_steps > 0) {
		p->delta_clip_top_steps--;
		p->clip_top += p->delta_clip_top;
		if (p->clip_top > 1.0) p->clip_top = 1.0;
		else if (p->clip_top < 0.0) p->clip_top = 0.0;
		changed = true;
	}
	// check for 'text move clip'
	if (p->delta_clip_bottom_steps > 0) {
		p->delta_clip_bottom_steps--;
		p->clip_bottom += p->delta_clip_bottom;
		if (p->clip_bottom > 1.0) p->clip_bottom = 1.0;
		else if (p->clip_bottom < 0.0) p->clip_bottom = 0.0;
		changed = true;
	}

	// check for 'text backgr move clip'
	if (p->delta_clip_bg_left_steps > 0) {
		p->delta_clip_bg_left_steps--;
		p->clip_bg_left += p->delta_clip_bg_left;
		if (p->clip_bg_left > 1.0) p->clip_bg_left = 1.0;
		else if (p->clip_bg_left < 0.0) p->clip_bg_left = 0.0;
		changed = true;
	}
	// check for 'text backgr move clip'
	if (p->delta_clip_bg_right_steps > 0) {
		p->delta_clip_bg_right_steps--;
		p->clip_bg_right += p->delta_clip_bg_right;
		if (p->clip_bg_right > 1.0) p->clip_bg_right = 1.0;
		else if (p->clip_bg_right < 0.0) p->clip_bg_right = 0.0;
		changed = true;
	}
	// check for 'text backgr move clip'
	if (p->delta_clip_bg_top_steps > 0) {
		p->delta_clip_bg_top_steps--;
		p->clip_bg_top += p->delta_clip_bg_top;
		if (p->clip_bg_top > 1.0) p->clip_bg_top = 1.0;
		else if (p->clip_bg_top < 0.0) p->clip_bg_top = 0.0;
		changed = true;
	}
	// check for 'text backgr move clip'
	if (p->delta_clip_bg_bottom_steps > 0) {
		p->delta_clip_bg_bottom_steps--;
		p->clip_bg_bottom += p->delta_clip_bg_bottom;
		if (p->clip_bg_bottom > 1.0) p->clip_bg_bottom = 1.0;
		else if (p->clip_bg_bottom < 0.0) p->clip_bg_bottom = 0.0;
		changed = true;
	}

	if (p->repeat_delta_x) {
		if (p->repeat_move_to_x < p->x && p->repeat_delta_x < 0) {
			if (p->repeat_move_x + p->repeat_delta_x < p->repeat_move_to_x)
				p->repeat_move_x = p->x;
			else p->repeat_move_x += p->repeat_delta_x;
		} else if (p->repeat_move_to_x > p->x && p->repeat_delta_x > 0) {
			if (p->repeat_move_x + p->repeat_delta_x > p->repeat_move_to_x)
				p->repeat_move_x = p->x;
			else p->repeat_move_x += p->repeat_delta_x;
		}
		changed = true;
	}
	// Now check if text item is for moving vertically 
	if (p->repeat_delta_y) {
		if (p->repeat_move_to_y < p->y && p->repeat_delta_y < 0) {
			if (p->repeat_move_y + p->repeat_delta_y < p->repeat_move_to_y)
				p->repeat_move_y = p->y;
			else p->repeat_move_y += p->repeat_delta_y;
		} else if (p->repeat_move_to_y > p->y && p->repeat_delta_y > 0) {
			if (p->repeat_move_y + p->repeat_delta_y > p->repeat_move_to_y)
				p->repeat_move_y = p->y;
			else p->repeat_move_y += p->repeat_delta_y;
		}
		changed = true;
	}
	if (p->repeat_delta_rotation != 0.0) {
		p->rotate += p->repeat_delta_rotation;
		if (p->rotate > 2*M_PI)
			p->rotate -= (2*M_PI);
		else if (p->rotate < 2*M_PI)
			p->rotate += (2*M_PI);
		changed = true;
	}
	if (p->repeat_delta_alpha != 0.0) {
		p->alpha += p->repeat_delta_alpha;
		if (p->alpha > 1.0) p->alpha = 1.0;
		else if (p->alpha < 0.0) p->alpha = 0.0;
		changed = true;
	}

	// If data was not changed, we can set the update flag to false
	// This way we stop all these updates for this placed text until
	// the move/update/change parameter sets it again
	p->update = changed;
}
