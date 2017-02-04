/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_TEXT_H__
#define __VIDEO_TEXT_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
#include <pango/pangocairo.h>
#ifdef WIN32
#include "windows_util.h"
#else
#include <sys/time.h>
#endif

#define	MAX_STRINGS	64
#define	MAX_FONTS	64
#define	MAX_TEXT_PLACES	64

#define	TEXT_ALIGN_LEFT   1
#define	TEXT_ALIGN_CENTER 2
#define	TEXT_ALIGN_RIGHT  4
#define	TEXT_ALIGN_TOP    8
#define	TEXT_ALIGN_MIDDLE 16
#define	TEXT_ALIGN_BOTTOM 32

class CVideoMixer;

struct linpat_t {
	double			fraction;
	double			red;
	double			green;
	double			blue;
	double			alpha;
	struct linpat_t*	next;
};

struct text_roller_item_t {
	int			type;
	u_int32_t		id;
	int32_t			pad_x;
	int32_t			pad_y;
	double			scale_x;
	double			scale_y;
	struct text_roller_item_t*	next;
};

struct text_roller_t {
	u_int32_t		width;
	u_int32_t		height;
	int32_t			delta_x;
	int32_t			delta_y;
	text_roller_item_t*	unplaced;
	text_roller_item_t*	placed;
	
};

struct text_item_t {

	// Base parameters
	u_int32_t		id;			// text place id
	u_int32_t		text_id;		// place texts string id
	u_int32_t		font_id;		// place texts font id
	int32_t			x;
	int32_t			y;
	int32_t			offset_x;
	int32_t			offset_y;

	int32_t			w;			// Placed text's width.
	int32_t			h;			// Placed text's height.
	u_int16_t		text_id_seqno;		// Seq no of string used for last placed text
	u_int16_t		font_id_seqno;		// Seq no of font used for last placed text

							// If scale changes, set text_id_seqno = 0 to trig update
							// of size WxH
	int32_t			align;
	int32_t			anchor_x;
	int32_t			anchor_y;
	double			scale_x;
	double			scale_y;
	double			rotate;
	//bool			display;
	//bool			blink;
	double			red;
	double			green;
	double			blue;
	double			alpha;
	text_roller_t*		pRollerText;
	bool			update;

	// Clip parameters
	double			clip_left;
	double			clip_right;
	double			clip_top;
	double			clip_bottom;
	double			clip_bg_left;
	double			clip_bg_right;
	double			clip_bg_top;
	double			clip_bg_bottom;

	// Clip abs
	int32_t			clip_abs_x;
	int32_t			clip_abs_y;
	int32_t			clip_abs_width;
	int32_t			clip_abs_height;


	// To show a grow number of chars in string
	int32_t			text_grow;
	u_int32_t		text_grow_delta;
	bool			text_grow_bg_fixed;

	// move parameters
	int32_t			delta_x;
	u_int32_t		delta_x_steps;
	int32_t			delta_y;
	u_int32_t		delta_y_steps;
	double			delta_scale_x;
	u_int32_t		delta_scale_x_steps;
	double			delta_scale_y;
	u_int32_t		delta_scale_y_steps;
	double			delta_rotation;
	u_int32_t		delta_rotation_steps;
	double			delta_alpha;
	u_int32_t		delta_alpha_steps;
	double			delta_alpha_bg;
	u_int32_t		delta_alpha_bg_steps;

	double			delta_clip_left;
	u_int32_t		delta_clip_left_steps;
	double			delta_clip_right;
	u_int32_t		delta_clip_right_steps;
	double			delta_clip_top;
	u_int32_t		delta_clip_top_steps;
	double			delta_clip_bottom;
	u_int32_t		delta_clip_bottom_steps;
	double			delta_clip_bg_left;
	u_int32_t		delta_clip_bg_left_steps;
	double			delta_clip_bg_right;
	u_int32_t		delta_clip_bg_right_steps;
	double			delta_clip_bg_top;
	u_int32_t		delta_clip_bg_top_steps;
	double			delta_clip_bg_bottom;
	u_int32_t		delta_clip_bg_bottom_steps;

