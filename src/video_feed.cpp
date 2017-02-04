/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


#ifndef WIN32
#include <unistd.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#else
#include <winsock.h>
#include <io.h>
#endif
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
#include <sys/types.h>
#include <math.h>
#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_MMAN
#include <sys/mman.h>
#endif

#include "video_feed.h"
#include "snowmix_util.h"


CVideoFeeds::CVideoFeeds(CVideoMixer* pVideoMixer, u_int32_t max_feeds)
{
	m_max_feeds = max_feeds;
	m_pFeed_list = NULL;
	m_feed_count = 0;
	m_pVideoMixer = pVideoMixer;
}

CVideoFeeds::~CVideoFeeds()
{
	while (m_pFeed_list) {
		struct feed_type* p = m_pFeed_list->next;
		free(m_pFeed_list);
		m_pFeed_list = p;
	}
	m_max_feeds = 0;
}

int CVideoFeeds::ParseCommand(struct controller_type* ctr, const char* ci)
{
	if (!m_pVideoMixer) return 0;

	// Commands are checked in the order of runtime importance
	// Commands usually only executed at startup or seldom used
	// are placed last
	// feed info
	if (!strncasecmp (ci, "info ", 5)) {
		return feed_info (ctr, ci+5);
	}
	// feed overlay (<id> | <id>..<id> | all | end | <id>..end) [ (<id> | <id>..<id> | all | end | <id>..end) ] ....
	else if (!strncasecmp (ci, "overlay ", 8)) {
		if (!m_pVideoMixer) return 1;
		return set_feed_overlay(ctr, ci+8);
	}
	// feed fast overlay <feed id> <col> <row> <feed col> <feed row> <cut cols> <cut rows> [ <scale 1> <scale 2> [ <par_w> <par_h> [ c ]]]
	else if (!strncasecmp (ci, "fast overlay ", 13)) {
		if (!m_pVideoMixer) return 1;
		return m_pVideoMixer->OverlayFeed(ci+13);
	}
	// feed pixel
	else if (!strncasecmp (ci, "pixel ", 6)) {
		return get_feed_pixel(ctr, ci+6);
	}
	// feed alpha
	else if (!strncasecmp (ci, "alpha ", 6)) {
		return set_feed_alpha(ctr, ci+6);
	}
	// feed cutout
	else if (!strncasecmp (ci, "cutout ", 7)) {
		return set_feed_cutout(ctr, ci+7);
	}
	// feed shift
	else if (!strncasecmp (ci, "shift ", 6)) {
		return set_feed_shift(ctr, ci+6);
	}
	// feed list verbose
	else if (!strncasecmp (ci, "list verbose ", 13)) {
		return feed_list_feeds (ctr, 1);
	}
	// feed list
	else if (!strncasecmp (ci, "list ", 5)) {
		return feed_list_feeds (ctr, 0);
	}
	// feed scale
	else if (!strncasecmp (ci, "scale ", 6)) {
		return set_feed_scale(ctr, ci+6);
	}
	// feed buffers
	else if (!strncasecmp (ci, "buffers ", 8)) {
		return feed_dump_buffers (ctr);
	}
	// feed name
	else if (!strncasecmp (ci, "name ", 5)) {
		return set_feed_name(ctr, ci+5);
	}
	// feed live
	else if (!strncasecmp (ci, "live ", 5)) {
		return set_feed_live(ctr, ci+5);
	}
	// feed recorded
	else if (!strncasecmp (ci, "recorded ", 9)) {
		return set_feed_recorded(ctr, ci+9);
	}
	// feed help
	else if (!strncasecmp (ci, "help ", 5)) {
		return list_help (ctr, ci+5);
	}
	// feed add
	else if (!strncasecmp (ci, "add ", 4)) {
		return set_feed_add(ctr, ci+4);
	}
	// feed filter
	else if (!strncasecmp (ci, "filter ", 7)) {
		return set_feed_filter(ctr, ci+7);
	}
	// feed par
	else if (!strncasecmp (ci, "par ", 4)) {
		return set_feed_par(ctr, ci+4);
	}
	// feed socket
	else if (!strncasecmp (ci, "socket ", 7)) {
		return set_feed_socket(ctr, ci+7);
	}
	// feed geometry
	else if (!strncasecmp (ci, "geometry ", 9)) {
		return set_feed_geometry(ctr, ci+9);
	}
	// feed idle
	else if (!strncasecmp (ci, "idle ", 5)) {
		return set_feed_idle(ctr, ci+5);
	}
	// feed drop
	else if (!strncasecmp (ci, "drop ", 5)) {
		return set_feed_drop(ctr, ci+5);
	}
	// feed idle
	else if (!strncasecmp (ci, "seqno ", 6)) {
		int id;
		ci += 6;
		while (isspace(*ci)) ci++;
		if (sscanf(ci, "%d", &id) != 1) return -1;
		feed_type* pFeed = FindFeed(id);
		if (pFeed && m_pVideoMixer->m_pController)
			m_pVideoMixer->m_pController->controller_write_msg(ctr, MESSAGE" feed %d seq_no %u\n", id, pFeed->seq_no);
		else return 1;
	} else return 1;

	return 0;
}

// Create a virtual feed
int CVideoFeeds::list_help(struct controller_type* ctr, const char* ci)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	m_pVideoMixer->m_pController->controller_write_msg (ctr, MESSAGE"Commands:\n"
		MESSAGE"feed add [<feed no> <feed name>]\n"
		MESSAGE"feed buffers\n"
		MESSAGE"feed alpha [<feed id> <alpha>]\n"
		MESSAGE"feed cutout <feed no> <start col> <start row> <columns> <rows>\n"
		MESSAGE"feed drop <feed id> <n drops>\n"
		MESSAGE"feed geometry <feed no> <width> <height>\n"
		MESSAGE"feed help // this list\n"
		MESSAGE"feed idle <feed no> <timeout in frames> <idle image file>\n"
		MESSAGE"feed info\n"
		MESSAGE"feed fast overlay <feed id> <col> <row> <feed col> <feed row> <cut cols> <cut rows> [ <scale 1> <scale 2> [ <par_w> <par_h> [ c ]]]\n"
		MESSAGE"feed filter [ <feed no> (fast | good | best | nearest | bilinear | gaussian)]\n"
		MESSAGE"feed list [verbose]\n"
		MESSAGE"feed live <feed no>\n"
		MESSAGE"feed name <feed no> <feed name>\n"
		MESSAGE"feed overlay (<id> | <id>..<id> | all | end | <id>..end) [ (<id> | <id>..<id> | all | end | <id>..end) ] ....\n"
		MESSAGE"feed par [<feed no> <width> <height>]\n"
		MESSAGE"feed pixel [<feed no> <x> <y>]\n"
		MESSAGE"feed recorded <feed no>\n"
		MESSAGE"feed scale [<feed no> <scale_1> <scale_2>]\n"
		MESSAGE"feed shift <feed no> <column> <row>\n"
		MESSAGE"feed socket <feed no> <socket name>\n"
		MESSAGE"\n");
	return 0;
}


