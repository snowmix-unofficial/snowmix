/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_FEED_H__
#define __VIDEO_FEED_H__

//#include <stdlib.h>
#include <sys/types.h>
#ifdef WIN32
#include "windows_util.h"
#else
#include <sys/time.h>
#endif
#include <cairo/cairo.h>

//#include "snowmix.h"
#include "controller.h"
#include "video_mixer.h"

#define MAX_VIDEO_FEEDS 12

// Struct holding use count for buffers of an area.
struct buffer_type {
	struct buffer_type	*prev;
	struct buffer_type	*next;

	struct area_list_type	*area_list;	/* Pointer to parrent area_list struct. */
	unsigned long		offset;		/* Offset relative to the base. */
	unsigned long		bsize;		/* Size of the buffer. */
	unsigned int		use;		/* Use count. */
};

// Struct describing a list of areas used by a feed.
struct area_list_type {
	struct area_list_type	*prev;
	struct area_list_type	*next;

	int			area_id;		// Area ID as received from the feed.
	char			*area_handle_name;	// Pointer to the area handle name.
	int			area_handle;		// File handle for the mapped area.
	unsigned long		area_size;		// Size of the shared memory area.
	u_int8_t		*buffer_base;		// Offset of where the shm buffer is mapped into memory.
	struct feed_type	*feed;			// Pointer to the feed owning this area.

	struct buffer_type	*buffers;		// List of buffers in use by this area.
};

// Struct to hold feed parameters
struct feed_type {
	struct feed_type	*prev;
	struct feed_type	*next;

	enum feed_state_enum	state;			/* State of the feed. */
	enum feed_state_enum	previous_state;		/* Previous state of the feed. */
	int			id;			/* ID number used for identifying
							   the feed to a controller. */
	struct timeval		start_time;		// Time feed changed to RUNNING
	char			*feed_name;		/* Descriptive name of the feed. */
	int			oneshot;		/* If true the feed is deleted on EOF. */
	int			is_live;		/* True if the feed is a live feed
							   (no pushback on source). */
	VideoType		m_video_type;		// Type of video BGRA, BGR ...

	unsigned int		width;			// Width of this feed.
	unsigned int		height;			// Height of this feed.
	unsigned int		cut_start_column;	// Cutout region specifiers
							//   (for PIP or Mosaic display).
	unsigned int		cut_start_row;
	int			cut_columns;
	int			cut_rows;
	int			par_w;			/* Pixel Aspect Ratio Width */
	int			par_h;			/* Pixel Aspect Ratio Height */
	int			scale_1;		/* Scale image to scale_1/scale_2 */
	int			scale_2;		/* Scale image to scale_1/scale_2 */
	double			alpha;			// Alpha blending
	cairo_filter_t		filter;			// Cairo Filter type.
							// CAIRO_FILTER_FAST, CAIRO_FILTER_GOOD, CAIRO_FILTER_BEST,
							// CAIRO_FILTER_NEAREST, CAIRO_FILTER_BILINEAR, CAIRO_FILTER_GAUSSIAN

	unsigned int		shift_columns;		/* Paint offset for PIP or Mosaic
							   display. */
	unsigned int		shift_rows;

	char			*dead_img_name;		/* Pointer to the name of the dead
							   feed image. */
	u_int8_t		*dead_img_buffer;	/* Pointer to the dead image. */
	int			dead_img_timeout;	/* Limit to when to start displaying
							   the dead image. */
	int			good_frames;		/* Number of received good frames. */
	int			missed_frames;		/* Number of missed frames. */
	int			dropped_frames;		/* Number of fifo overruns. */

	char			*control_socket_name;	/* Name of the unix domain control socket. */
	int			control_socket;		/* Socket for talking to the sender shmsink. */
	int			connect_retries;	/* Number of retries on connecting to the socket. */
	struct area_list_type	*area_list;		/* List of areas mapped by this feed. */

	int			max_fifo;		/* Max allowed number of fifo frames. */
	int			fifo_depth;		/* Number of elements in the fifo. */
	struct frame_fifo_type	frame_fifo [MAX_FIFO_DEPTH];	/* Pointer to the handle holding the last frames. */
	int			dequeued;		/* Flag to show that a frame have been dequeued from this feed. */
	u_int32_t		seq_no;			// Sequence number is incremented when frame changes
};

class CVideoFeeds {
  public:
	CVideoFeeds(CVideoMixer* pVideoMixer, u_int32_t max_feeds = MAX_VIDEO_FEEDS);
	~CVideoFeeds();
	u_int32_t		MaxFeeds() { return m_max_feeds; }

