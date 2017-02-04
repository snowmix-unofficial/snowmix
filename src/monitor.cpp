/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */


//#include <unistd.h>
#include <stdio.h>
//#include <limits.h>
#include <string.h>
#ifdef HAVE_MALLOC
#include <malloc.h>
#else
#include <stdlib.h>
#endif
//#include <sys/types.h>

//#include "video_scaler.h"
#include "monitor.h"

static bool SDLInitialized = false;

CMonitor_SDL::CMonitor_SDL(u_int32_t width, u_int32_t height, VideoType video_type) {
	m_screen = NULL;
	m_sdl_overlay = NULL;
	m_width = width;
	m_height = height;
	m_video_type = video_type;
fprintf(stderr, "VIDEO INIT\n");
	if (!SDLInitialized) {
		if (SDL_Init(SDL_INIT_VIDEO)) {
			fprintf(stderr, "CMonitor_SDL Failed to init SDL\n");
			//exit(0);
		} else SDLInitialized = true;
	}
	if (m_video_type == VIDEO_ARGB || m_video_type == VIDEO_BGRA) {
		//u_int32_t redmask, greenmask, bluemask, alphamask;
		if (video_type == VIDEO_ARGB) {
fprintf(stderr, "VIDEO BGRA\n");
//fprintf(stderr, "Monitor %ux%u\n", m_width, m_height);
			m_yuv_size = m_width * m_height * 4;
/*
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
			redmask		= 0x00ff0000;
			greenmask	= 0x0000ff00;
			bluemask	= 0x000000ff;
			alphamask	= 0xff000000;
#else
			redmask		= 0x0000ff00;
			greenmask	= 0x00ff0000;
			bluemask	= 0xff000000;
			alphamask	= 0x000000ff;
#endif
*/
		if (!(m_screen = SDL_SetVideoMode(m_width, m_height,0,SDL_RESIZABLE))) return;
	//		m_screen = SDL_CreateRGBSurface(SDL_HWSURFACE, m_width, m_height, 32,
	//			redmask, greenmask, bluemask, alphamask);
		//	m_sdl_overlay = SDL_CreateYUVOverlay(m_width, m_height, SDL_YUY2_OVERLAY, m_screen);
		}
	} else if (m_video_type == VIDEO_YUY2) {
fprintf(stderr, "VIDEO YUV2\n");
		if (!(m_screen = SDL_SetVideoMode(m_width, m_height,0,0))) return;
		m_sdl_overlay = SDL_CreateYUVOverlay(m_width, m_height, SDL_YUY2_OVERLAY, m_screen);
		m_yuv_size = 2 * m_width * m_height;
	} else if (m_video_type == VIDEO_I420) {
fprintf(stderr, "VIDEO I420\n");
		if (!(m_screen = SDL_SetVideoMode(m_width, m_height,0,0))) return;
		m_sdl_overlay = SDL_CreateYUVOverlay(m_width, m_height,
			SDL_IYUV_OVERLAY, m_screen);
		m_yuv_size = m_width * m_height + ((m_width*height)>>1);
	} else {
		fprintf(stderr, "CMonitor error. Unrecognized video format\n");
		return;
	}
	m_rect.x = 0;
	m_rect.y = 0;
	m_rect.w = m_width;;
	m_rect.h = m_height;
}
CMonitor_SDL::~CMonitor_SDL() {
	if (m_sdl_overlay) SDL_FreeYUVOverlay(m_sdl_overlay);
	if (m_screen) SDL_Quit();
}

void CMonitor_SDL::ShowDisplay(u_int8_t* pOverlay, int delay) {
	if (m_sdl_overlay && pOverlay) {
		SDL_LockYUVOverlay(m_sdl_overlay);
		//memcpy(m_sdl_overlay->pixels[0], pOverlay, m_yuv_size);
		SDL_UnlockYUVOverlay(m_sdl_overlay);
		SDL_DisplayYUVOverlay(m_sdl_overlay, &m_rect);
		if (delay) SDL_Delay(delay);
	} else if (pOverlay) {
		SDL_LockSurface(m_screen);
		//SDL_Surface* src = SDL_CreateRGBSurfaceFrom(pOverlay, m_width, m_height, 32, m_width*4, 0,0,0,0);
#ifdef HAVE_MACOSX

		u_int8_t* dst = (u_int8_t *) m_screen->pixels;
		for (unsigned int row=0; row < m_height; row++) {
			for (unsigned int col=0; col < m_width; col++) {
//				*dst++ = pOverlay[3];
//				*dst++ = pOverlay[2];
//				*dst++ = pOverlay[1];
//				*dst++ = pOverlay[0];
				*((unsigned int*)(dst)) = htonl(*((unsigned int*)(pOverlay)));
				dst += 4;
				pOverlay += 4;
			}
		}
#else
		memcpy((u_int8_t *) m_screen->pixels, pOverlay, (m_width*m_height)<<2);
#endif


		SDL_UpdateRect(m_screen, 0,0,0,0);
		SDL_UnlockSurface(m_screen);
	}
}
