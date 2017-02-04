/*
 * (c) 2013 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef WIN32
#include <unistd.h>
#endif
//#include <stdio.h>

//#include <sys/types.h>
//#include <string.h>

//#include <sys/mman.h>
//#include <sys/stat.h>
//#include <sys/socket.h>
//#include <sys/un.h>
//#include <fcntl.h>
//#include <errno.h>

//#include "snowmix_util.h"
#include "audio_clip.h"

//#define system_frame_no m_pVideoMixer->m_pController->m_frame_seq_no

/*
static int ClipLoader(void* p)
{
	CAudioClip* pAudioClip = (CAudioClip*) p;
	if (!p) {
		fprintf(stderr, "CAudioClip ClipLoader called with NULL "
			"pointer. Exiting\n");
		exit(1);
	}
	if (pAudioClip->m_verbose) fprintf(stderr, "AudioClip Clip Loader thread started.\n");
	return 0;
}
*/

CAudioClip::CAudioClip(CVideoMixer* pVideoMixer)
{
	m_verbose = 1;
	if (!(m_pVideoMixer = pVideoMixer))
		fprintf(stderr, "Warning : pVideoMixer set to NULL for CAudioClip\n");
}
CAudioClip::~CAudioClip()
{
}

int CAudioClip::ParseCommand(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) return -1;

	// audio clip load [<clip id> [<filename>]]
	if (!strncasecmp (str, "load ", 5)) {
		return clip_load(ctr, str+5);
	}
	return -1;
}

int CAudioClip::clip_load(struct controller_type* ctr, const char* str)
{
	if (!str) return -1;
	while (isspace(*str)) str++;
	if (!(*str)) {
		if (!m_pVideoMixer || !m_pVideoMixer->m_pController) return 1;
		m_pVideoMixer->m_pController->controller_write_msg (ctr,
			"");

		return 0;
	}

	// Load clip:
	return 0;
}
