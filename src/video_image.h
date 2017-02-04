/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_IMAGE_H__
#define __VIDEO_IMAGE_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>
#ifdef HAVE_PNG16
#include <libpng16/png.h>
#else
#ifdef HAVE_PNG15
#include <libpng15/png.h>
#else
#ifdef HAVE_PNG12
#include <libpng12/png.h>
#else
#include <png.h>
#endif
#endif
#endif
#include <cairo/cairo.h>

enum source_enum_t {
	SNOWMIX_SOURCE_NONE,
	SNOWMIX_SOURCE_IMAGE,
	SNOWMIX_SOURCE_FEED,
	SNOWMIX_SOURCE_FRAME
};


struct image_item_t {
	char*		file_name;
	char*		name;
	u_int32_t	id;
	u_int32_t	width;
	u_int32_t	height;

	// libpng parameters
	png_byte	bit_depth;
	png_byte	color_type;
	png_structp	png_ptr;
	png_infop	info_ptr;
	png_bytep*	row_pointers;

	// Pointer to BGRA data - 32 bit 
	u_int8_t*	pImageData;
};

struct image_place_t {
	u_int32_t	image_id;
	int32_t		x;
	int32_t		y;
	int32_t		anchor_x;
	int32_t		anchor_y;
	int32_t		offset_x;
	int32_t		offset_y;
	u_int32_t	align;
	bool		display;

	double		scale_x;
	double		scale_y;
	double		rotation;
	double		alpha;
	cairo_filter_t	filter;
	struct transform_matrix_t* pMatrix;

	// move parameters
	bool		update;			// Set to true if animation is to be performed
	int32_t		delta_x;
	u_int32_t	delta_x_steps;
	int32_t		delta_y;
	u_int32_t	delta_y_steps;
	int32_t		delta_offset_x;
	u_int32_t	delta_offset_x_steps;
	int32_t		delta_offset_y;
	u_int32_t	delta_offset_y_steps;
	double		delta_scale_x;
	u_int32_t	delta_scale_x_steps;
	double		delta_scale_y;
	u_int32_t	delta_scale_y_steps;
	double		delta_rotation;
	u_int32_t	delta_rotation_steps;
	double		delta_alpha;
	u_int32_t	delta_alpha_steps;
	//double	delta_alpha_bg;
	//u_int32_t	delta_alpha_bg_steps;
        double          delta_clip_left;
        u_int32_t       delta_clip_left_steps;
        double          delta_clip_right;
        u_int32_t       delta_clip_right_steps;
        double          delta_clip_top;
        u_int32_t       delta_clip_top_steps;
        double          delta_clip_bottom;
        u_int32_t       delta_clip_bottom_steps;


        // Clip parameters
        double                  clip_left;
        double                  clip_right;
        double                  clip_top;
        double                  clip_bottom;

};
	

// Max images to be hold and max images to be placed on video surface.
// Can be set arbitrarily.
#define	MAX_IMAGES		32
#define	MAX_IMAGE_PLACES	2*MAX_IMAGES

#include "controller.h"
#include "cairo_graphic.h"

class CVideoImage {
  public:
				CVideoImage(CVideoMixer* pVideoMixer,
					u_int32_t max_images = MAX_IMAGES,
					u_int32_t max_images_placed = MAX_IMAGE_PLACES);
				~CVideoImage();

	int			ParseCommand(CController* pController,
					struct controller_type* ctr, const char* str);

	char*			Query(const char* query, bool tcllist = true);
	unsigned int		MaxImages();
	struct image_item_t*	GetImageItem(unsigned int i);
	int			ImageItemSeqNo(u_int32_t image_id);
	int			ImagePlacedSeqNo(u_int32_t place_id);

	int			RemoveImagePlaced(u_int32_t place_id);
	unsigned int		MaxImagesPlace();
	struct image_place_t*	GetImagePlaced(unsigned int i);
	int			SetImageAlign(u_int32_t place_id, int align);

	void			UpdateMove(u_int32_t place_id);

	int			OverlayImage(const u_int32_t place_id, u_int8_t* pY,
					const u_int32_t width, const u_int32_t height);
	int			CreateImage(u_int32_t image_id, u_int8_t *source,
					u_int32_t width, u_int32_t height, const char* name = NULL);
	
  private:
	int 			set_image_help(struct controller_type* ctr,
					CController* pController, const char* str);

