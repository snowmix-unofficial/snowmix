/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __CAIRO_GRAPHIC_H__
#define __CAIRO_GRAPHIC_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
#include <pango/pangocairo.h>
// #include <sys/time.h>

// #define	MAX_STRINGS	64
// #define	MAX_FONTS	64
// #define	MAX_TEXT_PLACES	64

// #define	TEXT_ALIGN_LEFT   1
// #define	TEXT_ALIGN_CENTER 2
// #define	TEXT_ALIGN_RIGHT  4
// #define	TEXT_ALIGN_TOP    8
// #define	TEXT_ALIGN_MIDDLE 16
// #define	TEXT_ALIGN_BOTTOM 32

#define OVERLAY_SHAPE_LEVELS 10		// Max number of recursions for shape in shapes

// Cairo Matrix
struct transform_matrix_t {

	double  matrix_xx;
	double  matrix_yx;
	double  matrix_xy;
	double  matrix_yy;
	double  matrix_x0;
	double  matrix_y0;
};


#include "virtual_feed.h"

class CVideoMixer;

class CCairoGraphic {
  public:
	CCairoGraphic(u_int32_t width, u_int32_t height, u_int8_t* data = NULL);
	~CCairoGraphic();

	// Overlay frame an image
	void	OverlayFrameOld(u_int8_t* src_buf,			// Pointer to src buffer
								// (image to overlay)
        		u_int32_t src_width, u_int32_t src_height,      // width and height of src
        		int32_t x, int32_t y,			// x,y of clip
        		int32_t offset_x, int32_t offset_y,	// offset x,y of clip
        		u_int32_t w, u_int32_t h,		// width and height of clip
			u_int32_t src_x, u_int32_t src_y,	// start pixel of src to be shown
			u_int32_t align = SNOWMIX_ALIGN_TOP | SNOWMIX_ALIGN_LEFT, // Alignment
        		double scale_x = 1.0,			// scale of overlay image
			double scale_y = 1.0,			// scale of overlay image
        		double rotate = 0.0,			// Rotation
        		double alpha = 1.0,			// Blend factor
			cairo_filter_t = CAIRO_FILTER_FAST,	// Filter type
			transform_matrix_t* pTransformMatrix = NULL
			);
	void	OverlayFrame(u_int8_t* src_buf,			// Pointer to src buffer
								// (image to overlay)
        		u_int32_t src_width, u_int32_t src_height,      // width and height of src
        		int32_t x, int32_t y,			// x,y of clip
        		int32_t offset_x, int32_t offset_y,	// offset x,y of clip
        		u_int32_t w, u_int32_t h,		// width and height of clip
			u_int32_t src_x, u_int32_t src_y,	// start pixel of src to be shown
			u_int32_t align = SNOWMIX_ALIGN_TOP | SNOWMIX_ALIGN_LEFT, // Alignment
        		double scale_x = 1.0,			// scale of overlay image
			double scale_y = 1.0,			// scale of overlay image
        		double rotate = 0.0,			// Rotation
        		double alpha = 1.0,			// Blend factor
			cairo_filter_t = CAIRO_FILTER_FAST,	// Filter type
			transform_matrix_t* pTransformMatrix = NULL
			);


	// Using pango text
	void			OverlayTextOld(CVideoMixer* pVideoMixer, u_int32_t place_id);
	void			OverlayText(CVideoMixer* pVideoMixer, u_int32_t place_id);

	void			OverlayImage(CVideoMixer* pVideoMixer, u_int32_t place_id);
	void			OverlayVirtualFeed(CVideoMixer* pVideoMixer, virtual_feed_t* pFeed);

	void			OverlayPlacedShape(CVideoMixer* pVideoMixer, u_int32_t place_id);
	void			OverlayShape(CVideoMixer* pVideoMixer, u_int32_t shape_id,
					double alpha, double scale_x, double scale_y,
					double* rotation, u_int32_t* save, u_int32_t level);

	// This is just for test of cairo text. pango text is prefered.
	void			SetTextExtent(u_int32_t x, u_int32_t y, char* str,
					char* font_str = NULL, double red = 0.0,
					double green = 0.0, double blue = 0.0);

	//void			OverlayImage(CVideoMixer* pVideoMixer, u_int32_t place_id);

	cairo_status_t		WriteFilePNG(cairo_surface_t* surface, char* filename);
	//void			Rotate(double rotate) { if (m_pCr) cairo_rotate(m_pCr, rotate); }

	cairo_surface_t*	m_pSurface;

  private:
	void SetFont(char* font_str = NULL);
	u_int32_t		m_width;
	u_int32_t		m_height;
	cairo_t*		m_pCr;
	//cairo_status_t		m_status;
	PangoLayout*		m_pLayout;
	PangoFontDescription*	m_pDesc;
};

#endif	// CAIRO_GRAPHIC
