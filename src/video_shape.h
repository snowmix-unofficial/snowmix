/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_SHAPE_H__
#define __VIDEO_SHAPE_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
//#include <sys/time.h>

#include <cairo/cairo.h>

#include "snowmix.h"
#include "video_mixer.h"

#define MAX_SHAPES		16
#define	MAX_SHAPE_PLACES	16
#define MAX_PATTERNS		16

enum pattern_type_enum_t {
	NO_PATTERN,
	LINEAR_PATTERN,
	RADIAL_PATTERN,
	MESH_PATTERN,		// Not implemented yet
	MATRIX_PATTERN		// Not implemented yet
};

enum draw_operation_enum_t {
	//UNDEFINED,
	ALPHAADD,
	ALPHAMUL,
	ARC_CW,
	ARC_CCW,
	ARCREL_CW,
	ARCREL_CCW,
	CLIP, CLOSEPATH, CURVETO, CURVEREL,
	FILL, FILLPRESERVE,
	FILTER,
	LINECAP, LINEJOIN, LINEREL, LINETO, LINEWIDTH,
	MASK_PATTERN, MOVEREL, MOVETO,
	NEWPATH, NEWSUBPATH,
	OPERATOR,
	PAINT,
	RECTANGLE, RECURSION, RESTORE, ROTATION,
	SCALE, SCALETELL, SOURCE_RGB, SOURCE_RGBA, SOURCE_PATTERN, STROKE, STROKEPRESERVE, SHAPE, SAVE,
	TRANSLATE, TRANSFORM,
	IMAGE,
	FEED
};
	
struct pattern_t {
	char*			name;
	u_int32_t		id;
	pattern_type_enum_t	type;
	cairo_pattern_t*	pattern;
	double			x1;
	double			y1;
	double			x2;
	double			y2;
	double			radius0;
	double			radius1;
	char*			create_str;
};

struct draw_operation_t {
	draw_operation_enum_t	type;
	u_int32_t		parameter;
	double			x;
	double			y;
	double			radius;		// x2 for curveto/curverel
	double			width;		// y2 for curveto/curverel
	double			angle_from;	// width for rectangle, x3 for curveto/curverel
	double			angle_to;	// height for rectangle, y3 for curveto/curverel
	double			red;
	double			green;
	double			blue;
	double			alpha;
	char*			create_str;
	struct draw_operation_t*	next;
};

struct shape_t {
	char*			name;			// Name of shape
	u_int32_t		id;			// id of shape
	u_int32_t		ops_count;		// Number of draw operations
	draw_operation_t*	pDraw;			// List of drawing operations
	draw_operation_t*	pDrawTail;		// Tail of list
};

struct placed_shape_t {
	u_int32_t		shape_id;
	double			x;
	double			y;
	double			offset_x;
	double			offset_y;
	int32_t			anchor_x;
	int32_t			anchor_y;
	double			scale_x;
	double			scale_y;
	double			rotation;
	double			red;
	double			green;
	double			blue;
	double			alpha;

	// move parameters
	bool			update;

	double			delta_x;
	double			delta_y;
	u_int32_t		steps_x;
	u_int32_t		steps_y;

	double			delta_offset_x;
	double			delta_offset_y;
	u_int32_t		steps_offset_x;
	u_int32_t		steps_offset_y;

	double			delta_scale_x;
	double			delta_scale_y;
	u_int32_t		steps_scale_x;
	u_int32_t		steps_scale_y;

	double			delta_alpha;
	u_int32_t		steps_alpha;

	double			delta_rotation;
	u_int32_t		steps_rotation;

	// Extent
	double			ext_x1, ext_y1, ext_x2, ext_y2;

};


class CVideoShape {
  public:
	CVideoShape(CVideoMixer* pVideoMixer, u_int32_t max_shapes = MAX_SHAPES,
		u_int32_t max_shape_places = MAX_SHAPE_PLACES,
		u_int32_t max_patterns = MAX_PATTERNS);
	~CVideoShape();
	char*			Query(const char* query, bool tcllist = true);
	u_int32_t		MaxShapes() { return m_max_shapes; }
	u_int32_t		MaxPlaces() { return m_max_shape_places; }
	u_int32_t		MaxPatterns() { return m_max_patterns; }
	int 			ParseCommand(struct controller_type* ctr, const char* ci);
	void			Update();
	placed_shape_t*		GetPlacedShape(u_int32_t);
	shape_t*		GetShape(u_int32_t);
	pattern_t*		GetPattern(u_int32_t pattern_id);
	cairo_pattern_t*	GetCairoPattern(u_int32_t pattern_id);

private:

