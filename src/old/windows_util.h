/*
 * (c) 2015 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __WINDOWS_UTIL_H__
#define __WINDOWS_UTIL_H__

#include "stdafx.h"
#include <time.h>
#include <windows.h> 
#include <winsock.h>

#ifdef WIN32
#include <stdint.h>
#ifndef u_int8_t
typedef uint8_t u_int8_t;
#endif
#ifndef u_int16_t
typedef uint16_t u_int16_t;
#endif
#ifndef u_int32_t
typedef uint32_t u_int32_t;
#endif
#ifndef u_int64_t
typedef uint64_t u_int64_t;
#endif
#define __WORDSIZE 64
#endif


const __int64 DELTA_EPOCH_IN_MICROSECS= 11644473600000000;

/* IN UNIX the use of the timezone struct is obsolete;
 I don't know why you use it. See http://linux.about.com/od/commands/l/blcmdl2_gettime.htm
 But if you want to use this structure to know about GMT(UTC) diffrence from your local time
 it will be next: tz_minuteswest is the real diffrence in minutes from GMT(UTC) and a tz_dsttime is a flag
 indicates whether daylight is now in use
*/
struct timezone2 
{
  __int32  tz_minuteswest; /* minutes W of Greenwich */
  bool  tz_dsttime;     /* type of dst correction */
};

struct timeval2 {
__int32    tv_sec;         /* seconds */
__int32    tv_usec;        /* microseconds */
};

struct timespec {
	__int64	tv_sec;
	__int64	tv_nsec;
};

int gettimeofday(struct timeval *tv, struct timezone2 *tz);

/* timercmp is defined in winsock.h
# define timercmp(a, b, CMP)                                                  \
  (((a)->tv_sec == (b)->tv_sec) ?                                             \
   ((a)->tv_usec CMP (b)->tv_usec) :                                          \
   ((a)->tv_sec CMP (b)->tv_sec))
*/
# define timeradd(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec + (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec + (b)->tv_usec;                          \
    if ((result)->tv_usec >= 1000000)                                         \
	      {                                                                   \
        ++(result)->tv_sec;                                                   \
        (result)->tv_usec -= 1000000;                                         \
	      }                                                                   \
    } while (0)
# define timersub(a, b, result)                                               \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;                          \
    if ((result)->tv_usec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_usec += 1000000;                                           \
	    }                                                                     \
    } while (0)

#ifdef _MSC_VER 
//not #if defined(_WIN32) || defined(_WIN64) because we have strncasecmp in mingw
#define strncasecmp _strnicmp
#define strcasecmp	_stricmp
#define strdup		_strdup
#define open		_open
#define close		_close
#define unlink		_unlink
#define write		_write
#define read		_read
#define lseek		_lseek
#define getcwd		_getcwd
#define index		strchr
#define socklen_t	int
#define ssize_t		int
#define typeof		decltype
#define MAP_FAILED	((void *) -1)
#define MSG_NOSIGNAL 0
#endif

#endif	// __WINDOWS_UTIL_H__