// Query type 0=geometry 1=status 2=extended 3=state
// format	: format 
// ids		: list of IDs
// maxid	: max number of ID
// nextavail	: Next available ID
// id or ids	: listing each ID
char* CVideoFeeds::Query(const char* query, bool tcllist)
{
	int len;
	char *pstr, *retstr;
	const char *str;
	u_int32_t type, maxid, x, y;

	if (!query) return NULL;
	while (isspace(*query)) query++;
       	if (!(strncasecmp("state ", query, 6))) {
		query += 6; type = 3;
	}
	else if (!(strncasecmp("geometry ", query, 9))) {
		query += 9; type = 0;
	}
       	else if (!(strncasecmp("status ", query, 7))) {
		query += 7; type = 1;
	}
       	else if (!(strncasecmp("extended ", query, 9))) {
		query += 9; type = 2;
	}
       	else if (!(strncasecmp("pixel ", query, 6))) {
		query += 6; type = 4;
		while (isspace(*query)) query++;
		if (sscanf(query, "%u %u", &x, &y) != 2) return NULL;
		while (*query && isdigit(*query)) query++;
		while (isspace(*query)) query++;
		while (*query && isdigit(*query)) query++;
		while (isspace(*query)) query++;
	}
       	else if (!(strncasecmp("syntax", query, 6))) {
		return strdup("feed (geometry | status | extended | state | pixel <x> <y> | "
			"syntax) (format | ids | maxid | nextavail | <id_list>)");
	} else return NULL;

	while (isspace(*query)) query++;
	maxid = MaxFeeds();

	if (!strncasecmp(query, "format", 6)) {
		char* str = NULL;
		if (type == 0) {
			// geometry - feed_id offset geometry cut_start cut_geometry par scale alpha
			// geometry - feed <offset> <width>x<height> <cut_start_col>,<cut_start_row> <cut_col>x<cut_row> <par_w>:<par_h> <scale_x>:<scale_y> <alpha>");
			// status   - feed_id id state start_time good missed dropped fifo_depth dequeued retries
			// extended - feed_id id feed_name is_live max_fifo dead_timeout dead_img_name
			if (tcllist) str = strdup("id offset geometry cut_start cut_geometry par scale alpha filter");
			else str = strdup("feed <id> <offset> <geometry> <cut_start> <cut_geometry> <par> <scale> <alpha> <filter>");

		}
		else if (type == 1) {
			if (tcllist) str = strdup("id state start_time good missed dropped fifo_depth dequeued retries");
			else str = strdup("feed <id> <state> <start_time> <good_frames> <missed_frames> <dropped_frames> "
				"<fifo_depth> <dequeued> <retries>");
		}
		else if (type == 2) {
			if (tcllist) str = strdup("id feed_name is_live max_fifo dead_timeout dead_img_name");
			else str = strdup("feed <id> <feed_name> <is_live> <max_fifo> <dead_timeout> <dead_img_name>");
		}
		else if (type == 3) {
			if (tcllist) str = strdup("id state");
			else str = strdup("feed state <id> <state>");
		}
		else if (type == 4) {
			if (tcllist) str = strdup("id dead red green blue alpha");
			else str = strdup("feed <id> <dead> <red> <green> <blue>");
		}
		if (!str) goto malloc_failed;
		return str;
	}
	else if (!strncasecmp(query, "maxid", 5)) {
		len = (typeof(len))(log10((double)maxid)) + 3;	// in case of rounding error
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
		
		len = (typeof(len))(log10((double)maxid)) + 3;	// in case of rounding error
		char* str = (char*) malloc(len);
		if (!str) goto malloc_failed;
		*str = '\0';
		for (u_int32_t i = from; i < maxid ; i++) {
			if (!FindFeed(i)) {
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
		len = (typeof(len))(0.5 + log10((double)maxid)) + 1;
		len *= maxid;
		char* str = (char*) malloc(len+1);
		if (!str) goto malloc_failed;
		*str = '\0';
		char* p = str;
		for (u_int32_t i = 0; i < maxid ; i++) {
			if (FindFeed(i)) {
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
		feed_type* pf;
		while (from <= to) {
			if (!(pf = FindFeed(from))) { from++ ; continue; }
			// geometry - feed_id offset geometry cut_start cut_geometry par scale alpha
			// status   - feed_id state start_time good missed dropped fifo_depth dequeued retries
			// extended - feed_id feed_name is_live max_fifo dead_timeout dead_img_name
			// state - id state
			// state - feed state <id> <state>
			if (type < 2) len += 138;
			else if (type == 2) { len += 64;
				if (pf->feed_name) len += strlen(pf->feed_name);
				if (pf->dead_img_name) len += strlen(pf->dead_img_name);
			}
			else if (type == 3) len += 35;
			// feed 1234567890 <name> recorded 1234567890 1234567890 <name>
			// {1234567890 {name} recorded 1234567890 1234567890 {name}
			// 1234567890123    4567890123456789012345678901234567    8901234 +strlen(feedname)+strlen(deadname) + 2
			// feed state 1234567890 DISCONNECTED
			// { 1234567890 DISCONNECTED } 
			// 12345678901234567890123456789012345
			else if (type == 4) len += 34;
			// feed <id> <dead> <red> <green> <blue>");
			// feed 0123456789 dead 123 123 123
			// 123456789012345678901234567890123
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	if (len < 2) {
		fprintf(stderr, "CVideoFeeds::Query malformed query\n");
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
			feed_type* pf;
			if (!(pf = FindFeed(from))) { from++ ; continue; }

			// geometry - feed_id offset geometry cut_start cut_geometry par scale alpha
			// feed <offset> <width>x<height> <cut_start_col>,<cut_start_row> <cut_col>x<cut_row> <par_w>:<par_h> <scale_x>:<scale_y> <alpha>");
			if (type == 0) {
				if (tcllist) sprintf(pstr, "{%u {%u %u} {%u %u} {%u %u} {%d %d} {%d %d} {%d %d} %.5lf %s} ",
					from, pf->shift_columns, pf->shift_rows, pf->width, pf->height,
					pf->cut_start_column, pf->cut_start_row, pf->cut_columns, pf->cut_rows,
					pf->par_w, pf->par_h, pf->scale_1, pf->scale_2, pf->alpha, CairoFilter2String(pf->filter));
				else sprintf(pstr, "feed %u %u,%u %ux%u %u,%u %dx%d %d:%d %d:%d %.5lf %s",
					from, pf->shift_columns, pf->shift_rows, pf->width, pf->height,
					pf->cut_start_column, pf->cut_start_row, pf->cut_columns, pf->cut_rows,
					pf->par_w, pf->par_h, pf->scale_1, pf->scale_2, pf->alpha, CairoFilter2String(pf->filter));
			}
			// if (tcllist) str = strdup("id state start_time good missed dropped fifo_depth dequeued retries");
			// else str = strdup("feed <id> <state> <start_time> <good_frames> <missed_frames> <dropped_frames> "
			//	<fifo_depth> <dequeued> <retries>");
			else if (type == 1) {
				sprintf(pstr, "%s%u %s "
#ifdef HAVE_MACOSX		  // suseconds_t is int on OS X 10.9
					"%ld.%06d "
#else			       // suseconds_t is long int on Linux
					"%ld.%06lu "
#endif
					"%u %u %u %d %d %d%s",
					(tcllist ? "{" : "feed "), from, feed_state_string(pf->state),
					pf->start_time.tv_sec, pf->start_time.tv_usec,
					pf->good_frames, pf->missed_frames, pf->dropped_frames,
					pf->fifo_depth, pf->dequeued, pf->connect_retries,
					(tcllist ? "} " : "\n"));
			}

			// if (tcllist) str = strdup("id feed_name is_live max_fifo dead_timeout dead_img_name");
			// else str = strdup("feed <id> <<feed_name>> <is_live> <max_fifo> <dead_timeout> <<dead_img_name>>");
			else if (type == 2) {
				if (tcllist) sprintf(pstr, "{%u {%s} %s %d %d {%s}} ",
					from, pf->feed_name, pf->is_live ? "live" : "recorded",
					pf->max_fifo, pf->dead_img_timeout, pf->dead_img_name ? pf->dead_img_name : "");
				else sprintf(pstr, "feed %u <%s> %s %d %d <%s>\n",
					from, pf->feed_name, pf->is_live ? "live" : "recorded",
					pf->max_fifo, pf->dead_img_timeout, pf->dead_img_name ? pf->dead_img_name : "");
			}
			else if (type == 3) {
				sprintf(pstr, "%s%u %s%s",
					(tcllist ? "{" : "feed state "),
					from,
					feed_state_string(pf->state),
					(tcllist ? "} " : "\n"));
			}
			else if (type == 4) {
				u_int8_t red, green, blue, alpha;
				int n = GetPixelValue(from, x, y, &red, &green, &blue, &alpha);
				if (n < 0) {
					// feed <id> <dead> <red> <green> <blue>");
					// feed 0123456789 dead 123 123 123
					sprintf(pstr, "%s%u fail - - - -%s",
						(tcllist ? "{" : "feed "),
						from,
						(tcllist ? "} " : "\n"));
				} else {
					sprintf(pstr, "%s%u %s %hhu %hhu %hhu %hhu%s",
						(tcllist ? "{" : "feed "),
						from,
						n ? "dead" : "feed",
						red, green, blue, alpha,
						(tcllist ? "} " : "\n"));
				}
			}
			while (*pstr) pstr++;
			from++;
		}
		while (isspace(*nextstr)) nextstr++;
		str = nextstr;
	}
	return retstr;

malloc_failed:
	fprintf(stderr, "CVideoFeeds::Query failed to allocate memory\n");
	return NULL;
}
int CVideoFeeds::feed_info(struct controller_type* ctr, const char* ci)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;

	// Count feed
	struct feed_type	*feed;
	timeval timenow;
	gettimeofday(&timenow, NULL);
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"Feed info:\n"
		STATUS"Feed count : %u of %d\n"
		STATUS"time       : %u.%u\n"
		STATUS"feed id : state islive oneshot geometry cutstart cutsize offset fifo good missed dropped <name>\n",
		m_feed_count, (int)MAX_VIDEO_FEEDS, timenow.tv_sec, (u_int32_t)(timenow.tv_usec/1000)
		);
	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"feed %d : %s %s %s %ux%u %u,%u %dx%d %u,%u %d:%d %u %u %u <%s>\n",
			feed->id, feed_state_string(feed->state),
			feed->is_live ? "live" : "recorded", feed->oneshot ? "oneshot" : "continuously",
			feed->width, feed->height, feed->cut_start_column, feed->cut_start_row,
			feed->cut_columns, feed->cut_rows,
			feed->shift_columns, feed->shift_rows,
			feed->fifo_depth, feed->max_fifo,
			feed->good_frames, feed->missed_frames, feed->dropped_frames,
			feed->feed_name ? feed->feed_name : ""
			);
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");
	return 0;
}

// feed add
int CVideoFeeds::set_feed_add(struct controller_type* ctr, const char* ci)
{
	int	no;
	char	name [100];

	if (!m_pVideoMixer || ! m_pVideoMixer->m_pController || !ci) return 1;
	if (!m_pVideoMixer->GetSystemWidth() ||
		!m_pVideoMixer->m_frame_rate) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Setup geometry and frame rate first\n");
		return 1;
	}
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		struct feed_type	*feed;
		for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"feed %d <%s>\n", feed->id,
				feed->feed_name ? feed->feed_name : "");
		}
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			STATUS"\n");
		return 0;
	}

	memset (name, 0, sizeof (name));
	int x = sscanf (ci, "%d %99c", &no, &name [0]);
	if (x != 2) return -1;
	return feed_add_new (ctr, no, name);
}