	int		overlay_placed_shape(struct controller_type* ctr, const char *str);
	int		set_verbose(struct controller_type* ctr, const char* str);
	int		list_shape_info(struct controller_type* ctr, const char* str);
	int		list_shape_help(struct controller_type* ctr, const char* str);
	int		list_shape_place_help(struct controller_type* ctr, const char* str);
	int		list_shape_status(struct controller_type* ctr, const char* str);
	int		set_shape_moveentry(struct controller_type* ctr, const char* str);
	int		set_shape_place_move_coor(struct controller_type* ctr, const char* str);
	int		set_shape_place_move_offset(struct controller_type* ctr, const char* str);
	int		set_shape_place_move_scale(struct controller_type* ctr, const char* str);
	int		set_shape_place_move_alpha(struct controller_type* ctr, const char* str);
	int		set_shape_place_move_rotation(struct controller_type* ctr, const char* str);
	int		set_shape_place_coor(struct controller_type* ctr, const char* str);
	int		set_shape_place_shape(struct controller_type* ctr, const char* str);
	int		set_shape_place_scale(struct controller_type* ctr, const char* str);
	int		set_shape_place_offset(struct controller_type* ctr, const char* str);
	int		set_shape_place_rotation(struct controller_type* ctr, const char* str);
	int		set_shape_place_rgb(struct controller_type* ctr, const char* str);
	int		set_shape_place_alpha(struct controller_type* ctr, const char* str);
	int		set_shape_place_anchor(struct controller_type* ctr, const char* str);
	int		set_shape_place(struct controller_type* ctr, const char* str);
	int		set_shape_add(struct controller_type* ctr, const char* str);
	int		set_pattern_add(struct controller_type* ctr, const char* str);
	int		set_pattern_radial(controller_type*, const char*);
	int		set_pattern_linear(controller_type*, const char*);
	int		set_pattern_stoprgba(controller_type*, const char*);
	int		set_shape_list(struct controller_type* ctr, const char* str);
	int		set_shape_xy(struct controller_type* ctr, const char* str,
				draw_operation_enum_t type);
	int		set_shape_arc(struct controller_type* ctr, const char* str,
				draw_operation_enum_t type);
	int		set_shape_no_parameter(struct controller_type* ctr,
				const char* str, draw_operation_enum_t type);
	int		set_shape_one_parameter(struct controller_type* ctr,
				const char* str, draw_operation_enum_t type);
	int		set_shape_source_rgb(struct controller_type* ctr, const char* str);
	int		set_shape_source_pattern(struct controller_type* ctr, const char* str);
	int		set_shape_mask_pattern(struct controller_type* ctr, const char* str);
	int		set_shape_rectangle(struct controller_type* ctr, const char* str);
	int		set_shape_rotation(struct controller_type* ctr, const char* str);
	int		set_shape_curve(struct controller_type* ctr, const char* str,
				draw_operation_enum_t type);
	int		set_shape_line_join(struct controller_type* ctr, const char* str);
	int		set_shape_filter(struct controller_type* ctr, const char* str);
	int		set_shape_operator(struct controller_type* ctr, const char* str);
	int		set_shape_transform(struct controller_type* ctr, const char* str);
	int		set_shape_image(struct controller_type* ctr, const char* str);
	int		set_shape_feed(struct controller_type* ctr, const char* str);

