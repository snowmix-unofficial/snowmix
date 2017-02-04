/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIRTUAL_FEED_H__
#define __VIRTUAL_FEED_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>
#include <cairo/cairo.h>

#include "cairo_graphic.h"
#include "video_feed.h"
#include "video_image.h"

class CCairoGraphic;


/*
// Cairo Matrix
struct transform_matrix_t {

	double	matrix_xx;
	double	matrix_yx;
	double	matrix_xy;
	double	matrix_yy;
	double	matrix_x0;
	double	matrix_y0;
};
*/
enum virtual_feed_enum_t {
	VIRTUAL_FEED_NONE,
	VIRTUAL_FEED_IMAGE,
	VIRTUAL_FEED_FEED
};


struct virtual_feed_t {
	char*			name;
	u_int32_t		id;
	u_int32_t		source_id;
	//feed_type*		pFeed;
	//image_item_t*		pImage;
	virtual_feed_enum_t	source_type;
	u_int32_t		width;
	u_int32_t		height;
	double			scale_x;
	double			scale_y;
	int32_t			x;
	int32_t			y;
	int32_t			offset_x;
	int32_t			offset_y;
	int32_t			anchor_x;
	int32_t			anchor_y;
	u_int32_t		align;
	u_int32_t		mode;
	u_int32_t		clip_x;
	u_int32_t		clip_y;
	u_int32_t		clip_w;
	u_int32_t		clip_h;
	double			rotation;
	double			alpha;
	cairo_filter_t		filter;

	// move parameters
	bool			update;		// true if animation is to be performed
	int32_t			delta_x;
	u_int32_t		delta_x_steps;
	int32_t			delta_y;
	u_int32_t		delta_y_steps;
	int32_t			delta_offset_x;
	u_int32_t		delta_offset_x_steps;
	int32_t			delta_offset_y;
	u_int32_t		delta_offset_y_steps;
	int32_t			delta_clip_x;
	u_int32_t		delta_clip_x_steps;
	int32_t			delta_clip_y;
	u_int32_t		delta_clip_y_steps;
	int32_t			delta_clip_w;
	u_int32_t		delta_clip_w_steps;
	int32_t			delta_clip_h;
	u_int32_t		delta_clip_h_steps;
	double			delta_scale_x;
	u_int32_t		delta_scale_x_steps;
	double			delta_scale_y;
	u_int32_t		delta_scale_y_steps;
	double			delta_rotation;
	u_int32_t		delta_rotation_steps;
	double			delta_alpha;
	u_int32_t		delta_alpha_steps;

	transform_matrix_t*	pMatrix;

};


#define MAX_VIRTUAL_FEEDS	32

class CVirtualFeed {
  public:
	CVirtualFeed(CVideoMixer* pVideoMixer, u_int32_t max_feeds = MAX_VIRTUAL_FEEDS);
	~CVirtualFeed();
	u_int32_t	MaxFeeds() { return m_max_feeds; }

	int		ParseCommand(struct controller_type* ctr, const char* str);
	char*		Query(const char* query, bool tcllist = true);
	int		UpdateMove(u_int32_t id);
	int		OverlayFeed(CCairoGraphic* pCairoGraphic, const char* str);

	virtual_feed_enum_t GetFeedSourceType(u_int32_t id);
	int		GetFeedSourceId(u_int32_t id);
	char*		GetFeedName(u_int32_t id);
	int		GetFeedGeometry(u_int32_t id, u_int32_t* w, u_int32_t* h);
	int		GetFeedCoordinates(u_int32_t id, int32_t* x, int32_t* y);
	int		GetFeedOffset(u_int32_t id, int32_t* offset_x, int32_t* offset_y);
	int		GetFeedClip(u_int32_t id, u_int32_t* clip_x, u_int32_t* clip_y,
				u_int32_t* clip_w, u_int32_t* clip_h);
	int		GetFeedScale(u_int32_t id, double* scale_x, double* scale_y);
	int		GetFeedRotation(u_int32_t id, double* rotation);
	int		GetFeedAlign(u_int32_t id, u_int32_t* align);
	int		GetFeedAlpha(u_int32_t id, double* rotation);
	int		GetFeedFilter(u_int32_t id, cairo_filter_t* filter);
	transform_matrix_t*	GetFeedMatrix(u_int32_t id);

private:
	int		list_help (struct controller_type *ctr, const char* ci);
	int		CreateFeed(u_int32_t id);
	int		DeleteFeed(u_int32_t id);