// feed name
int CVideoFeeds::set_feed_name(struct controller_type* ctr, const char* ci)
{
	int	no;
	char	name [100];

	if (!m_pVideoMixer || ! m_pVideoMixer->m_pController) return 1;
	memset (name, 0, sizeof (name));
	int x = sscanf (ci, "%d %99c", &no, &name [0]);
	if (x != 2) return -1;
	return feed_set_name (ctr, no, name);
}

// feed par
int CVideoFeeds::set_feed_par(struct controller_type* ctr, const char* ci)
{
	if (!(*ci)) return feed_list_par (ctr, 0);
	else {
		int no;
		int par_w, par_h;
		int x = sscanf(ci, "%d %d %d",
			&no, &par_w, &par_h);
		if (x != 3) return -1;
		return feed_set_par(ctr, no, par_w, par_h);
	}
}

// feed filter
int CVideoFeeds::set_feed_filter(struct controller_type* ctr, const char* ci)
{
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) return feed_list_filter (ctr, 0);

	// else 
	unsigned int no;
	cairo_filter_t filter;
	if (sscanf(ci, "%u", &no) != 1) return -1;
	while (isdigit(*ci)) ci++;
	while (isspace(*ci)) ci++;
	if (!strncasecmp (ci, "fast ", 5)) filter=CAIRO_FILTER_FAST;
	else if (!strncasecmp (ci, "good ", 5)) filter=CAIRO_FILTER_GOOD;
	else if (!strncasecmp (ci, "best ", 5)) filter=CAIRO_FILTER_BEST;
	else if (!strncasecmp (ci, "nearest ", 8)) filter=CAIRO_FILTER_NEAREST;
	else if (!strncasecmp (ci, "bilinear ", 9)) filter=CAIRO_FILTER_BILINEAR;
	else if (!strncasecmp (ci, "gaussian ", 9)) filter=CAIRO_FILTER_GAUSSIAN;
	else return -1;
	return feed_set_filter(ctr, no, filter);
}

// feed scale
int CVideoFeeds::set_feed_scale(struct controller_type* ctr, const char* ci)
{
	if (!(*ci)) return feed_list_scales (ctr, 0);
	else {
		int no;
		int scale_1, scale_2;
		int x = sscanf(ci, "%d %d %d",
			&no, &scale_1, &scale_2);
		if (x != 3) return -1;
		return feed_set_scale(ctr, no, scale_1, scale_2);
	}
}

// feed socket
int CVideoFeeds::set_feed_socket(struct controller_type* ctr, const char* ci)
{
	int	no;
	char	name [100];

	memset (name, 0, sizeof (name));
	int x = sscanf (ci, "%d %s", &no, &name [0]);
	if (x != 2) return -1;
	return feed_set_socket (ctr, no, name);
}

// feed geometry
int CVideoFeeds::set_feed_geometry(struct controller_type* ctr, const char* ci)
{
	int	no;
	unsigned int	width;
	unsigned int	height;

	int x = sscanf (ci, "%d %u %u",
		&no, &width, &height);
	if (x != 3) return -1;
	return feed_set_geometry (ctr, no, width, height);
}

// feed idle
int CVideoFeeds::set_feed_idle(struct controller_type* ctr, const char* ci)
{
	int	no;
	int	timeout;
	char	name [256];

	memset (name, 0, sizeof (name));

	// FIXME. name (filename could be longer than 256
	int x = sscanf (ci, "%d %d %s",
		&no, &timeout, &name[0]);
	if (x != 3 && x != 2) return -1;
	return feed_set_idle (ctr, no, timeout, name);
}

// feed pixel [<feed no> <x> <y>]
int CVideoFeeds::get_feed_pixel(struct controller_type* ctr, const char* ci)
{
	u_int8_t red, green, blue, alpha = 0;
	u_int32_t feed_id, x, y = 0;
	if (!ci || !m_pVideoMixer || !m_pVideoMixer->m_pController) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) return -1;
	if (sscanf(ci, "%u %u %u", &feed_id, &x, &y) != 3) return -1;
	int n = GetPixelValue(feed_id, x, y, &red, &green, &blue, &alpha);
	if (n < 0) return -1;
	m_pVideoMixer->m_pController->controller_write_msg(ctr, STATUS
		"%s %d : %hhu %hhu %hhu %hhu\n", n ? "dead" : "feed", n, red, green, blue, alpha);
	return 0;
}

// feed overlay (<id> | <id>..<id> | all | end | <id>..end) [ (<id> | <id>..<id> | all | end | <id>..end) ] ....
int CVideoFeeds::set_feed_overlay(struct controller_type* ctr, const char* ci)
{
	int from, to;
	from = to = 0;
	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) return -1;
		while (*ci) {
		if (!strncmp(ci,"all",3)) {
			from = 0;
			to = MaxFeeds();
			while (*ci) ci++;
		} else if (!strncmp(ci,"end",3)) {
			to = MaxFeeds();
			while (*ci) ci++;
		} else {
			int n=sscanf(ci, "%u", &from);
			if (n != 1) return 1;
			while (isdigit(*ci)) ci++;
			n=sscanf(ci, "..%u", &to);
			if (n != 1) to = from;
			else {
				ci += 2;
			}
			while (isdigit(*ci)) ci++;
			while (isspace(*ci)) ci++;
		}
		if (from > to) return 1;
		while (from <= to) {
			if (m_pVideoMixer) m_pVideoMixer->BasicFeedOverlay(from);
			from++;
		}
	}
	return 0;
}

// feed alpha [<feed id> <alpha>]
int CVideoFeeds::set_feed_alpha(struct controller_type* ctr, const char* ci)
{
	double alpha;
	u_int32_t feed_no;

	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) {
		struct feed_type* pFeed = m_pFeed_list;
		m_pVideoMixer->m_pController->controller_write_msg(ctr, STATUS"Video feed alpha values\n");
		while (pFeed) {
			m_pVideoMixer->m_pController->controller_write_msg(ctr,
				STATUS"feed %u alpha %.4lf\n", pFeed->id, pFeed->alpha);
			pFeed = pFeed->next;
		}
		m_pVideoMixer->m_pController->controller_write_msg(ctr, STATUS"\n");
		return 0;
	} 

	if (sscanf (ci, "%u %lf", &feed_no, &alpha) != 2) return -1;
	return feed_set_alpha (feed_no, alpha);
}

// feed drop <feed id> <n drops>
int CVideoFeeds::set_feed_drop(struct controller_type* ctr, const char* ci)
{
	u_int32_t feed_no, drops;
	int n;

	if (!ci) return -1;
	while (isspace(*ci)) ci++;
	if (!(*ci)) return -1;
	if (sscanf (ci, "%u %u", &feed_no, &drops) != 2) return -1;
	n = DropFeedFrames (feed_no, drops);
	return n;
}

