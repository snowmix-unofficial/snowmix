#
# GST_LIBS= -pthread -lgstreamer-0.10 -lgobject-2.0 -lgmodule-2.0 -lgthread-2.0 -lrt -lxml2 -lglib-2.0


AM_CPPFLAGS	= -I.

#	@GTK_PLUS_CFLAGS@	\
#	@PNG12_CFLAGS@		\
#	@PANGO_CFLAGS@		\
#	@OSMESA_CFLAGS@		\
#	@GLEW_CFLAGS@		\
#	@SDL_CFLAGS@
#
#	@GSTREAMER_CFLAGS@

#include_HEADERS = snowmix.h

noinst_LTLIBRARIES = librelay.la
noinst_HEADERS = relay.h

### Program Part
bin_PROGRAMS = relay
#

relay_SOURCES = main.cpp \
	relay.h



relay_LDADD = librelay.la	\
	-lpthread		\
	@WITH_LIBRT@		\
	@SDL_LIBS@

#snowmix_LDADD = libsnowmix.la	\
#	-lpthread		\
#	@WITH_LIBRT@		\
#	@PNG12_LIBS@		\
#	@TCL_LIBS@		\
#	@PNG12_LIBS@		\
#	@PANGO_LIBS@		\
#	@OSMESA_LIBS@		\
#	@GLEW_LIBS@		\
#	@SDL_LIBS@
#
#	@GTK_PLUS_LIBS@		\
#	@GSTREAMER_LIBS@
#	-lGL   -lGLEW   -lGLU -lGL \
#

librelay_la_SOURCES =	\
	relay.h		\
	controller.cpp	\
	controller.h	\
	queues.cpp	\
	queues.h	\
	receivers.cpp	\
	receivers.h	\
	senders.cpp	\
	senders.h	\
	tsanalyzer.h	\
	tsanalyzer.cpp
	

##$(LIBOBJS)

# WITH_SDL
# WITH_CAIRO
# WITH_PANGO
# WITH_GSTREAMER
# WITH_X264

AM_CXXFLAGS = -D_REENTRANT -DNOCONTROLS -Wall -fexceptions -DWITH_SDL -DWITH_CAIRO -DWITH_PANGO @PLATFORM_OS@ @MALLOC_H@ @SDL_CFLAGS@ @TCLFLAGS@ @TCLVERSION@

