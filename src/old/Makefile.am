#
# GST_LIBS= -pthread -lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lxml2 -lglib-2.0


AM_CPPFLAGS	= -I/opt/local/include \
	-I.		\
	@PNG_CFLAGS@		\
	@CAIRO_CFLAGS@		\
	@PANGO_CFLAGS@		\
	@OSMESA_CFLAGS@		\
	@GL_CFLAGS@		\
	@GLU_CFLAGS@		\
	@GLUT_CFLAGS@		\
	@SDL_CFLAGS@

#	@GSTREAMER_CFLAGS@
#	@GLEW_CFLAGS@		\
#
# WITH_SDL
# WITH_CAIRO
# WITH_GSTREAMER
# WITH_X264
AM_CXXFLAGS = -D_REENTRANT	\
	-DNOCONTROLS		\
	-Wall			\
	-fexceptions		\
	-DWITH_SDL		\
	-DWITH_CAIRO		\
	@WITH_OSMESA@		\
	@WITH_X11@		\
	@WITH_GLU@		\
	@PLATFORM_OS@		\
	@MMAN_H@		\
	@GLUT_H@		\
	@MALLOC_H@		\
	@SDL_CFLAGS@		\
	@TCLFLAGS@		\
	@TCLVERSION@


#	@WITH_GLEW@		\
#
#include_HEADERS = snowmix.h

#noinst_LTLIBRARIES = libsnowcup.la libvideotest.la
noinst_LTLIBRARIES = libsnowmix.la
noinst_HEADERS =	\
	add_by.h	\
	audio_clip.h	\
	audio_feed.h	\
	audio_mixer.h	\
	audio_sink.h	\
	audio_util.h	\
	cairo_graphic.h	\
	command.h	\
	controller.h	\
	monitor.h	\
	opengl_video.h	\
	snowmix.h	\
	snowmix_util.h	\
	tcl_interface.h	\
	version.h	\
	video_feed.h	\
	video_image.h	\
	video_mixer.h	\
	video_output.h	\
	video_shape.h	\
	video_text.h	\
	virtual_feed.h

### Program Part
bin_PROGRAMS = snowmix
# bars_SOURCES = bars.cpp


######################### SNOWMIX #######################

snowmix_SOURCES = main.cpp

snowmix_LDADD = libsnowmix.la	\
	-lpthread		\
	@WITH_LIBRT@		\
	@PNG_LIBS@		\
	@TCL_LIBS@		\
	@PNG_LIBS@		\
	@CAIRO_LIBS@		\
	@PANGO_LIBS@		\
	@OSMESA_LIBS@		\
	@GL_LIBS@		\
	@GLU_LIBS@		\
	@GLUT_LIBS@		\
	@X11_LIBS@		\
	@SDL_LIBS@

#	-lglut			\
#	@GLEW_LIBS@		\
#	-lX11			\
#	@GTK_PLUS_LIBS@		\
#	@GSTREAMER_LIBS@
#	-lGL   -lGLEW   -lGLU -lGL \
#

libsnowmix_la_SOURCES =		\
	add_by.h		\
	audio_clip.cpp		\
	audio_clip.h		\
	audio_feed.cpp		\
	audio_feed.h		\
	audio_mixer.cpp		\
	audio_mixer.h		\
	audio_sink.cpp		\
	audio_sink.h		\
	audio_util.cpp		\
	audio_util.h		\
	cairo_graphic.cpp	\
	cairo_graphic.h		\
	controller.h		\
	controller.cpp		\
	command.h		\
	command.cpp		\
	monitor.cpp		\
	monitor.h		\
	opengl_video.h		\
	opengl_video.cpp	\
	snowmix.h		\
	snowmix_util.h		\
	snowmix_util.cpp	\
	tcl_interface.h		\
	tcl_interface.cpp	\
	video_shape.h		\
	video_shape.cpp		\
	video_feed.h		\
	video_feed.cpp		\
	video_image.h		\
	video_image.cpp		\
	video_mixer.cpp		\
	video_mixer.h		\
	video_output.cpp	\
	video_output.h		\
	video_text.cpp		\
	video_text.h		\
	virtual_feed.cpp	\
	virtual_feed.h


######################### SNOWMIX GUI ###################
if SNOWMIXGUI

AM_CPPFLAGS += @GTK_PLUS_CFLAGS@

bin_PROGRAMS += snowmixgui

snowmixgui_SOURCES = maingui.cpp

snowmixgui_LDADD = $(snowmix_LDADD) \
	@GTK_PLUS_LIBS@

snowmixgui_LDFLAGS = -DHAVE_GUI
snowmixgui_CXXFLAGS = $(AM_CXXFLAGS) -DHAVE_GUI
endif