	// repeat move parameters
	int32_t			repeat_move_x;
	int32_t			repeat_move_y;
	int32_t			repeat_delta_x;
	int32_t			repeat_delta_y;
	int32_t			repeat_move_to_x;
	int32_t			repeat_move_to_y;	
	double			repeat_delta_rotation;
	double			repeat_delta_alpha;

	// background parameters
	bool			background;
	int32_t			pad_left;
	int32_t			pad_right;
	int32_t			pad_top;
	int32_t			pad_bottom;
	u_int32_t		round_left_top;
	u_int32_t		round_right_top;
	u_int32_t		round_left_bottom;
	u_int32_t		round_right_bottom;
	double			red_bg;
	double			green_bg;
	double			blue_bg;
	double			alpha_bg;
	struct linpat_t*	linpat_v;
	struct linpat_t*	linpat_h;

	//struct text_item_t* next;
};

#include "controller.h"

class CTextItems {
  public:
	CTextItems(CVideoMixer* pVideoMixer, u_int32_t max_strings,
		u_int32_t max_fonts, u_int32_t max_places);
	~CTextItems();
	int		ParseCommand(CController* pController, struct controller_type* ctr,
				const char* str);
	bool		StringHasMultibyte(u_int32_t id);
	char*		GetString(u_int32_t id);
	char*		GetFont(u_int32_t id);
	text_item_t*	GetTextPlace(u_int32_t id);

	void		UpdateMove(u_int32_t id);
	void		UpdateMoveAll();

	char*		Query(const char* query, bool tcllist = true);


	u_int32_t	MaxStrings(){ return m_max_strings; }
	u_int32_t	MaxFonts(){ return m_max_strings; }
	u_int32_t	MaxPlaces(){ return m_max_places; }
	u_int16_t	GetPlacedTextsFontSeqNo(u_int32_t id);
	u_int16_t	GetPlacedTextsStringSeqNo(u_int32_t id);
	PangoFontDescription* GetPangoFontDesc(u_int32_t id);


  private:
	int		AddBackgroundParameters(u_int32_t place_id, int32_t pl, int32_t pr,
				int32_t pt, int32_t pb);
	int		AddBackgroundParameters(u_int32_t place_id, int32_t pl, int32_t pr,
				int32_t pt, int32_t pb, double red, double green, double blue,
				double alpha);
	int		AddFont(u_int32_t id, char* str);
	int		AddRepeatMove(u_int32_t place_id, int32_t dx = 0, int32_t dy = 0,
				int32_t ex = 0, int32_t ey = 0, double dr = 0.0, double da = 0.0);
	int		AddString(u_int32_t id, char* str);
	int		PlaceText(u_int32_t place_id, u_int32_t text_id, u_int32_t font_id,
				u_int32_t x, u_int32_t y,
				double red, double green, double blue,
				double alpha = 0.0, const char* anchor = "nw");
	int		RemovePlacedText(u_int32_t place_id);
	int		SetTextAnchor(u_int32_t place_id, const char* anchor);
	int		SetTextBackgroundMoveAlpha(u_int32_t place_id, double delta_alpha,
				u_int32_t delta_alpha_steps);
	int		SetTextMoveAlpha(u_int32_t place_id, double delta_alpha,
				u_int32_t delta_alpha_steps);
	int		SetTextMoveCoor(u_int32_t place_id, int32_t delta_x, int32_t delta_y,
				u_int32_t delta_x_steps, u_int32_t delta_y_steps);
	int		SetTextMoveRotation(u_int32_t place_id, double delta_rotation,
				u_int32_t delta_rotation_steps);
	int		SetTextMoveScale(u_int32_t place_id, double delta_scale_x,
				double delta_scale_y, u_int32_t delta_scale_x_steps,
				u_int32_t delta_scale_y_steps);
	int		SetTextMoveClip(u_int32_t place_id,
				double delta_clip_x1, double delta_clip_y1,
				double delta_clip_x2, double delta_clip_y2,
				u_int32_t delta_clip_x1_steps, u_int32_t delta_clip_y1_steps,
				u_int32_t delta_clip_x2_steps, u_int32_t delta_clip_y2_steps);
	int		SetTextBackgroundMoveClip(u_int32_t place_id,
				double delta_clip_bg_x1, double delta_clip_bg_y1,
				double delta_clip_bg_x2, double delta_clip_bg_y2,
				u_int32_t delta_clip_bg_x1_steps,
				u_int32_t delta_clip_bg_y1_steps,
				u_int32_t delta_clip_bg_x2_steps,
				u_int32_t delta_clip_bg_y2_steps);
	int		SetTextCoor(u_int32_t place_id, int32_t x, int32_t y,
				const char* anchor);
	int		SetTextOffset(u_int32_t place_id, int32_t offset_x, int32_t offset_y);
	int		SetTextAlign(u_int32_t place_id, int align);
	int		SetTextAlpha(u_int32_t place_id, double alpha);
	int		SetTextRotation(u_int32_t place_id, double rotation);
	int		SetTextScale(u_int32_t place_id, double scale_x, double scale_y);
	int		SetTextString(u_int32_t place_id, u_int32_t text_id);
	int		SetFontID(u_int32_t place_id, u_int32_t font_id);
	int		SetTextClip(u_int32_t place_id, double clip_x1, double clip_y1,
				double clip_x2, double clip_y2);
	int		SetTextBackgroundRound(u_int32_t place_id, u_int32_t left_top, u_int32_t right_top,
				u_int32_t left_bottom, u_int32_t right_bottom);
	int		SetTextClipAbs(u_int32_t place_id, int32_t x, int32_t y, int32_t width, int32_t height);
	int		SetTextBackgroundAlpha(u_int32_t place_id, double alpha_bg);
	int		SetTextBackgroundClip(u_int32_t place_id, double clip_x1, double clip_y1,
				double clip_x2, double clip_y2);
	int		SetTextBackgroundRGB(u_int32_t place_id, double red_bg,
				double green_bg, double blue_bg);
	int		SetTextGrow(u_int32_t place_id, int grow, u_int32_t grow_delta, bool fixed);
	int		SetTextColor(u_int32_t place_id, double red, double green,
				double blue);
	int		DeleteTextLinpat(u_int32_t place_id);
	int		SetTextLinpat(u_int32_t place_id, double fraction, double red,
				double green, double blue, double alpha, char orient);

