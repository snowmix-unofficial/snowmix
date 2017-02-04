/*
 * (c) 2013 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


//#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
//#include <limits.h>
#include <string.h>
//#include <malloc.h>
//#include <stdlib.h>
//#include <sys/types.h>
#include <math.h>
//#include <string.h>

//#include <sys/types.h>
//#include <sys/stat.h>

#ifndef WIN32
#include <unistd.h>
#else
#define _USE_MATH_DEFINES
#include "windows_util.h"
#endif
#include <math.h>

#include <fcntl.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#include <stdlib.h>		// For some reason calloc and free requires this on OS X and not in other files
#else
#include <stdlib.h>
#endif
#include "snowmix_util.h"

bool set_fd_nonblocking(int fd) {
#ifndef WIN32
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0) return false;
	return (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 ? true : false);
#else
	unsigned long ulValue = 1;
	return (ioctlsocket(fd, FIONBIO, &ulValue) != 0 ? true : false);
#endif
};


static void CopyOverlapString(char* dest, char* src) {
	if (!dest || !src) return;
	while (*src) *dest++ = *src++;
	*dest = *src;
}

/*
 * TrimFileName will remove unnecessary .. and .
 *
 * // => /
 * ../xyz => ../xyz
 * ./xyz => xyz
 *
 * xyz/./abc => xyz/abc
 * xyz/../abc => abc
 *
 */
bool TrimFileName(char* name) {
	if (!name || !(*name)) return true;

	// Remove initial repeated '/'
	if (*name == '/' && *(name+1) == '/') {
		char* p = name+2;
		while (*p == '/') p++;
		CopyOverlapString(name+1,p);

	// Keep '../xyz'
	} else if (*name == '.' && *(name+1) == '.' && *(name+2) == '/') {
		// could be '..//'
		if (*(name+3) == '/') {
			char* p = name+4;
			while (*p == '/') p++;
			CopyOverlapString(name+3,p);
		}
		name += 3;

	// remove leading './'
	} else if (*name == '.' && *(name+1) == '/') {
		char* p = name+2;
		while (*p == '/') p++;
		CopyOverlapString(name,p);
	}

	// keep intial './'
	while (*name == '.' && *(name+1) == '/') {
		if (*(name+2) == '/') {
			char* p = name+3;
			while (*p == '/') p++;
			CopyOverlapString(name+2,p);
		}
		name += 2;
	}
	if (*name == '/') name++;
	char* dir = name;

	while (*name) {
		// dirname or filename starting
		if (*name == '/') {
			// check for multiple '//'
			if (*(name+1) == '/') {
				char* p = name+2;
				while (*p == '/') p++;
				CopyOverlapString(name+1,p);
				continue;
			}
			if (*(name+1) == '.' && *(name+2) == '/') {
				CopyOverlapString(name+1, name+3);
				name++;
				dir = name;
				continue;
			}
			if (*(name+1) == '.' && *(name+2) == '.' && *(name+3) == '/') {
				CopyOverlapString(dir, name+4);
				name=dir;
				continue;
			}
			dir = name+1;
		}
		name++;
	}
	return false;
}
			

int SetAnchor(int32_t* anchor_x, int32_t* anchor_y, const char* anchor, u_int32_t width, u_int32_t height) {
	if (!anchor_x || !anchor_y || !anchor || !(*anchor) || !width || !height) return -1;
	if (strcasecmp("nw", anchor) == 0) {
		*anchor_x = 0;
		*anchor_y = 0;
	} else if (strcasecmp("ne", anchor) == 0) {
		*anchor_x = width;
		*anchor_y = 0;
	} else if (strcasecmp("se", anchor) == 0) {
		*anchor_x = width;
		*anchor_y = height;
	} else if (strcasecmp("sw", anchor) == 0) {
		*anchor_x = 0;
		*anchor_y = height;
	} else if (strcasecmp("n", anchor) == 0) {
		*anchor_x = width>>1;
		*anchor_y = 0;
	} else if (strcasecmp("w", anchor) == 0) {
		*anchor_x = width;
		*anchor_y = height>>1;
	} else if (strcasecmp("s", anchor) == 0) {
		*anchor_x = width>>1;
		*anchor_y = height;
	} else if (strcasecmp("e", anchor) == 0) {
		*anchor_x = width;
		*anchor_y = height>>1;
	} else if (strcasecmp("c", anchor) == 0) {
		*anchor_x = width>>1;
		*anchor_y = height>>1;
	} else return -1;
	return 0;
}