	int		SetShapeAnchor(u_int32_t place_id, const char* anchor);
	int		ShapeOps(u_int32_t shape_id);
	int		AddToShapeCommand(u_int32_t shape_id, draw_operation_enum_t type);
	int		AddToShapeWithParameter(u_int32_t shape_id,
				draw_operation_enum_t type, double parameter);
	int		AddToShapeFilter(u_int32_t shape_id, cairo_filter_t filter);
	int		AddToShapeOperator(u_int32_t shape_id, cairo_operator_t op);
	int		AddToShapeXY(u_int32_t shape_id, draw_operation_enum_t type,
				double x, double y);
	int		AddToShapeLineJoin(u_int32_t shape_id, double join, double miter);
	int		AddToShapeImage(u_int32_t shape_id, u_int32_t image_id,
				double x, double y, double scale_x, double scale_y);
	int		AddToShapeFeed(u_int32_t shape_id, u_int32_t image_id,
				double x, double y, double scale_x, double scale_y);
	int		AddToShapeArc(u_int32_t shape_id, draw_operation_enum_t type,
				double x, double y, double radius, double angle_from,
				double angle_to);
	int		AddToShapeCurve(u_int32_t shape_id, draw_operation_enum_t type,
				double x1, double y1, double x2, double y2,
				double x3, double y3);
	int		AddToShapeRotation(u_int32_t shape_id, double rotation,
				double absolute_rotation, bool absolute);
	int		AddToShapeTransform(u_int32_t shape_id, double xx, double yx,
				double xy, double yy, double x0, double y0);
	//bool		NumberWithPI(const char* str, double* res);
	int		AddToShapeSourcePattern(u_int32_t shape_id, u_int32_t pattern_id);
	int		AddToShapeMaskPattern(u_int32_t shape_id, u_int32_t pattern_id);
	int		AddToShapeSourceRGBA(u_int32_t shape_id, double red, double green,
				double blue, double alpha);
	int		AddShape(u_int32_t shape_id, const char* name);
	int		AddPattern(u_int32_t shape_id, const char* name);
	int		PatternCreateRadial(u_int32_t pattern_id, double cx0, double cy0,
				double radius0, double cx1, double cy1, double radius1);
	int		PatternCreateLinear(u_int32_t pattern_id, double x1, double y1,
				double x2, double y2);
	int		PatternStopRGBA(u_int32_t pattern_id, double offset,
				double red, double green, double blue, double alpha);
	int		ShapeMoveEntry(u_int32_t shape_id, u_int32_t entry_id,
				u_int32_t to_entry);
	int		SetPlacedShapeAlpha(u_int32_t place_id, double alpha);
	int		SetPlacedShapeCoor(u_int32_t place_id, double x, double y, const char* anchor);
	int		SetPlacedShapeMoveCoor(u_int32_t place_id, double delta_x,
				double delta_y, u_int32_t steps_x, u_int32_t steps_y);
	int		SetPlacedShapeMoveOffset(u_int32_t place_id, double delta_x,
				double delta_y, u_int32_t steps_x, u_int32_t steps_y);
	int		SetPlacedShapeMoveScale(u_int32_t place_id, double delta_x,
				double delta_y, u_int32_t steps_x, u_int32_t steps_y);
	int		SetPlacedShapeMoveAlpha(u_int32_t place_id, double delta_alpha,
				u_int32_t steps_alpha);
	int		SetPlacedShapeMoveRotation(u_int32_t place_id, double delta_rotation,
				u_int32_t steps_rotation);
	int		SetPlacedShapeRGB(u_int32_t place_id, double red, double green,
				double blue);
	int		AddToShapeRectangle(u_int32_t shape_id, double x, double y, double width, double height);
	int		SetPlacedShapeRotation(u_int32_t place_id, double rotation);
	int		SetPlacedShapeScale(u_int32_t place_id, double scale_x, double scale_y);
	int		SetPlacedShapeOffset(u_int32_t place_id, double offset_x, double offset_y);
	int		SetPlacedShapeShape(u_int32_t place_id, u_int32_t shape_id);
	int		AddPlacedShape(u_int32_t place_id, u_int32_t shape_id, double x,
				double y, double scale_x, double scale_y, double rotation,
				double red, double green, double blue, double alpha,
				const char* anchor);
	int		DeleteShape(u_int32_t shape_id);
	int		DeletePlaced(u_int32_t place_id);
	int		DeletePattern(u_int32_t pattern_id);

	char*		DuplicateCommandString(const char* str);


/*
	int 		list_help(struct controller_type* ctr, const char* str);
	int 		list_info(struct controller_type* ctr, const char* str);
	int 		set_verbose(struct controller_type* ctr, const char* ci);

	int		CreateShape(u_int32_t shape_id);
	int		DeleteShape(u_int32_t shape_id);
	int		SetShapeName(u_int32_t shape_id);
	int		CreateDrawPlace(u_int32_t place_id, u_int32_t shape_id, double x, double y);
*/

	u_int32_t		m_max_shapes;
	u_int32_t		m_max_shape_places;
	u_int32_t		m_max_patterns;
	shape_t**		m_shapes;
	placed_shape_t**	m_placed_shapes;
	pattern_t**		m_patterns;

	CVideoMixer*		m_pVideoMixer;
	u_int32_t		m_shape_count;
	u_int32_t		m_shape_placed_count;
	u_int32_t		m_pattern_count;
	u_int32_t		m_verbose;
	bool			m_update;
};
	
#endif	// __VIDEO_SHAPE_H__
