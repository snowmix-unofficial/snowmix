/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __VIDEO_MIXER_H__
#define __VIDEO_MIXER_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
#ifdef WIN32
#include "windows_util.h"
#else
#include <sys/time.h>
#endif

#include "snowmix.h"
#include "monitor.h"
#include "audio_feed.h"
#include "audio_mixer.h"
#include "audio_sink.h"
#include "video_feed.h"
#include "video_text.h"
//#include "video_scaler.h"
#include "controller.h"
#include "video_image.h"
#include "video_shape.h"
#include "virtual_feed.h"
#include "tcl_interface.h"
#include "cairo_graphic.h"
#include "video_output.h"

class CTclInterface;
class CCairoGraphic;
class CVideoImage;
class CVideoFeeds;
class CVideoText;
class CVirtualFeed;
class CAudioFeed;
class CAudioMixer;
class CAudioSink;
class CVideoShape;
class CVideoOutput;
#if defined (HAVE_OSMESA) || defined (HAVE_GLX)
class COpenglVideo;
#endif

class CVideoMixer {
  public:
	CVideoMixer();
	~CVideoMixer();
	int		MainMixerLoop();
	int		MainMixerLoop2();
	int		SetGeometry(const u_int32_t width,const u_int32_t height, const char* pixelformat);
	int 		output_producer (struct feed_type* system_feed, struct timeval time_now, bool late = false);
	int		OverlayFeed(const char *str);
	int		OverlayImage(const char *str);
	int		OverlayText(const char *str, CCairoGraphic* pCairoGraphic);
	int		CairoOverlayFeed(CCairoGraphic* pCairoGraphic, char* str);
	//int		OverlayVirtualFeed(CCairoGraphic* pCairoGraphic, const char* str);
	int		BasicFeedOverlay(u_int32_t feed_no);
	u_int32_t	SystemFrameNo() { return (m_pController ? m_pController->m_frame_seq_no : 0); };
	char*		Query(const char* query, bool tcllist = true);

	// Search Path functions
	bool		AddSearchPath(char* path);
	bool		RemoveSearchPath(int index);
	const char*	GetSearchPath(int index);
	int		OpenFile(const char* file_name, int mode, int perm = -1, char** open_name = NULL);

	// Access to system info
	u_int32_t	GetSystemWidth();
	u_int32_t	GetSystemHeight();
	void		SystemGeometry(u_int32_t *pWidth = NULL, u_int32_t *pHeight = NULL);

	// Variables
	char*		m_mixer_name;
	char		m_pixel_format[5];
	VideoType	m_video_type;
	u_int32_t	m_pixel_bytes;
	double		m_frame_rate;
	u_int32_t	m_block_size;
	//int		m_exit_program;
	//bool		m_system_monitor;
	CMonitor_SDL*	m_pMonitor;
	//CVideoScaler*	m_pVideoScaler;
	CCairoGraphic*	m_pCairoGraphic;
	CTextItems*	m_pTextItems;
	struct timeval	m_start;
	struct timeval	m_start_master;
	struct timeval	m_time_last;
	struct timeval	m_mixer_duration;
	u_int32_t	m_output_called;
        u_int32_t	m_output_missed;
	CVideoImage*	m_pVideoImage;
	CAudioFeed*	m_pAudioFeed;
	CAudioMixer*	m_pAudioMixer;
	CAudioSink*	m_pAudioSink;
	CVideoFeeds*	m_pVideoFeed;
	CVideoShape*	m_pVideoShape;
	CVirtualFeed*	m_pVirtualFeed;
	CTclInterface*	m_pTclInterface;
	CVideoOutput*	m_pVideoOutput;
#if defined (HAVE_OSMESA) || defined (HAVE_GLX)
	COpenglVideo*	m_pOpenglVideo;
#endif

	// Overall control variables.
	char*		m_pOutput_master_name;
	int		m_output_stack[MAX_STACK];

	CController*	m_pController;			// Active controller for this mixer

	u_int8_t*	m_overlay;			// Active overlay
	char*		m_command_pre;			// Name of command to run always and before overlay
	char*		m_command_finish;		// Name of command to run before output
	char*		m_write_png_file;		// Write a PNG file of output once if not NULL
	char**		m_search_path;			// Array of search paths

  private:
	char		m_search_path_size;		// Number of places in array
	u_int32_t	m_geometry_width;		// System Geometry
	u_int32_t	m_geometry_height;		// System Geometry
	u_int32_t	m_verbose;
};

#endif	// VIDEO_MIXER