bool NumberWithPI(const char* str, double* res)
{
	double divisor;
	if (!str || !res) return false;
	while (isspace(*str)) str++;
	if (!(*str)) return false;
	if (*str == 'P' && *(str+1) == 'I') {
		str += 2;
		*res = M_PI;
	} else {
		if (sscanf(str, "%lf", res) != 1) return false;
		
		if (*str == '-' || *str == '+') str++;
		while (isdigit(*str)) str++;
		if (*str == '.') str++;
		while (isdigit(*str)) str++;
		if (*str == 'P' && *(str+1) == 'I') {
			str += 2;
			*res *= M_PI;
		}
	}
	if (sscanf(str, "/%lf", &divisor) == 1) {
		if (divisor == 0.0) return false;
		*res /= divisor;
	}
	return true;
}

const char* FindUTF8Multibyte(const char* str) {
	if (!str) return NULL;
	while ((*str) && (((*str) & 0xb0) != 0xb0)) str++;
	return *str ? str : NULL;
}

u_int32_t UTF8Bytes(const char c) {
	u_int32_t bytes;

	// is the 2 msb set ? then 2 or more chars
	if ((c & 0xc0) == 0xc0)  {
		// is the 3rd msb set ? then 3 or more chars
		if (c & 0x20) {
			// is the 4 msb set ? then 4 or more chars
			if (c & 0x10) {
				// is the 5 msb set ? then 5 or more chars
				if (c & 0x08) {
					// is the 6th msb set ? then 6 or more chars
					if (c & 0x08) bytes = 6;
					else bytes = 5;
				} else bytes = 4;
			} else bytes = 3;
		} else bytes = 2;
	} else bytes = 1;
	return bytes;
}

// Returns range from string
// format : all | <number>..<number> | <number>..end
const char* GetIdRange(const char* str, u_int32_t* first, u_int32_t* last, u_int32_t bottom, u_int32_t top) {
	if (!str || !first || !last) return NULL;
	//const char* tmpstr = str;
	while(isspace(*str)) str++;

	// case : all
	if (strncmp(str,"all",3) == 0 && (!str[3] || isspace(str[3]))) {
		str += 3;
		*first = bottom;
		*last = top;
		return str;
	}
	// case : end
	if (strncmp(str,"end",3) == 0 && (!str[3] || isspace(str[3]))) {
		str += 3;
		*first = top;
		*last = top;
		return str;
	}
	// cases : <number> | <number>..end | <number>..<number>
	if (sscanf(str, "%u", first) == 1) {

		// Check that first is within range
		if (*first > top) return NULL;

		// Skip past first number value
		while ((*str) && isdigit(*str)) str++;

		// case : <number>
		if (!(*str) || isspace(*str)) {
			*last = *first;
			return str;
		}

		if (strncmp(str,"..",2)) return NULL;
		str += 2;

		// case : <number>..end
		if (strncmp(str,"end",3) == 0 && (!str[3] || isspace(str[3]))) {
			*last = top;
			return str+3;
		}

		// case : <number>..<number>
		if (sscanf(str, "%u", last) == 1) {
			// Skip past second number value
			while ((*str) && isdigit(*str)) str++;
			if (!(*str) || isspace(*str)) {
				//*last = *first;
				return str;
			}
		}

		// otherwise illegal syntax
	}
	return NULL;
}

//CAverage::CAverage(u_int32_t count)
CAverage::CAverage(u_int32_t count) {
	m_count = count > 1 ? count : 2;
	m_array = (double*) calloc(count, sizeof(double));
	m_index = 0;
	m_total = 0.0;
	m_count = count;
}
CAverage::~CAverage()
{
	if (m_array) free(m_array);
}

void CAverage::Add(double value)
{
	if (!m_array) return;
	m_total += (value - m_array[m_index]);
	m_array[m_index++] = value;
	m_index %= m_count;
}
double CAverage::Last(int index)
{
	if (!m_array) return 0.0;
	if (index < 0) return m_array[(m_index+m_count-1)%m_count];
	return m_array[(m_index + m_count - index) % m_count];

}
double CAverage::Average()
{
	return m_total / m_count;
}
double CAverage::Max()
{
	if (!m_array) return 0.0;
	u_int32_t n = 0;
	double max = m_array[0];
	for (n=1 ; n < m_count ; n++) {
	  if (m_array[n] > max) max = m_array[n];
	}
	return max;
}
double CAverage::Min()
{
	if (!m_array) return 0.0;
	u_int32_t n = 0;
	double min = m_array[0];
	for (n=1 ; n < m_count ; n++) {
	  if (m_array[n] < min) min = m_array[n];
	}
	return min;
}