// feed cutout
int CVideoFeeds::set_feed_cutout(struct controller_type* ctr, const char* ci)
{
	int	no;
	unsigned int	cut_start_column, cut_start_row, cut_columns, cut_rows;

	int x = sscanf (ci, "%d %u %u %u %u", &no,
		&cut_start_column, &cut_start_row, &cut_columns, &cut_rows);
	if (x != 5) return -1;
	return feed_set_cutout (ctr, no, cut_start_column, cut_start_row,
		cut_columns, cut_rows);
}

// feed shift
int CVideoFeeds::set_feed_shift(struct controller_type* ctr, const char* ci)
{
	int	no;
	unsigned int	shift_columns, shift_rows;

	int x = sscanf (ci, "%d %u %u", &no,
		&shift_columns, &shift_rows);
	if (x != 3) return -1;
	return feed_set_shift (ctr, no, shift_columns, shift_rows);
}

// feed live
int CVideoFeeds::set_feed_live(struct controller_type* ctr, const char* ci)
{
	int	no;

	int x = sscanf (ci, "%d", &no);
	if (x != 1) return -1;
	return feed_set_live (ctr, no);
}

// feed recorded
int CVideoFeeds::set_feed_recorded(struct controller_type* ctr, const char* ci)
{
	int	no;

	int x = sscanf (ci, "%d", &no);
	if (x != 1) return -1;
	return feed_set_recorded (ctr, no);
}

// Return current feed state
feed_state_enum* CVideoFeeds::PreviousFeedState(int id)
{
	struct feed_type	*feed = NULL;
	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		if (feed->id == id) {
			break;
		}
	}
	if (feed) return &(feed->previous_state);
	else return NULL;
}

// Return current feed state
feed_state_enum* CVideoFeeds::FeedState(int id)
{
	struct feed_type	*feed = NULL;
	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		if (feed->id == id) {
			break;
		}
	}
	if (feed) return &(feed->state);
	else return NULL;
}

// Tells whether a feed is exist and returns width and height
bool CVideoFeeds::FeedGeometry(int id, u_int32_t* width, u_int32_t* height)
{
	struct feed_type *feed = FindFeed(id);
	if (!feed) return false;
	if (width) *width = feed->width;
	if (height) *height = feed->height;
	return true;
}

// Tells whether a feed exists and return details
bool CVideoFeeds::FeedGeometryDetails(int id, u_int32_t* shift_columns, u_int32_t* shift_rows, u_int32_t* cut_start_column, u_int32_t* cut_start_row, u_int32_t* cut_columns, u_int32_t* cut_rows, u_int32_t* par_w, u_int32_t* par_h, u_int32_t* scale_1, u_int32_t* scale_2, double* alpha, cairo_filter_t* filter)
{
	struct feed_type *feed = FindFeed(id);
	if (!feed) return false;
	if (shift_columns) *shift_columns = feed->shift_columns;
	if (shift_rows) *shift_rows = feed->shift_rows;
	if (cut_start_column) *cut_start_column = feed->cut_start_column;
	if (cut_start_row) *cut_start_row = feed->cut_start_row;
	if (cut_columns) *cut_columns = feed->cut_columns;
	if (cut_rows) *cut_rows = feed->cut_rows;
	if (par_w) *par_w = feed->par_w;
	if (par_h) *par_h = feed->par_h;
	if (scale_1) *scale_1 = feed->scale_1;
	if (scale_2) *scale_2 = feed->scale_2;
	if (alpha) *alpha = feed->alpha;
	if (filter) *filter = feed->filter;
	return true;
}


// Tells whether a feed exists
bool CVideoFeeds::FeedExist(int id)
{
	return (FindFeed(id) ? true : false);
}

/* Find a feed by its id number. */
struct feed_type* CVideoFeeds::FindFeed (int id)
{
	struct feed_type	*feed;
	//if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return NULL;

	for (feed = m_pFeed_list; feed != NULL;
		feed = feed->next) {
		if (feed->id == id) {
			return feed;
		}
	}
	return NULL;
}

/* Add a new feed to the system. */
int CVideoFeeds::feed_add_new (struct controller_type *ctr, int id, char *name)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	/* First check if the controller no is unique. */
	struct feed_type	*feed;

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		if (feed->id == id) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Feed ID %d already used\n", id);
			return -1;
		}
	}

	/* Setup a new feed structure. */
	trim_string (name);
	if (!(feed = (feed_type*) calloc (1, sizeof (*feed)))) {
		fprintf(stderr, "Failed to allocate memory for feed %d named %s\n", id, name);
		return -1;
	}
	feed->state		= SETUP;
	feed->previous_state	= SETUP;
	feed->id		= id;
	feed->control_socket	= -1;
	feed->alpha		= 1.0;
	feed->max_fifo		= MAX_FIFO_DEPTH;	/* Default value. */
	feed->feed_name		= strdup (name);
	feed->seq_no		= 1;
	feed->m_video_type	= VIDEO_BGRA;

	list_add_tail (m_pFeed_list, feed);
	m_feed_count++;

	return 0;
}

/* List scale of all feeds in the system*/
int CVideoFeeds::feed_list_scales (struct controller_type *ctr, int verbose)
{
	struct feed_type	*feed;

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Feed ID %d  Scale: %2d:%-2d\n",
			feed->id, feed->scale_1, feed->scale_2);
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr,"MSG:\n");
	return 0;
}

/* List par of all feeds in the system*/
int CVideoFeeds::feed_list_par (struct controller_type *ctr, int verbose)
{
	struct feed_type	*feed;

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Feed ID %d  Pixel Aspect Ratio: %2d:%-2d\n",
			feed->id, feed->par_w, feed->par_h);
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr,"MSG:\n");
	return 0;
}


/* List filter setting for all feeds */
int CVideoFeeds::feed_list_filter (struct controller_type *ctr, int verbose)
{
	struct feed_type	*feed;

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Feed ID %d  filter %s\n", feed->id,
			CairoFilter2String(feed->filter));
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr,"MSG:\n");
	return 0;
}
/* List all feeds in the system. */
int CVideoFeeds::feed_list_feeds (struct controller_type *ctr, int verbose)
{
	struct feed_type	*feed;
	if (!m_pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for feed_list_feeds\n");
		return -1;
	}

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		if (verbose == 0) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"Feed ID %d  Name: %s\n", feed->id,
				feed->feed_name ? feed->feed_name : "");
		} else {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"Feed ID %d\n",       feed->id);
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				STATUS"  Name:       %s\n", feed->feed_name ?
					feed->feed_name : "");
			switch (feed->state) {
				case SETUP:	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      SETUP\n"); break;
				case PENDING:      m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      PENDING\n"); break;
				case DISCONNECTED: m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      DISCONNECTED\n"); break;
				case RUNNING:      m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      RUNNING\n"); break;
				case STALLED:      m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      STALLED\n"); break;
				case READY:	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      READY\n"); break;
				case NO_FEED:      m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      NOFEED\n"); break;
				case UNDEFINED:    m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  State:      UNDEFINED\n"); break;
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Socket:     %s\n", feed->control_socket_name ? feed->control_socket_name : "N/A");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Fullscreen: %s\n", (unsigned) feed->width == m_pVideoMixer->GetSystemWidth() && (unsigned) feed->height == m_pVideoMixer->GetSystemHeight() ? "Yes" : "No");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Geometry:   %d %d\n", feed->width, feed->height);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Cutout:     %d %d %d %d\n", feed->cut_start_column, feed->cut_start_row,
										       feed->cut_columns, feed->cut_rows);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Shift:      %d %d\n", feed->shift_columns, feed->shift_rows);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Open:       %s\n", feed->control_socket >= 0 ? "Yes" : "No");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Is Live:    %s\n", feed->is_live ? "Yes" : "No");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Oneshot:    %s\n", feed->oneshot ? "Yes" : "No");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Idle Time:  %d\n", feed->dead_img_timeout);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Image file: %s\n", feed->dead_img_name ? feed->dead_img_name : "");
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Frames:     %d\n", feed->good_frames);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Dropped:    %d\n", feed->dropped_frames);
			m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"  Missed:     %d\n", feed->missed_frames);
		}
	}
	m_pVideoMixer->m_pController->controller_write_msg (ctr, STATUS"\n");

	return 0;
}