	int 			ParseCommand(struct controller_type* ctr, const char* ci);
	char*			Query(const char* query, bool tcllist = true);
	//int			UpdateMove(u_int32_t id);
	int 			output_select_feed (struct controller_type *ctr, int no);
	feed_state_enum*	PreviousFeedState(int id);
	feed_state_enum*	FeedState(int id);
	bool			FeedExist(int id);
	bool			FeedGeometry(int id, u_int32_t* width, u_int32_t* height);
	bool			FeedGeometryDetails(int id, u_int32_t* shift_columns, u_int32_t* shift_rows,
					u_int32_t* cut_start_column, u_int32_t* cut_start_row,
					u_int32_t* cut_columns, u_int32_t* cut_rows,
					u_int32_t* par_w, u_int32_t* par_h,
					u_int32_t* scale_1, u_int32_t* scale_2,
					double* alpha = NULL,
					cairo_filter_t* filter = NULL);
	struct feed_type* 	FindFeed (int id);
	struct feed_type*	GetFeedList() { return m_pFeed_list; }
	void			SetFeedList(struct feed_type* pFeed) { m_pFeed_list = pFeed; }
	int			SetFds (int maxfd, fd_set *rfds, fd_set *wfds);
	void			CheckRead (fd_set *rfds, u_int32_t block_size);
	void			Timertick (u_int32_t block_size, struct feed_type* feed_list);
	struct timeval*		GetFeedStartTime(int id);
	u_int8_t*		GetFeedFrame(int id, u_int32_t* pwidth = NULL, u_int32_t* pheight = NULL);
	int			GetPixelValue(u_int32_t feed_id, u_int32_t x, u_int32_t y, u_int8_t* pred = NULL,
					u_int8_t* pgreen = NULL, u_int8_t* pblue = NULL, u_int8_t* palpha = NULL);
	u_int32_t		FeedSeqNo(int id);

private:
	void			ReleaseBuffers (struct feed_type *feed);
	struct area_list_type*	FindArea (struct feed_type *feed, int area_id);
	struct buffer_type*	UseBuffer (struct feed_type *feed, int area_id, unsigned long offset, unsigned long bsize);
	void			AckBufferPtr (u_int32_t block_size, struct buffer_type *buffer);
	void			UseBufferPtr (struct buffer_type *buffer);


private:
	int		set_feed_drop(struct controller_type* ctr, const char* ci);
	int		DropFeedFrames(u_int32_t feed_id, u_int32_t drops);

	int 		list_help(struct controller_type* ctr, const char* str);
	int 		feed_info(struct controller_type* ctr, const char* str);
	int 		set_feed_add(struct controller_type* ctr, const char* ci);
	int 		set_feed_name(struct controller_type* ctr, const char* ci);
	int 		set_feed_par(struct controller_type* ctr, const char* ci);
	int 		set_feed_filter(struct controller_type* ctr, const char* ci);
	int 		set_feed_scale(struct controller_type* ctr, const char* ci);
	int 		set_feed_socket(struct controller_type* ctr, const char* ci);
	int 		set_feed_geometry(struct controller_type* ctr, const char* ci);
	int 		set_feed_idle(struct controller_type* ctr, const char* ci);
	int 		set_feed_alpha(struct controller_type* ctr, const char* ci);
	int		get_feed_pixel(struct controller_type* ctr, const char* ci);
	int 		set_feed_cutout(struct controller_type* ctr, const char* ci);
	int 		set_feed_shift(struct controller_type* ctr, const char* ci);
	int 		set_feed_live(struct controller_type* ctr, const char* ci);
	int 		set_feed_recorded(struct controller_type* ctr, const char* ci);
	int		set_feed_overlay(struct controller_type* ctr, const char* ci);
	int 		feed_add_new (struct controller_type *ctr, int id, char *name);
	int 		feed_list_scales (struct controller_type *ctr, int verbose);
	int 		feed_list_par (struct controller_type *ctr, int verbose);
	int 		feed_list_filter (struct controller_type *ctr, int verbose);
	int 		feed_list_feeds (struct controller_type *ctr, int verbose);
	int 		feed_set_name (struct controller_type *ctr, int id, char *name);
	int 		feed_set_socket (struct controller_type *ctr, int id, char *name);
	int 		feed_set_idle (struct controller_type *ctr, int id, int timeout, char *name);
	int 		feed_set_geometry (struct controller_type *ctr, int id, unsigned int width, unsigned int height);
	int 		feed_set_scale (struct controller_type *ctr, int id, int scale_1, int scale_2);
	int 		feed_set_par (struct controller_type *ctr, int id, int par_w, int par_h);
	int		feed_set_filter (struct controller_type *ctr, int id, cairo_filter_t filter);
	int		feed_set_alpha(u_int32_t feed_id, double alpha);
	int 		feed_set_cutout (struct controller_type *ctr, int id, unsigned int cut_start_column, unsigned int cut_start_row, unsigned int cut_columns, unsigned int cut_rows);
	int 		feed_set_shift (struct controller_type *ctr, int id, unsigned int shift_columns, unsigned int shift_rows);
	int 		feed_set_live (struct controller_type *ctr, int id);
	int 		feed_set_recorded (struct controller_type *ctr, int id);
	int 		feed_dump_buffers (struct controller_type *ctr);

	u_int32_t		m_max_feeds;
	//virtual_feed_t**	m_feeds;
	struct feed_type*	m_pFeed_list;
	CVideoMixer*		m_pVideoMixer;
	u_int32_t		m_feed_count;
};
	
#endif	// VIDEO_FEED_H
