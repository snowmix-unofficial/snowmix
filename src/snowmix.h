/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __SNOWMIX_H__
#define __SNOWMIX_H__

#include <sys/types.h>
//#include <inttypes.h>
#include <stdint.h>
#ifdef HAVE_PCBSD
#define __WORDSIZE 64
#endif

#ifdef WIN32
#include "windows_util.h"
#endif

#include "version.h"
#define BANNER "Snowmix version "SNOWMIX_VERSION".\n"
#define STATUS "STAT: "
#define MESSAGE "MSG: "
#define ANCHOR_SYNTAX "(n | s | e | w | c | ne | nw | se | sw)"
#define ANCHOR_TAGS "nsewc"
#define SNOWMIX_AUTHOR "Peter Maersk-Moller"


#ifdef HAVE_MACOSX	// Xcode/LLVM on OS X uses %llu for u_int64_t
#define FUI64 "%llu"
#define FSI64 "%lld"
#else
#if __WORDSIZE == 32
#define FUI64 "%llu"
#define FSI64 "%lld"
#else
#if __WORDSIZE == 64
#define FUI64 "%lu"
#define FSI64 "%ld"
#else
#error Unknown WORDSIZE to determine format of u_int64_t in printf
#endif
#endif
#endif

// Video types. Layout shown below
// YUY2   : Y0U0Y1V0 Y2U1Y3V1 Y4U2Y5V2 Y6U3Y7V3
// YUV420 : Y0Y1Y2Y3Y4Y5Y6Y7 U0U1 V0V1
typedef enum VideoType {
	VIDEO_BGRA,
	VIDEO_ARGB,
	VIDEO_BGR,
	VIDEO_RGB,
	VIDEO_YUY2,
	VIDEO_I420,
	VIDEO_NONE
} VideoType;

// Forward declaration
class CVideoMixer;
struct feed_type;

/* Define the max number of feeds in a single output. */
#define MAX_STACK       10

// Define the max backlog for listening for control connections
#define MAX_LISTEN_BACKLOG 10

enum feed_state_enum {
	UNDEFINED,
	SETUP,
	READY,
	PENDING,
	RUNNING,
	STALLED,
	DISCONNECTED,
	NO_FEED
};

#define SNOWMIX_ALIGN_LEFT   1
#define SNOWMIX_ALIGN_CENTER 2
#define SNOWMIX_ALIGN_RIGHT  4
#define SNOWMIX_ALIGN_TOP    8
#define SNOWMIX_ALIGN_MIDDLE 16
#define SNOWMIX_ALIGN_BOTTOM 32


#define feed_state_string(state)	(state == RUNNING ? "RUNNING" : state == PENDING ? "PENDING" : state == READY ? "READY" : state == SETUP ? "SETUP" : state == STALLED ? "STALLED" : state == DISCONNECTED ? "DISCONNECTED" : "UNKNOWN")

extern int exit_program;

/********************************************************************************
*										*
*	Stuff for handling gstreamer shmsink/shmsrc data.			*
*										*
********************************************************************************/

#define MAX_FIFO_DEPTH	10
struct frame_fifo_type {
	struct buffer_type	*buffer;
};

struct shm_commandbuf_type {
	unsigned int	type;
	int		area_id;

	union {
		struct {
			size_t		size;
			unsigned int	path_size;
		} new_shm_area;
		struct {
			unsigned long	offset;
			unsigned long	bsize;
		} buffer;
		struct {
			unsigned long	offset;
		} ack_buffer;
	} payload;
};

// Forward decleration (for CVideoMixer)
//struct feed_type *find_feed (int id);
//struct output_memory_list_type *output_get_free_block (CVideoMixer* pVideoMixer);

/************************************************************************
*                                                                       *
*       Output handling functions.                                      *
*                                                                       *
************************************************************************/

void dump_cb (u_int32_t block_size, const char *src, const char *dst, struct shm_commandbuf_type *cb, size_t len);
//void output_reset (CVideoMixer* pVideoMixer);

/* Function macro to add a element to the head of a list. */
#define list_add_head(list,new) {	\
	new->prev = NULL;		\
	new->next = list;		\
	if (list != NULL) {		\
		list->prev = new;	\
	}				\
	list = new;			\
} while (0)

/* Function macro to add a element to the tail of a list (somewhat slow). */
#define list_add_tail(list,new) {		\
	if (list == NULL) {			\
		list = new;			\
	} else {				\
		typeof (new) e = list;		\
		while (e->next != NULL) {	\
			e = e->next;		\
		}				\
		e->next = new;			\
		new->prev = e;			\
	}					\
	new->next = NULL;			\
} while (0)


/* Function macro to remove a element from a list. */
#define list_del_element(list,element) {				\
	/* Remove from memory list. */					\
	if (element->prev == NULL) {					\
		/* This is the first element in the chain. */		\
		list = element->next;					\
		if (element->next != NULL) {				\
			/* Not the last. */				\
			element->next->prev = NULL;			\
		}							\
	} else {							\
		/* This is not the first element. */			\
		element->prev->next = element->next;			\
		if (element->next != NULL) {				\
			/* Not the last. */				\
			element->next->prev = element->prev;		\
		}							\
	}								\
} while (0)



#endif	// SNOWMIX