	int		AddTextRoller(u_int32_t id, u_int32_t width, u_int32_t height, int32_t delta_x, int32_t delta_y);
	int		DeleteTextRoller(u_int32_t id);

	int		list_help (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_verbose (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_roller (struct controller_type *ctr,
				CController* pController, const char* ci);
	
	int		set_text_place_font (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_string (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_string (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_string_concat (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_font (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		list_text_fonts_available (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_clip (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_clipabs (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_grow (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_info (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_repeat_move (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_background (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_backgr_alpha (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_backgr_clip (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_backgr_rgb (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_backgr_round (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_backgr_move_alpha (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_move_alpha (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_move_clip (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_backgr_move_clip (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_move_coor (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_move_rotation (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_move_scale (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_coor (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_offset (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_alpha (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_anchor (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_rotation (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_rgb (struct controller_type *ctr, CController* pController,
				const char* ci);
	int		set_text_place_scale (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_backgr_linpat (struct controller_type *ctr,
				CController* pController, const char* ci);
	int		set_text_place_align (struct controller_type *ctr, CController* pController,
				const char* ci);
	int		set_text_place (struct controller_type *ctr, CController* pController,
				const char* ci);

	CVideoMixer*		m_pVideoMixer;
	u_int32_t		m_max_strings;
	u_int32_t		m_max_fonts;
	u_int32_t		m_max_places;
	char**			m_text_strings;
	bool*			m_text_strings_multibyte;
	u_int16_t*		m_text_strings_seqno;
	u_int16_t*		m_fonts_seqno;
	char**			m_fonts;
	PangoFontDescription**	m_pDesc;				// Pango Font Description
	text_item_t**		m_text_places;
	text_item_t*		m_list_text_item_head;
	text_item_t*		m_list_text_item_tail;
	u_int32_t		m_string_count;
	u_int32_t		m_font_count;
	u_int32_t		m_placed_count;
	u_int32_t		m_verbose;
};
#endif	// VIDEO_TEXT
