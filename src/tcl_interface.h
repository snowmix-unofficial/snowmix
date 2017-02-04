/*
 * (c) 2012 Peter Maersk-Moller. All rights reserved. This code
 * contains trade secrets of Peter Maersk-Moller and any
 * unauthorized use or disclosure is strictly prohibited.
 *
 * Contributor : Peter Maersk-Moller    snowmix@maersk-moller.net
 *
 *
 */

#ifndef __TCL_INTERFACE_H__
#define __TCL_INTERFACE_H__

//#include <stdlib.h>
#include <sys/types.h>
//#include <SDL/SDL.h>
//#include <pango/pangocairo.h>
//#include <sys/time.h>
//#include <video_image.h>

#include "video_mixer.h"

#ifdef HAVE_TCL
#ifdef WITH_TCL86
#include <tcl8.6/tcl.h>
#else
#ifdef WITH_TCL85
#include <tcl8.5/tcl.h>
#else
#ifdef WITH_TCL
#include <tcl.h>
#else
#error No tcl8.6 or tcl8.5 found
#endif
#endif
#endif
#endif


class CTclInterface {
  public:
	CTclInterface(CVideoMixer* pVideoMixer);
	~CTclInterface();

	int	ParseCommand(struct controller_type* ctr, const char* str);
	int	InterpreterCreated() { return (m_pInterp ? true : false); };

	struct controller_type* m_ctr;
	CVideoMixer*		m_pVideoMixer;

 private:
	int	list_help(struct controller_type* ctr, const char* str);
#ifdef HAVE_TCL
	int	set_tcl_eval(struct controller_type* ctr, const char* str);
	int	set_tcl_exec(struct controller_type* ctr, const char* str);

	Tcl_Interp*	 m_pInterp;
#endif

};
	
#endif	// TCL_INTERFACE_H
