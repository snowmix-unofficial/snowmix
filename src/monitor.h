/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __MONITOR_H__
#define __MONITOR_H__

//#include <stdlib.h>
#include <sys/types.h>
#include <SDL/SDL.h>
//#include "video_scaler.h"
#include "snowmix.h"

class CMonitor_SDL {
  public:
	CMonitor_SDL(u_int32_t width, u_int32_t height, VideoType video_type); 
	~CMonitor_SDL();
	void ShowDisplay(u_int8_t* pOverlay, int delay = 0);
  private:
	VideoType	m_video_type;	// Defines video layout
	u_int32_t	m_width;	// Source width
	u_int32_t	m_height;	// Source height
	u_int32_t	m_yuv_size;	// YUV Overlay size

	SDL_Surface*	m_screen;
	SDL_Overlay*	m_sdl_overlay;
	SDL_Rect	m_rect;
};

#endif	// MONITOR
