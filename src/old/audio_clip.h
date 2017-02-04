/*
 * (c) 2013 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __AUDIO_CLIP_H__
#define __AUDIO_CLIP_H__

#include <sys/types.h>
//#include <sys/select.h>

//#include "SDL.h"
//#include "SDL_thread.h"

//#include <snowmix.h>
//#include <audio_util.h>
#include "controller.h"
#include "video_mixer.h"

class CAudioClip {
  public:
	CAudioClip(CVideoMixer* pVideoMixer);
	~CAudioClip();

	int	ParseCommand(struct controller_type* ctr, const char* str);


	int		m_verbose;
private:
	int	clip_load(struct controller_type* ctr, const char* str);

	CVideoMixer*	m_pVideoMixer;
};
	
#endif	// __AUDIO_CLIP_H__