	int			set_image_load (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_name (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_write (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_align(struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_anchor(struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_alpha (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_rotation (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place_scale (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_place (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_source (struct controller_type *ctr,
					CController* pController, const char* ci);

	int			set_image_geometry (struct controller_type *ctr,
					CController* pController, const char* str);
	int			set_image_place_image (struct controller_type *ctr,
					CController* pController, const char* str);
	int 			set_image_place_move_scale (struct controller_type *ctr,
					CController* pController, const char* ci);
	int 			set_image_place_move_coor (struct controller_type *ctr,
					CController* pController, const char* ci);
	int 			set_image_place_move_offset (struct controller_type *ctr,
					CController* pController, const char* ci);
	int 			set_image_place_move_alpha (struct controller_type *ctr,
					CController* pController, const char* ci);
	int 			set_image_place_move_rotation (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_place_move_clip (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_place_clip (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_place_filter (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_place_offset (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_image_place_matrix (struct controller_type *ctr,
					CController* pController, const char* ci);
	int			set_verbose(struct controller_type* ctr,
					CController* pController, const char* str);


	int 			SetImageMoveAlpha(u_int32_t place_id, double delta_alpha,
					u_int32_t delta_alpha_steps);
	int 			SetImageMoveRotation(u_int32_t place_id, double delta_rotation,
					u_int32_t delta_rotation_steps);
	int 			SetImageMoveScale(u_int32_t place_id, double delta_scale_x,
					double delta_scale_y, u_int32_t delta_scale_x_steps,
					u_int32_t delta_scale_y_steps);
	int 			SetImageMoveCoor(u_int32_t place_id, int32_t delta_x,
					int32_t delta_y, u_int32_t delta_x_steps,
					u_int32_t delta_y_steps);
	int 			SetImageMoveOffset(u_int32_t place_id, int32_t delta_offset_x,
					int32_t delta_offset_y, u_int32_t delta_offset_x_steps,
					u_int32_t delta_offset_y_steps);

	int			WriteImage(u_int32_t image_id, const char* file_name);
	int			LoadImage(u_int32_t image_id, const char* file_name);
	int			SetImageName(u_int32_t image_id, const char* file_name);

	int			SetImageAlpha(u_int32_t place_id, double alpha);

	int			SetImageOffset(u_int32_t place_id, int32_t offset_x, int32_t offset_y);

	int			SetImageRotation(u_int32_t place_id, double rotation);

	int			SetImageScale(u_int32_t place_id, double scale_x, double scale_y);

	int			SetImagePlace(u_int32_t place_id, u_int32_t image_id, int32_t x,
					int32_t y, bool display, bool coor, const char* anchor = "nw");

	int			SetImagePlaceImage(u_int32_t place_id, u_int32_t image_id);

	int			SetImageClip(u_int32_t place_id, double clip_left, double clip_right,
					double clip_top, double clip_bottom);

	int			SetImageFilter(u_int32_t place_id, cairo_filter_t filter);

	int			SetImageMoveClip(u_int32_t place_id, double delta_clip_left,
					double delta_clip_right, double delta_clip_top,
					double delta_clip_bottom, u_int32_t step_left,
					u_int32_t step_right, u_int32_t step_top, u_int32_t step_bottom);

	int			SetImageAnchor(u_int32_t place_id, const char* anchor);

	int			SetImageMatrix(u_int32_t id, double xx, double xy, double yx, double yy,
					double x0, double y0);

	void			DeleteImageItem(u_int32_t image_id);
	int			SetImageSource(u_int32_t image_id, source_enum_t source_type,
					u_int32_t source_id, u_int32_t offset_x,
					u_int32_t offset_y, u_int32_t width,
					u_int32_t height, double scale_x, double scale_y,
					double rotation, double alpha, cairo_filter_t filter);

	FILE*			FileOpen(const char* file_name, const char* mode, char** open_name = NULL);

	CVideoMixer*		m_pVideoMixer;
	image_place_t**		m_image_places;		// Array to hold pointer to placed images
	image_item_t**		m_image_items;		// Array to hold loaded images
	u_int32_t		m_max_image_places;	// Max number in array
	u_int32_t		m_max_images;
	u_int32_t*		m_seqno_images;
	u_int32_t*		m_seqno_image_places;
	u_int32_t			m_verbose;

	//u_int32_t		m_max_images_placed;
	//struct image_place_t**	m_image_placed;
	//u_int32_t		m_width;
	//u_int32_t		m_height;
};
	
#endif	// VIDEO_IMAGE