/* Set the name of a feed. */
int CVideoFeeds::feed_set_name (struct controller_type *ctr, int id, char *name)
{
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_name\n", id);
		return -1;
	}

	if (feed->feed_name) {
		free (feed->feed_name);
	}
	trim_string (name);
	feed->feed_name = strdup (name);
	
	return 0;
}


/* Set the input socket for a feed. */
int CVideoFeeds::feed_set_socket (struct controller_type *ctr, int id, char *name)
{
	struct feed_type	*feed = FindFeed (id);

	if (!m_pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for feed_set_socket\n");
		return -1;
	}

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_socket\n", id);
		return -1;
	}

	if (feed->control_socket_name) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Socket already set.\n");
		return -1;
	}

	trim_string (name);
	if (*(name) != '/') {
		fprintf(stderr, "Warning: Control socket name <%s> for feed %d does not start with a '/'\n",name, id);
		return -1;
	}
	feed->control_socket_name = strdup (name);

	/* This locks the geometry. */
	if (feed->width == 0 && feed->height == 0) {
		feed->width	    = m_pVideoMixer->GetSystemWidth();
		feed->height	   = m_pVideoMixer->GetSystemHeight();
		feed->cut_start_column = 0;
		feed->cut_start_row    = 0;
		feed->cut_columns      = m_pVideoMixer->GetSystemWidth();
		feed->cut_rows	 = m_pVideoMixer->GetSystemHeight();
		feed->shift_columns    = 0;
		feed->shift_rows       = 0;
		feed->par_w	    = 1;
		feed->par_h	    = 1;
		feed->scale_1	  = 1;
		feed->scale_2	  = 1;
		feed->filter	       = CAIRO_FILTER_FAST;
	}
	feed->previous_state = feed->state;
	feed->state = PENDING;
	feed->start_time.tv_sec = 0;
	feed->start_time.tv_usec = 0;

	return 0;
}


/* Set the idle timeout value and the timeout image. */
int CVideoFeeds::feed_set_idle (struct controller_type *ctr, int id, int timeout, char *name)
{
	struct feed_type	*feed = FindFeed (id);
	uint8_t			*buffer;
	int			fd;

	if (!m_pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL in feed_set_idle\n");
		return -1;
	}

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid feed no: %d in feed_set_idle\n", id);
		return -1;
	}
	if (timeout < 1) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid timeout value: %d\n", timeout);
		return -1;
	}
	if (!m_pVideoMixer->m_pixel_bytes) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid pixel_bytes value: %u\n", m_pVideoMixer->m_pixel_bytes);
		return -1;
	}

	/* This locks the geometry. */
	if (feed->width == 0 && feed->height == 0) {
		feed->width	    = m_pVideoMixer->GetSystemWidth();
		feed->height	   = m_pVideoMixer->GetSystemHeight();
		feed->cut_start_column = 0;
		feed->cut_start_row    = 0;
		feed->cut_columns      = m_pVideoMixer->GetSystemWidth();
		feed->cut_rows	 = m_pVideoMixer->GetSystemHeight();
		feed->shift_columns    = 0;
		feed->shift_rows       = 0;
		feed->par_w	    = 1;
		feed->par_h	    = 1;
		feed->scale_1	  = 1;
		feed->scale_2	  = 1;
		feed->filter	       = CAIRO_FILTER_FAST;
	}

	if (feed->width == 0 || feed->height == 0) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid feed geometry: %ux%u\n", feed->width, feed->height);
		return -1;
	}


	/* Try to open the new file. */
	trim_string (name);
	if (*name) {
		//fd = open (name, O_RDONLY);
		fd = m_pVideoMixer->OpenFile(name, O_RDONLY);
		if (fd < 0) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"MSG: Unable to open dead image file: \"%s\"\n", name);
			return -1;
		}

		/* Load the file into a empty buffer. */
		buffer = (u_int8_t*) calloc (1, m_pVideoMixer->m_pixel_bytes * feed->width *
			feed->height);

		// Check the buffer for NULL
		if (!buffer) {
			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"Failed to get buffer for idle image\n");
			return 1;
		}
		memset(buffer, 1, m_pVideoMixer->m_pixel_bytes * feed->width * feed->height);
		if (name) {
			if (read (fd, buffer, m_pVideoMixer->m_block_size) < (signed)
				(m_pVideoMixer->m_pixel_bytes * feed->width * feed->height)) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"MSG: Unable to read %u bytes from file \"%s\"\n",
					m_pVideoMixer->m_pixel_bytes * feed->width * feed->height,
					name);
				free (buffer);
				return -1;
			}
		}
		close(fd);

		/* Store the new name. */
		if (feed->dead_img_name) {
			free (feed->dead_img_name);
		}
		feed->dead_img_name = strdup (name ? name : "");
	
		/* Update the buffer pointer. */
		if (feed->dead_img_buffer) {
			free (feed->dead_img_buffer);
		}
		feed->dead_img_buffer = buffer;
	}
	feed->dead_img_timeout = timeout;

	return 0;
}


/* Set the geometry of the feed. */
int CVideoFeeds::feed_set_geometry (struct controller_type *ctr, int id, unsigned int width, unsigned int height)
{
	struct feed_type	*feed = FindFeed (id);
	if (!m_pVideoMixer) {
		fprintf(stderr, "pVideoMixer was NULL for feed_set_geometry<n");
		return -1;
	}

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_geometry\n", id);
		return -1;
	}
	if (width < 1 || width > m_pVideoMixer->GetSystemWidth()) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid width value: %d. Width must be between 1 and system width set to %u\n",
			width, m_pVideoMixer->GetSystemWidth());
		return -1;
	}
	if (height < 1 || height > m_pVideoMixer->GetSystemHeight()) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid height value: %d. Height must be between 1 and system height set to %u\n",
			height, m_pVideoMixer->GetSystemHeight());
		return -1;
	}
	if (feed->width > 0 || feed->height > 0) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Can't change geometry. It's already set.\n");
		return -1;
	}

	// fprintf(stderr, "VideoScaler (%dx%d) for feed %d named %s\n",
	//	width, height, feed->id, feed->feed_name);

	feed->width	    = width;
	feed->height	   = height;
	feed->cut_start_column = 0;
	feed->cut_start_row    = 0;
	feed->cut_columns      = width;
	feed->cut_rows	 = height;
	feed->shift_columns    = 0;
	feed->shift_rows       = 0;
	feed->par_w	    = 1;
	feed->par_h	    = 1;
	feed->scale_1	  = 1;
	feed->scale_2	  = 1;
	feed->filter	       = CAIRO_FILTER_FAST;

	return 0;
}

/* Set the feed scale */
int CVideoFeeds::feed_set_scale (struct controller_type *ctr, int id, int scale_1, int scale_2)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid feed no: %d in feed_set_scale\n", id);
		return -1;
	}
	if (scale_1 == scale_2) scale_1 = scale_2 = 1;
	do {
		if ((scale_1 & 1) || (scale_2 & 1)) break;
		scale_1 = scale_1 >> 1;
		scale_2 = scale_2 >> 1;
	} while (1);
	if (m_pVideoMixer->m_pController->m_verbose)
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"Feed %d scaled %d:%d\n",
			feed->id, scale_1, scale_2);
	if (scale_1 < 1 || scale_2 < 1) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"MSG: Invalid scale value %d %d.\n", scale_1, scale_2);
		return -1;
	}
	feed->scale_1 = scale_1;
	feed->scale_2 = scale_2;
	return 0;
}

// Set the feed par
int CVideoFeeds::feed_set_par (struct controller_type *ctr, int id, int par_w, int par_h)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_par\n", id);
		return -1;
	}
	if (par_w == par_h || (par_w < 1) || (par_h < 1)) par_w = par_h = 1;
	do {
		if ((par_w & 1) || (par_h & 1)) break;
		par_w = par_w >> 1;
		par_h = par_h >> 1;
	} while (1);
	//fprintf(stderr, "Feed %d par %d:%d\n", feed->id, par_w, par_h);
	feed->par_w = par_w;
	feed->par_h = par_h;
	return 0;
}

// Set the feed filter
int CVideoFeeds::feed_set_filter (struct controller_type *ctr, int id, cairo_filter_t filter)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_filter\n", id);
		return -1;
	}
	feed->filter = filter;
	return 0;
}

int CVideoFeeds::feed_set_alpha(u_int32_t feed_id, double alpha) {

	struct feed_type	*feed = FindFeed (feed_id);

	if (!feed) return -1;
	if (alpha < 0.0) alpha = 0.0;
	else if (alpha > 1.0) alpha = 1.0;
	feed->alpha = alpha;
	return 0;
}