	int		SetFeedName(u_int32_t id, const char* feed_name);

	int		SetVirtualSource(u_int32_t id, u_int32_t source_id, bool feed);
	//int		SetFeedSource(u_int32_t id, feed_type* pFeed);
	//int		SetFeedSource(u_int32_t id, image_item_t* pImage);

	int		SetFeedGeometry(u_int32_t id, u_int32_t width,
				u_int32_t height);

	int		SetFeedScale(u_int32_t id, double scale_x, double scale_y);
	int		SetFeedScaleDelta(u_int32_t id, double delta_scale_x,
				double delta_scale_y, u_int32_t delta_scale_x_steps,
				u_int32_t delta_scale_y_steps);

	int		SetFeedRotation(u_int32_t id, double rotation);
	int		SetFeedRotationDelta(u_int32_t id, double delta_rotation,
				u_int32_t delta_rotation_steps);

	int		SetFeedAlpha(u_int32_t id, double alpha);
	int		SetFeedAlphaDelta(u_int32_t id, double delta_alpha,
				u_int32_t delta_alpha_steps);

	int		PlaceFeed(u_int32_t id, int32_t* x, int32_t* y,
				u_int32_t* clip_x = NULL, u_int32_t* clip_y = NULL,
				u_int32_t* clip_w = NULL, u_int32_t* clip_h = NULL,
				double* rotation = NULL, double* scale_x = NULL,
				double* scale_y = NULL, double* alpha = NULL);

	int		SetFeedCoordinates(u_int32_t id, int32_t x, int32_t y);
	int		SetFeedOffset(u_int32_t id, int32_t offset_x, int32_t offset_y);
	int		SetFeedCoordinatesDelta(u_int32_t id, int32_t delta_x,
				int32_t delta_y, int32_t steps_x, int32_t steps_y);
	int		SetFeedOffsetDelta(u_int32_t id, int32_t delta_offset_x,
				int32_t delta_offset_y, int32_t steps_offset_x,
				int32_t steps_offset_y );

	int		SetFeedClip(u_int32_t id, u_int32_t clip_x, u_int32_t clip_y,
				u_int32_t clip_w, u_int32_t clip_h);
	int		SetFeedClipDelta(u_int32_t id,
				int32_t delta_clip_x, int32_t delta_clip_y,
				int32_t delta_clip_w, int32_t delta_clip_h,
				u_int32_t delta_clip_x_steps,
				u_int32_t delta_clip_y_steps,
				u_int32_t delta_clip_w_steps,
				u_int32_t delta_clip_h_steps);
	int		SetFeedMatrix(u_int32_t id, double xx, double xy, double yx,
				double yy, double x0, double y0);
	int		SetFeedAlign(u_int32_t id, u_int32_t align);
	int		SetFeedAnchor(u_int32_t id, const char* anchor);

	int		SetFeedFilter(u_int32_t id, cairo_filter_t filter);

	//virtual_feed_t*	FindFeedByNumber(u_int32_t id);
	int		set_virtual_feed_add(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_align(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_alpha(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_anchor(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_coor(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_clip(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_geometry(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_move_clip(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_move_alpha(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_offset(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_move_rotation(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_move_offset(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_move_scale(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_move_coor(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_place_rect(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_scale(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_source(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_matrix(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_filter(struct controller_type* ctr,
				const char* str);
	int		set_virtual_feed_rotation(struct controller_type* ctr,
				const char* str);
	int		set_verbose(struct controller_type* ctr, const char* str);
	int		set_info(struct controller_type* ctr, const char* str);


	u_int32_t		m_max_feeds;
	virtual_feed_t**	m_feeds;
	CVideoMixer*		m_pVideoMixer;
	u_int32_t		m_verbose;
	u_int32_t		m_feed_count;
};
	
#endif	// VIRTUAL_FEED_H
