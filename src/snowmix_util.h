/*
 * (c) 2013 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __SNOWMIX_UTIL_H__
#define __SNOWMIX_UTIL_H__

//#include <stdlib.h>
#include <sys/types.h>
#include <inttypes.h>
//#include <ctype.h>
//#include <sys/time.h>
//#include <video_image.h>

#include "snowmix.h"
//#include <malloc.h>

int SetAnchor(int32_t* anchor_x, int32_t* anchor_y, const char* anchor, u_int32_t width, u_int32_t height);
bool set_fd_nonblocking(int fd);

bool TrimFileName(char* name);

bool NumberWithPI(const char* str, double* res);

// Returns range from string
// format : all | <number>..<number> | <number>..end
const char* GetIdRange(const char* str, u_int32_t* first, u_int32_t* last, u_int32_t bottom, u_int32_t top);

const char* FindUTF8Multibyte(const char* str);
u_int32_t UTF8Bytes(const char c);

class CAverage {
  public:
	u_int32_t p;
	CAverage(u_int32_t count);
	~CAverage();
	void	Add(double value);
	double	Average();
	double	Max();
	double	Min();
	double	Last(int index=-1);

  private:
	u_int32_t	m_count;
	u_int32_t	m_index;
	double*		m_array;
	double		m_total;
};

#define CairoFilter2String(a) (a == CAIRO_FILTER_FAST ? "fast" : a == CAIRO_FILTER_GOOD ? "good" : a == CAIRO_FILTER_BEST ? "best" : a == CAIRO_FILTER_NEAREST ? "nearest" : a == CAIRO_FILTER_BILINEAR ? "bilinear" : a == CAIRO_FILTER_GAUSSIAN ? "gaussian" : "unknown")

#define time2double(a) ((double) a.tv_sec + ((double) a.tv_usec) / 1000000.0)


#define double2time(a,time) time.tv_sec = (typeof(time.tv_sec))(a) ; time.tv_usec = (typeof(time.tv_usec))((a - time.tv_sec)*1000000.0)

#define list_get_head_withframe(list,listtail, buf) { \
	if (list == NULL || listtail == NULL) {	\
		buf = NULL;			\
		list = NULL;			\
		listtail = NULL;		\
	} else {				\
		buf = list;			\
		list = buf->next_withframe;	\
		if (list) list->prev_withframe = NULL;	\
		else listtail = NULL;		\
	}					\
} while (0)


/* Function macro to add a element to the tail of a list (speeded up version). */
#define list_add_tail_withframe(list,listtail,new) {               \
	new->next_withframe = NULL;		\
        if (list == NULL || listtail == NULL) { \
                list = new;                     \
                listtail = new;                 \
		new->prev_withframe = NULL;	\
        } else {                                \
		new->prev_withframe = listtail;	\
		listtail->next_withframe = new;	\
		listtail = new;			\
        }                                       \
} while (0)

#endif	// SNOWMIX_UTIL_H
