/*
 * (c) 2014 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __RELAY_H__
#define __RELAY_H__

#include <sys/types.h>
//#include <stdint.h>
//#include <sys/select.h>

class CController;

#define MAX_MTU_SIZE 1500

#ifdef HAVE_MACOSX      // Xcode/LLVM on OS X uses %llu for u_int64_t
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


enum state_enum_t {
	UNDEFINED,
	SETUP,
	READY,
	PENDING,
	CONNECTED,
	RUNNING,
	STALLED,
	STOPPED,
	DISCONNECTED
};

#define State2String(a) (a == UNDEFINED ? "undefined" : \
	a == SETUP ? "setup" :				\
	  a == READY ? "ready" :			\
	    a == PENDING ? "pending" :			\
	      a == CONNECTED ? "connected" :		\
	        a == RUNNING ? "running" :		\
	          a == STALLED ? "stalled" :		\
	            a == STOPPED ? "stopped" :		\
	              a == DISCONNECTED ? "disconnected" : \
			"unknown")

enum source_type_t {
	SOURCE_UNDEFINED,
	SOURCE_RECEIVER,
	SOURCE_SENDER,
	SOURCE_ANALYZER
};

#define SourceType2String(a) ( a == SOURCE_UNDEFINED ? "undefined" :    \
	a == SOURCE_RECEIVER ? "receiver" :                             \
	  a == SOURCE_SENDER ? "sender" :				\
	    a == SOURCE_ANALYZER ? "analyzer" : "unknown" )

#define Timeval2U64(a)  (1000000LLU*a.tv_sec+a.tv_usec)

void trim_string(char* str);

//#include "version.h"
#define RELAY_VERSION "0.2"
#define BANNER "relay version "RELAY_VERSION"\n"
//#define STATUS "STAT: "
//#define MESSAGE "MSG: "

// Max number of backlog controller connections
#define MAX_LISTEN_BACKLOG 10


// printf format specs
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

#define ip_parms(a) a>>24,(a&0xff0000)>>16,(a&0xff00)>>8,a&0xff

// Function macro to add a element to the head of a list.
#define list_add_head(list,new) {	\
	new->m_prev = NULL;		\
	new->m_next = list;		\
	if (list != NULL) {		\
		list->m_prev = new;	\
	}				\
	list = new;			\
} while (0)

/* Function macro to remove a element from a list. */
#define list_del_element(list,element) {				\
	/* Remove from memory list. */					\
	if (element->m_prev == NULL) {					\
		/* This is the first element in the chain. */		\
		list = element->m_next;					\
		if (element->m_next != NULL) {				\
			/* Not the last. */				\
			element->m_next->m_prev = NULL;			\
		}							\
	} else {							\
		/* This is not the first element. */			\
		element->m_prev->m_next = element->m_next;		\
		if (element->m_next != NULL) {				\
			/* Not the last. */				\
			element->m_next->m_prev = element->m_prev;	\
		}							\
	}								\
} while (0)

struct host_allow_t {
	u_int32_t		ip_from;
	u_int32_t		ip_to;
	struct host_allow_t*	next;
};

int SetHostAllow(CController* pCtr, const char* str, host_allow_t** host_allow_list, const char* prepend, bool newline = true);
bool HostNotAllowed (u_int32_t ip, host_allow_t* host_allow_list);

#endif	// RELAY_H