/* Set the cutout area of the feed. */
int CVideoFeeds::feed_set_cutout (struct controller_type *ctr, int id, unsigned int cut_start_column, unsigned int cut_start_row, unsigned int cut_columns, unsigned int cut_rows)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_cutout\n", id);
		return -1;
	}
	if (cut_start_column >= feed->width) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid start column value: 0 < %d < %d\n",
					    cut_start_column, feed->width);
		return -1;
	}
	if (cut_start_row >= feed->height) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid start row value: 0 < %d < %d\n",
					   cut_start_row, feed->height);
		return -1;
	}
	if (cut_columns > feed->width) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid cut width: 0 < %d < %d\n",
					   cut_columns, feed->width);
		return -1;
	}
	if (cut_rows > feed->height) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid cut height: 0 < %d < %d\n",
					   cut_rows, feed->height);
		return -1;
	}

	feed->cut_start_column = cut_start_column;
	feed->cut_start_row    = cut_start_row;
	feed->cut_columns      = cut_columns;
	feed->cut_rows	 = cut_rows;

	return 0;
}


/* Set the shifted position of the feed. */
int CVideoFeeds::feed_set_shift (struct controller_type *ctr, int id, unsigned int shift_columns, unsigned int shift_rows)
{
	struct feed_type	*feed = FindFeed (id);

	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) {
		fprintf(stderr, "pVideoMixer was NULL in feed_set_shift\n");
		return 1;
	}

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_shift\n", id);
		return -1;
	}
	if (shift_columns >= m_pVideoMixer->GetSystemWidth()) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid shift column value: 0 < %d < %d\n",
					   shift_columns, m_pVideoMixer->GetSystemWidth());
		return -1;
	}
	if (shift_rows >= m_pVideoMixer->GetSystemHeight()) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid shift row value: 0 < %d < %d\n",
					   shift_rows, m_pVideoMixer->GetSystemHeight());
		return -1;
	}

	feed->shift_columns = shift_columns;
	feed->shift_rows    = shift_rows;

	return 0;
}


/* Set the feed to be a live feed. */
int CVideoFeeds::feed_set_live (struct controller_type *ctr, int id)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_live\n", id);
		return -1;
	}

	feed->is_live = 1;

	return 0;
}


/* Set the feed to be a recorded feed. */
int CVideoFeeds::feed_set_recorded (struct controller_type *ctr, int id)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed = FindFeed (id);

	if (!feed) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Invalid feed no: %d in feed_set_recorded\n", id);
		return -1;
	}

	feed->is_live = 0;

	return 0;
}


/* Dump buffer info for all the feeds. */
int CVideoFeeds::feed_dump_buffers (struct controller_type *ctr)
{
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
	struct feed_type	*feed;

	for (feed = m_pFeed_list; feed != NULL; feed = feed->next) {
		struct area_list_type	*area;

		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"DBG: Feed %d %s\n", feed->id, feed->feed_name);
		for (area = feed->area_list; area != NULL; area = area->next) {
			struct buffer_type	*buffer;

			m_pVideoMixer->m_pController->controller_write_msg (ctr,
				"DBG:   Area ID: %d  Blocks: ", area->area_id);
			for (buffer = area->buffers; buffer != NULL; buffer = buffer->next) {
				m_pVideoMixer->m_pController->controller_write_msg (ctr,
					"%lu ", buffer->offset / buffer->bsize);
			}
			m_pVideoMixer->m_pController->controller_write_msg (ctr, "\n");
		}
	}
	return 0;
}

/* Select the currently streamed feed. */
int CVideoFeeds::output_select_feed (struct controller_type *ctr, int no)
{
	struct feed_type		*feed;
	if (!m_pVideoMixer || !m_pVideoMixer->m_pController) {
		fprintf(stderr, "pVideoMixer was NULL in output_select_feed\n");
		return 1;
	}

	feed = FindFeed (no);
	if (feed == NULL) {
		/* Unknown feed. */
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Unknown feed: %d\n", no);
		return -1;
	}
	if (feed->width  != m_pVideoMixer->GetSystemWidth() ||
	    feed->height != m_pVideoMixer->GetSystemHeight()) {
		m_pVideoMixer->m_pController->controller_write_msg (ctr, "MSG: Not a full-screen feed\n");
		return -1;
	}
	m_pVideoMixer->m_output_stack [0] = no;
	m_pVideoMixer->m_output_stack [1] = -1;

	/* Let everyone know that a new feed have been selected. */
	m_pVideoMixer->m_pController->controller_write_msg (NULL, "Selected %d %s\n", no, feed->feed_name);

	return 0;
}


/* Populate rfds and wrfd for the feed subsystem. */
int CVideoFeeds::SetFds (int maxfd, fd_set *rfds, fd_set *wfds)
{
	if (!m_pVideoMixer) {
		fprintf(stderr, "m_pVideoMixer was NULL in feed_set_fds\n");
		return -1;
	}
	if (!m_pVideoMixer->m_pController) {
		fprintf(stderr, "pController was NULL in feed_set_fds\n");
		return -1;
	}
	if (!m_pVideoMixer->m_pVideoFeed) {
		fprintf(stderr, "pVideoFeed was NULL in feed_set_fds\n");
		return -1;
	}
	struct feed_type* feed = GetFeedList();
	CController* pController = m_pVideoMixer->m_pController;


	

	for ( ; feed != NULL; feed = feed->next) {
		if (feed->control_socket < 0 && feed->control_socket_name != NULL) {
			/* No socket yet. Try to create it. */
			feed->control_socket = socket (AF_UNIX, SOCK_STREAM, 0);
			if (feed->control_socket >= 0) {
				/* Bind it to the socket. */
				struct sockaddr_un	addr;

				memset (&addr, 0, sizeof (addr));
				addr.sun_family = AF_UNIX;
				strncpy (addr.sun_path, feed->control_socket_name, sizeof (addr.sun_path) - 1);
				if (connect (feed->control_socket, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
					if (feed->connect_retries == 0) {
						pController->controller_write_msg (NULL, "Feed %d \"%s\" unable to connect: %s\n",
								      feed->id, feed->feed_name ? feed->feed_name : "",
								      strerror (errno));
					}
					close (feed->control_socket);
					feed->control_socket = -1;
					feed->connect_retries++;
				} else {
					pController->controller_write_msg (NULL, "Feed %d \"%s\" connected\n",
							      feed->id, feed->feed_name ? feed->feed_name : "");
					feed->connect_retries = 0;
#ifdef HAVE_MACOSX
					int yes = 1;
					if (setsockopt (feed->control_socket, SOL_SOCKET, SO_NOSIGPIPE, &yes, sizeof (yes)) < 0) {
						perror("Warning. Failed to set SO_NOSIGPIPE for feed control socket");
					}
#endif
				}
			}
		}

		if (feed->control_socket >= 0 && (feed->is_live || feed->fifo_depth < feed->max_fifo)) {
			/* Add its descriptor to the bitmap. */
			FD_SET (feed->control_socket, rfds);
			if (feed->control_socket > maxfd) {
				maxfd = feed->control_socket;
			}
		}
	}

	return maxfd;
}
// Check for incomming data on a feed.
void CVideoFeeds::CheckRead (fd_set *rfds, u_int32_t block_size)
{
	struct feed_type		*feed, *next_feed;
	int				x;
	struct shm_commandbuf_type	cb;

	if (!m_pVideoMixer) {
		fprintf(stderr, "m_pVideoMixer was NULL in feed_set_fds\n");
		return;
	}
	if (!m_pVideoMixer->m_pController) {
		fprintf(stderr, "pController was NULL in feed_set_fds\n");
		return;
	}
	feed = GetFeedList();
	CController* pController = m_pVideoMixer->m_pController;
	for (next_feed = feed ? feed->next : NULL; feed != NULL;
	     feed = next_feed, next_feed = feed ? feed->next : NULL) {
//fprintf(stderr, "Checking feed %d\n", feed->id);
		if (feed->control_socket < 0) continue;			/* Not open. */
		if (!FD_ISSET (feed->control_socket, rfds)) continue;	/* Not this one. */

		/* This one have pending data. */
		x = read (feed->control_socket, &cb, sizeof (cb));
		if (x <= 0) {
			pController->controller_write_msg (NULL, "Feed %d \"%s\" disconnected\n",
					      feed->id, feed->feed_name ? feed->feed_name : "");
			close (feed->control_socket);
			feed->control_socket = -1;
			feed->previous_state = feed->state;
			feed->state = DISCONNECTED;
//fprintf(stderr, "Feed %d set to DISCONNECTED\n", feed->id);
			ReleaseBuffers (feed);

			if (feed->oneshot) {
				/* Delete this feed. */
				//FIXME:
			}
		} else {
			/* Got something of the socket. Mark the feed as running. */
			if (feed->state != RUNNING) {
	
				// We changed to RUNNING and will restart the clock
				gettimeofday(&(feed->start_time), NULL);
				feed->previous_state = feed->state;
				feed->state = RUNNING;
fprintf(stderr, "Feed %d set to RUNNING\n", feed->id);
			}

			/* Interpret the received message. */
			switch (cb.type) {
				case 1:	{	/* COMMAND_NEW_SHM_AREA. */
					/* Create a new area for this feed. */
					struct area_list_type	*area = (area_list_type*)
						calloc (1, sizeof (*area));

					area->area_id = cb.area_id;
					area->area_size = cb.payload.new_shm_area.size;
					area->buffers = NULL;		/* No buffers used yet. */
					area->feed = feed;
					area->area_handle_name = (char*)
						calloc (cb.payload.new_shm_area.path_size + 1, 1);
#ifndef WIN32
					ssize_t n =
#else
					int n =
#endif
						read (feed->control_socket, area->area_handle_name,
						cb.payload.new_shm_area.path_size);
					if (n < 1) {
						if (n < 0) {
							perror("Read error from control socket.\n");
							fprintf(stderr, "Socket handle name %s\n",
								area->area_handle_name);
						} else fprintf(stderr, "Read zero bytes from socket %s\n",
								area->area_handle_name);
					}

					/* Map the area into memory. */
#ifndef WIN32
					area->area_handle = shm_open (area->area_handle_name,
						O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP);
#else
#pragma message("We need a shm_open() replacement for Windows")
#endif
					if (area->area_handle < 0) {
						fprintf (stderr, "Unable to open shared memory buffer "
							"handle: \"%s\"\n", area->area_handle_name);
						dump_cb (block_size, "Her", "Der", &cb, sizeof (cb));
						exit (1);
					}

#ifndef WIN32
					area->buffer_base = (uint8_t*) mmap (NULL, area->area_size, PROT_READ, MAP_SHARED, area->area_handle, 0);
#else
#pragma message ("We need a mmap() replacement for Windows")
					area->buffer_base = (typeof(area->buffer_base))(MAP_FAILED);
#endif
					if (area->buffer_base == MAP_FAILED) {
						perror ("Unable to map area into memory.");
						exit (1);
					}

					fprintf (stderr, "New SHM Area. Feed: %s  ID: %d  size: %lu  pathname: %s\n",
						 feed->feed_name, area->area_id, area->area_size, area->area_handle_name);

					list_add_head (feed->area_list, area);
					break;
				}

				case 2:	{	/* COMMAND_CLOSE_SHM_AREA. */
					struct area_list_type	*area = FindArea (feed, cb.area_id);
					if (area != NULL) {
						if (area->buffers != NULL) {
							/* This area still ahve outstanding buffers. */
							fprintf (stderr, "Close SHM Area. Feed: %s  ID: %d - Still active buffers\n",
								 feed->feed_name, cb.area_id);
						} else {
							/* Release the area. */
							fprintf (stderr, "Close SHM Area. Feed: %s  ID: %d - OK\n",
							 	 feed->feed_name, cb.area_id);

							list_del_element (feed->area_list, area);
							if (area->buffer_base != NULL) {
								/* Unmap the memory segment. */
#ifndef WIN32
								munmap (area->buffer_base, area->area_size);
#else
#pragma message("We need a munmap() replacement for Windows")
#endif
								close (area->area_handle);
							}
							free (area->area_handle_name);
							free (area);
						}
					} else {
						fprintf (stderr, "Close SHM Area. Feed: %s  ID: %d - Unknown area\n",
							 feed->feed_name, cb.area_id);
					}
					break;
				}
		
				case 3: {	/* COMMAND_NEW_BUFFER. */
					struct buffer_type	*buffer;

					/* Add the buffer to the input fifo - or loose if. */
/*					if (feed->id == 9) {
					fprintf (stderr, "New Buffer. Feed: %s  ID: %d  block: %4lu  offset: %9lu  size: %9lu\n",
						 feed->feed_name,
						 cb.area_id,
						 cb.payload.buffer.offset/cb.payload.buffer.bsize,
						 cb.payload.buffer.offset,
						 cb.payload.buffer.bsize);
					}
*/

					/* Get a buffer handle for it. */
					buffer = UseBuffer (feed, cb.area_id, cb.payload.buffer.offset, cb.payload.buffer.bsize);

					/* Figure out how to add it to the fifo. */
					if (feed->fifo_depth >= feed->max_fifo) {
						/* The fifo is full. Discard the head buffer (head drop). */
						AckBufferPtr (block_size, feed->frame_fifo [0].buffer);

						memmove (&feed->frame_fifo [0].buffer,
							 &feed->frame_fifo [1].buffer,
							 sizeof (feed->frame_fifo [0]) * (MAX_FIFO_DEPTH - 1));
						feed->frame_fifo [MAX_FIFO_DEPTH - 1].buffer = NULL;

						feed->frame_fifo [feed->max_fifo - 1].buffer = buffer;
						feed->dropped_frames++;
					} else if (feed->fifo_depth == 0) {
						/* The fifo is empty. Duplicate this frame to half the slots. */
						int	x;

						feed->frame_fifo [0].buffer = buffer;
						feed->fifo_depth++;
						for (x = 1; x < feed->max_fifo/2; x++) {
							UseBufferPtr (buffer);
							feed->frame_fifo [x].buffer = buffer;
							feed->fifo_depth++;
						}
						feed->good_frames++;
					} else {
						/* Just add it to the end of the fifo. */
						feed->frame_fifo [feed->fifo_depth++].buffer = buffer;
						feed->good_frames++;
					}

					break;
				}

				case 4:	{	/* COMMAND_ACK_BUFFER. */
					/* Not expected from a feed socket. */
					fprintf (stderr, "ACK Buffer. Feed: %s  ID: %d  block: %4lu  offset: %9lu - DUH\n",
						 feed->feed_name,
						 cb.area_id,
						 cb.payload.ack_buffer.offset/block_size,
						 cb.payload.ack_buffer.offset);
					break;
				}

				default:
					fprintf (stderr, "Unknown packet. Feed: %s  Code: %d\n",
						 feed->feed_name, cb.type);
					break;
			}
		}
	}
}

int CVideoFeeds::DropFeedFrames(u_int32_t feed_id, u_int32_t drops) {
	feed_type* pFeed = FindFeed(feed_id);
	if (!pFeed || pFeed->fifo_depth < (signed) drops || !m_pVideoMixer || !m_pVideoMixer->m_block_size) return -1;
	// We now know that drops+1 <= fifo_depth, so it is safe to drop drops frames.
	// Signal the buffers are free.
	for (unsigned i = 0; i < drops; i++) AckBufferPtr (m_pVideoMixer->m_block_size, pFeed->frame_fifo [0].buffer);
	// And update the fifo
	memmove (&pFeed->frame_fifo [0].buffer,
		&pFeed->frame_fifo [drops].buffer,
		sizeof (pFeed->frame_fifo [0])*(MAX_FIFO_DEPTH-drops));
	pFeed->frame_fifo [MAX_FIFO_DEPTH - 1].buffer = NULL;
	pFeed->fifo_depth -= drops;
	pFeed->dropped_frames++;
	return 0;
}

// Update feeds based on the output frame rate.
void CVideoFeeds::Timertick (u_int32_t block_size, struct feed_type* feed_list)
{
	struct feed_type	*feed;

	for (feed = feed_list; feed != NULL; feed = feed->next) {
		if (feed->is_live == 0 && feed->dequeued == 0) {
			/* This feed is not being dequeued and it is not live. Don't advance the fifo. */
			continue;
		}
		feed->dequeued = 0;	/* Clear the dequeue flag again. */

		/* Shift the contents of the fifo buffers to advance time. */
		if (feed->fifo_depth > 1) {
			/* The fifo contains multiple frames. Discard the head buffer. */
			AckBufferPtr (block_size, feed->frame_fifo [0].buffer);

			memmove (&feed->frame_fifo [0].buffer,
				 &feed->frame_fifo [1].buffer,
				 sizeof (feed->frame_fifo [0]) * (MAX_FIFO_DEPTH - 1));
			feed->frame_fifo [MAX_FIFO_DEPTH - 1].buffer = NULL;
			feed->fifo_depth--;
			feed->missed_frames = 0;
			feed->seq_no++;
		} else if (feed->fifo_depth == 1) {
			/* The fifo only holds a single frame. Keep it there and do idle timer math. */
			feed->missed_frames++;
			if (feed->missed_frames > feed->dead_img_timeout && feed->dead_img_timeout > 0) {
				/* The feed is dead. Flush the last image. */
				AckBufferPtr (block_size, feed->frame_fifo [0].buffer);
				feed->frame_fifo [0].buffer = NULL;
				feed->previous_state = feed->state;
				feed->state = STALLED;
				feed->fifo_depth = 0;
				feed->seq_no++;
			}
		} else {
			/* The feed is dead. */
			feed->missed_frames++;
			if (feed->state == RUNNING) feed->state = STALLED;
		}
	}
}

/********************************************************************************
*										*
*	Stuff for handling gstreamer shmsink/shmsrc data.			*
*										*
********************************************************************************/

//struct area_list_type;

/* Find a area_id element for a feed. */
struct area_list_type* CVideoFeeds::FindArea (struct feed_type *feed, int area_id)
{
	struct area_list_type	*area;
       
	for (area = feed->area_list; area != NULL; area = area->next) {
		if (area->area_id != area_id) continue;

		return area;	/* Found it. */
	}
	return NULL;		/* Not found. */
}


/* Locate a buffer control struct. */
struct buffer_type *FindBufferByArea (struct area_list_type *area, unsigned long offset)
{
	struct buffer_type 	*buffer;

	for (buffer = area->buffers; buffer != NULL; buffer = buffer->next) {
		if (buffer->offset != offset) continue;

		return buffer;	/* Found it. */
	}
	return NULL;		/* Not found. */
}



/** Transmit an ACK message to a feed. */
static void transmit_ack (u_int32_t block_size, struct feed_type *feed, int area_id, unsigned long offset)
{
	struct shm_commandbuf_type	cb;

	memset (&cb, 0, sizeof (cb));
	cb.type = 4;
	cb.area_id = area_id;
	cb.payload.ack_buffer.offset = offset;
	if (send (feed->control_socket,
#ifdef WIN32
		(const char*)
#endif
		&cb, sizeof (cb),
#ifdef MSG_NOSIGNAL
		MSG_NOSIGNAL
#else
		0
#endif
		) > 0) {
		if (feed->id == 9) {
//		dump_cb (block_size, "Proxy", feed->control_socket_name, &cb, sizeof (cb));
		}
	} else {
		/* Feed disappeared. */
		perror ("send to feed");
		//FIXME: Cleanup.
	}
}



/* Bump up the use count on this buffer reference. */
struct buffer_type* CVideoFeeds::UseBuffer (struct feed_type *feed, int area_id, unsigned long offset, unsigned long bsize)
{
	struct area_list_type	*area = FindArea (feed, area_id);
	struct buffer_type	*buffer;

	if (area == NULL) {
		fprintf (stderr, "Area %d Unknown\n", area_id);
		return NULL;	/* Onknown area. */
	}
       
	buffer = FindBufferByArea (area, offset);
	if (buffer == NULL) {
		/* Not seen before. Create a new descriptor entry. */
		buffer = (buffer_type*) calloc (1, sizeof (*buffer));
		buffer->area_list = area;
		buffer->offset    = offset;
		buffer->bsize     = bsize;
		buffer->use       = 1;

		list_add_head (area->buffers, buffer);
	} else {
		/* Already pending. Just bump the use counter. */
		buffer->use++;
	}

	return buffer;
}


/* Increment the use count on this buffer pointer. */
void CVideoFeeds::UseBufferPtr (struct buffer_type *buffer)
{
	if (buffer == NULL) return;	/* Nothing to be done. */

	buffer->use++;
}


/* Decrement the use count on this buffer pointer. */
void CVideoFeeds::AckBufferPtr (u_int32_t block_size, struct buffer_type *buffer)
{
	if (buffer == NULL) return;	/* Nothing to be done. */

	buffer->use--;
	if (buffer->use == 0) {
		/* No longer in use. Free it back on the master. */
		struct area_list_type	*area = buffer->area_list;
		struct feed_type	*feed = area->feed;

		transmit_ack (block_size, feed, area->area_id, buffer->offset);

		/* Remove from memory list. */
		list_del_element (area->buffers, buffer);
		free (buffer);
	}
}


/* Release all buffers assigned to a feed. */
void CVideoFeeds::ReleaseBuffers (struct feed_type *feed)
{
	struct area_list_type	*area, *next_area;

	for (area = feed->area_list, next_area = area ? area->next : NULL;
	     area != NULL;
	     area = next_area, next_area = area ? area->next : NULL) {
		struct buffer_type	*buffer, *next_buffer;

		/* Delete all buffers associated with this area. */
		for (buffer = area->buffers, next_buffer = buffer ? buffer->next : NULL;
		     buffer != NULL;
		     buffer = next_buffer, next_buffer = buffer ? buffer->next : NULL) {
			list_del_element (area->buffers, buffer);
			free (buffer);
		}

		list_del_element (feed->area_list, area);
		if (area->buffer_base != NULL) {
			/* Unmap the memory segment. */
//fprintf(stderr, "Feed Releas Buffer unmap %s PMM\n", area->area_handle_name);
#ifndef WIN32
			munmap (area->buffer_base, area->area_size);
			if (shm_unlink(area->area_handle_name)) {
#else
#pragma message("We need a munmap() and shm_unlink replacement for Windows")
			if (1) {
#endif
				perror("shm_unlink error : ");
			}
			close (area->area_handle);
		}
		free (area->area_handle_name);
		free (area);
	}

	/* Clear the fifo structs. */
	memset (&feed->frame_fifo, 0, sizeof (feed->frame_fifo));
	feed->fifo_depth = 0;
}

u_int32_t CVideoFeeds::FeedSeqNo(int id) {
	feed_type* pFeed = FindFeed(id);
	return pFeed ? pFeed->seq_no : 0;
}

struct timeval* CVideoFeeds::GetFeedStartTime(int id) {
	feed_type* pFeed = FindFeed(id);
	return pFeed ? &(pFeed->start_time) : NULL;
}

// Get Pixel values for a given feed.
// Return values:
//	-1 : Feed not found
//	-2 : x and/or y outside geometry
//	-3 : Feed found but no frame
//	 0 : Feed found and values from an input frame
//	 1 : Feed found but values from the dead_image
int CVideoFeeds::GetPixelValue(u_int32_t feed_id, u_int32_t x, u_int32_t y, u_int8_t* pred, u_int8_t* pgreen, u_int8_t* pblue, u_int8_t* palpha)
{
	int val = 0;
	u_int8_t* src_buf = NULL;
	feed_type* pFeed = FindFeed(feed_id);
	if (!pFeed) return -1;
	if (x >= pFeed->width || y >= pFeed->height) return -2;
	if (pFeed->fifo_depth <= 0) {
		val = 1;
		// Nothing in the fifo.
		src_buf = pFeed->dead_img_buffer;
	} else {
		// Pick the first one from the fifo.
		src_buf = pFeed->frame_fifo [0].buffer->area_list->buffer_base +
			pFeed->frame_fifo [0].buffer->offset;
	}
	if (!src_buf) return -3;
	src_buf += ((y*pFeed->width + x)*4);
	if (pred)   { *pred   = src_buf[0]; }
	if (pgreen) { *pgreen = src_buf[1]; }
	if (pblue)  { *pblue  = src_buf[2]; }
	if (palpha) { *palpha = src_buf[3]; }
	return val;
}

u_int8_t* CVideoFeeds::GetFeedFrame(int id, u_int32_t* pwidth, u_int32_t *pheight)
{
	feed_type* pFeed = FindFeed(id);
	u_int8_t* src_buf = NULL;
	if (pFeed) {
		if (pFeed->fifo_depth <= 0) {
			// Nothing in the fifo. Show the idle image.
			src_buf = pFeed->dead_img_buffer;
		} else {
			// Pick the first one from the fifo.
			src_buf = pFeed->frame_fifo [0].buffer->area_list->buffer_base +
				pFeed->frame_fifo [0].buffer->offset;
		}
		if (pwidth) *pwidth = pFeed->width;
		if (pheight) *pheight = pFeed->height;
		pFeed->dequeued = 1;	    // Signal the feed has been used.
	}
	return src_buf;
}
