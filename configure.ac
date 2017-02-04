AC_INIT([snowmix], [0.5.1], snowmix@maersk-moller.net)
# AM_INIT_AUTOMAKE([Snowmix], [0.5.1])
# AM_INIT_AUTOMAKE([Snowmix], [0.5.1])
AM_INIT_AUTOMAKE
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LN_S
AC_PROG_MKDIR_P

# AC_CHECK_HEADERS([src])
# AM_CONFIG_HEADER(config.h)

WITH_OPENGL=""
SNOWMIX_INIFILES=`find ini -type f | sed 's/^ini\///g' | awk '{ printf("%s ",$0); }'`
SNOWMIX_TCLINCFILES=`ls tcl/*.inc | sed 's/^tcl\///g' | awk '{ printf("%s ",$0); }'`
AC_SUBST(SNOWMIX_INIFILES)
AC_SUBST(SNOWMIX_TCLINCFILES)

AC_ARG_ENABLE([snowmixgui],
[  --enable-snowmixgui     Turn on GUI for Snowmix. Under development.],
[case "${enableval}" in
  yes) snowmixgui=true ;;
  no)  snowmixgui=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-snowmixgui]) ;;
esac],[snowmixgui=false])
AM_CONDITIONAL([SNOWMIXGUI], [test "x$snowmixgui" = "xtrue"])


#AC_ARG_ENABLE([snowmixglew],
#[  --enable-snowmixglew    Turn on GL Extension Wrangler],
#[case "${enableval}" in
#  yes) snowmixglew=true ;;
#  no)  snowmixglew=false ;;
#  *) AC_MSG_ERROR([bad value ${enableval} for --enable-snowmixglew]) ;;
#esac],[snowmixglew=false])
#AM_CONDITIONAL([SNOWMIXGLEW], [test "x$snowmixglew" = "xtrue"])

AC_ARG_ENABLE([snowmixosmesa],
[  --enable-snowmixosmesa   Turn on Off Screen Mesa (OpenGL)],
[case "${enableval}" in
  yes) snowmixosmesa=true ; snowmixgl=true ;;
  no)  snowmixosmesa=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-snowmixosmesa]) ;;
esac],[snowmixosmesa=false])
AM_CONDITIONAL([SNOWMIXMESA], [test "x$snowmixosmesa" = "xtrue"])

AC_ARG_ENABLE([snowmixx11],
[  --enable-snowmixx11     Turn on X11 (OpenGL for X-Windows)],
[case "${enableval}" in
  yes) snowmixx11=true ; snowmixgl=true ;;
  no)  snowmixx11=false ; snowmixgl=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-snowmixx11]) ;;
esac],[snowmixx11=false])
AM_CONDITIONAL([SNOWMIXX11], [test "x$snowmixx11" = "xtrue"])

AC_ARG_ENABLE([snowmixglu],
[  --enable-snowmixglu     Turn on OpenGL Utility Library],
[case "${enableval}" in
  yes) snowmixglu=true ;;
  no)  snowmixglu=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-snowmixglu]) ;;
esac],[snowmixglu=false])
AM_CONDITIONAL([SNOWMIXGLU], [test "x$snowmixglu" = "xtrue"])

AC_ARG_ENABLE([snowmixglut],
[  --enable-snowmixglut     Turn on OpenGL Utility Toolkit Library],
[case "${enableval}" in
  yes) snowmixglut=true ;;
  no)  snowmixglut=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-snowmixglut]) ;;
esac],[snowmixglut=false])
AM_CONDITIONAL([SNOWMIXGLUT], [test "x$snowmixglut" = "xtrue"])

WITH_LIBRT="-lrt"
AC_CANONICAL_HOST

TCLADDLIBS=""

SNOWMIX_VERSION=`cat VERSION`
AC_SUBST(SNOWMIX_VERSION)

case $host_os in
  darwin* )
        # Do something specific for mac
	PLATFORM_OS="-DHAVE_MACOSX"
	WITH_LIBRT=""
	if test -d /opt/local ; then
	  if test -d /opt/local/include ; then
	    export CPPFLAGS='-I/opt/local/include'
	  else
	    if test -d /usr/local/include ; then
	      export CPPFLAGS='-I/usr/local/include'
	    fi
	  fi
	  if test -d /opt/local/lib ; then
	    export LDFLAGS='-L/opt/local/lib'
	  else
	    if test -d /usr/local/lib ; then
	      export LDFLAGS='-L/usr/local/lib'
	    fi
	  fi
	else
	  if test -d /usr/local/include ; then
	    export CPPFLAGS='-I/usr/local/include'
	  fi
	  if test -d /usr/local/lib ; then
	    export LDFLAGS='-L/usr/local/lib'
	  fi
	fi
	if test -d /usr/include/malloc ; then
	  export CPPFLAGS=$CPPFLAGS' -I/usr/include/malloc'
	fi
        ;;
  linux*)
        # Do something specific for linux
	PLATFORM_OS="-DHAVE_LINUX"
        if test -f /etc/os-release ; then
          osvariant=`grep '^ID=' /etc/os-release | cut -f2 -d= | tr 'A-Z' 'a-z'`
          if [[ X"$osvariant" = Xchakra ]] ; then TCLADDLIBS=" -ltcl" ; fi
        fi
        ;;
   freebsd*)
        # Do something specific for FreeBSD
        AC_MSG_WARN([Your platform may not be supported. Assuming PCBSD])
	PLATFORM_OS="-DHAVE_PCBSD -DHAVE_BSD"
        ;;
   *BSD*)
        # Do something specific for BSD
        AC_MSG_WARN([Your platform may not be supported])
	PLATFORM_OS="-DHAVE_BSD"
        ;;
    *)
        #Default Case
        AC_MSG_ERROR([Your platform is not currently supported])
	PLATFORM_OS=""
        ;;
esac

dnl Check for mman.h
AC_CHECK_HEADER(sys/mman.h,
  MMAN_H="-DHAVE_MMAN",
  MMAN_H=""
)
AC_SUBST(MMAN_H)

# SDL
dnl Get SDL cflags and SDL libraries to include
AC_CHECK_HEADER(SDL.h,
  SDL_LIBS="`sdl-config --libs` "
  SDL_CFLAGS="`sdl-config --cflags` ",
  [AC_CHECK_HEADER(SDL/SDL.h,
    SDL_LIBS="`sdl-config --libs` "
    SDL_CFLAGS="`sdl-config --cflags` ",
    [AC_MSG_ERROR([Could not find SDL.h. SDL is essential for building Snowmix.])]
  )]
)
AC_SUBST(SDL_LIBS)
AC_SUBST(SDL_CFLAGS)

# libpng16
dnl *** libpng16 ***
AC_CHECK_LIB([png16],
  [png_sig_cmp],
  HAVE_PNG="-DHAVE_PNG16"
  PNG_LIBS="`echo  $(pkg-config --libs libpng16)` "
  PNG_CFLAGS="`echo  $(pkg-config --cflags libpng16)` ",
  [AC_CHECK_LIB([png15],
    [png_sig_cmp],
    HAVE_PNG="-DHAVE_PNG15"
    PNG_LIBS="`echo  $(pkg-config --libs libpng15)` "
    PNG_CFLAGS="`echo  $(pkg-config --cflags libpng15)` ",
    [AC_CHECK_LIB([png12],
      [png_sig_cmp],
      HAVE_PNG="-DHAVE_PNG12"
      PNG_LIBS="`echo  $(pkg-config --libs libpng12)` "
      PNG_CFLAGS="`echo  $(pkg-config --cflags libpng12)` ",
      HAVE_PNG=""
      PNG_LIBS=""
      PNG_CFLAGS=""
      [AC_MSG_ERROR([Could not find libpng16, libpng15 or libpng12. libpng is essential for building Snowmix])]
    )]
  )]
)
AC_SUBST(PNG_LIBS)
AC_SUBST(PNG_CFLAGS)
AC_SUBST(HAVE_PNG)

dnl *** cairo ***
AC_CHECK_LIB([cairo],
  [cairo_create],
  CAIRO_LIBS="`echo  $(pkg-config --libs cairo)` "
  CAIRO_CFLAGS="`echo  $(pkg-config --cflags cairo)` "
  [AC_MSG_NOTICE([We have Cairo])],
  CAIRO_LIBS=""
  CAIRO_CFLAGS=""
  [AC_MSG_ERROR([Could not find libcairo. libcairo is essential for building Snowmix])]
)
AC_SUBST(CAIRO_LIBS)
AC_SUBST(CAIRO_CFLAGS)

dnl *** pangocairo ***
AC_CHECK_LIB([pangocairo],
  [pango_cairo_update_context],
  PANGO_LIBS="`echo  $(pkg-config --libs pangocairo)` "
  PANGO_CFLAGS="`echo  $(pkg-config --cflags pangocairo)` "
  [AC_MSG_NOTICE([We have pangocairo])],
  [AC_CHECK_LIB([pangocairo-1.0],
    [pango_cairo_update_context],
    PANGO_LIBS="`echo  $(pkg-config --libs pangocairo)` "
    PANGO_CFLAGS="`echo  $(pkg-config --cflags pangocairo)` "
    [AC_MSG_NOTICE([We have pangocairo])],
    PANGO_LIBS=""
    PANGO_CFLAGS=""
    [AC_MSG_ERROR([Could not find libpangocairo. libpangocairo is essential for building Snowmix])]
  )]
)
AC_SUBST(PANGO_LIBS)
AC_SUBST(PANGO_CFLAGS)

dnl *** GTK+ ***
AS_IF([test "x$snowmixgui" = "xtrue"],
  GTK_PLUS_LIBS="`echo  $( pkg-config --libs gtk+-3.0)` "
  GTK_PLUS_CFLAGS="`echo  $(pkg-config --cflags gtk+-3.0)` ",
  GTK_PLUS_LIBS=""
  GTK_PLUS_CFLAGS=""
  [AC_MSG_NOTICE([GTK+ skipped. snowmixgui is not enabled. Default])]
)
AC_SUBST(GTK_PLUS_LIBS)
AC_SUBST(GTK_PLUS_CFLAGS)

# Gstreamer
#dnl *** Gstreamer ***
#AC_CHECK_LIB([gstreamer-1.0],
#  [gst_init],
#  GSTREAMER_LIBS="`echo  $(pkg-config --libs gstreamer-1.0)` "
#  GSTREAMER_CFLAGS="`echo  $(pkg-config --cflags gstreamer-1.0)` ",
#  [AC_CHECK_LIB([gstreamer-0.10],
#    [gst_init],
#    GSTREAMER_LIBS="`echo  $(pkg-config --libs gstreamer-0.10)` "
#    GSTREAMER_CFLAGS="`echo  $(pkg-config --cflags gstreamer-0.10)` ",
#    [AC_MSG_ERROR([Could not find libgstreamer-1.0 or libgstreamer-0.10])]
#  )]
#)
#AC_SUBST(GSTREAMER_LIBS)
#AC_SUBST(GSTREAMER_CFLAGS)

# X11 for XOpenDisplay
dnl *** X11 ***
AS_IF([test "x$snowmixx11" = "xtrue"],
  [AC_MSG_NOTICE([snowmixx11 is enabled. X11 for OpenGL. Checking ...])]
  [AC_CHECK_LIB([X11],
    [XOpenDisplay],
    WITH_X11="-DHAVE_GLX"
#    WITH_OPENGL="-DHAVE_OPENGL"
    X11_LIBS="`echo  $(pkg-config --libs x11)` "
    X11_CFLAGS="`echo  $(pkg-config --cflags x11)` ",
    WITH_X11=""
    X11_LIBS=""
    X11_CFLAGS=""
    [AC_MSG_ERROR([Could not find x11 library])]
  )],
  WITH_X11=""
  X11_LIBS=""
  X11_CFLAGS=""
  [AC_MSG_NOTICE([Skipping x11. snowmixx11 is not enabled (default). See ./configure --help.])]
)
AC_SUBST(WITH_X11)
AC_SUBST(X11_LIBS)
AC_SUBST(X11_CFLAGS)

# Check GL for glXCreateContext
dnl *** GL ***
AS_IF([test "x$snowmixgl" = "xtrue"],
  [AC_MSG_NOTICE([snowmixgl is enabled. GL for OpenGL. Checking ...])]
  [AC_CHECK_LIB([GL],
    [glXCreateContext],
    WITH_GL="-DHAVE_GL"
#    WITH_OPENGL="-DHAVE_OPENGL"
    GL_LIBS="`echo  $(pkg-config --libs gl)` "
    GL_CFLAGS="`echo  $(pkg-config --cflags gl)` ",
    WITH_GL=""
    GL_LIBS=""
    GL_CFLAGS=""
    [AC_MSG_ERROR([Could not find gl library])]
  )],
  WITH_GL=""
  GL_LIBS=""
  GL_CFLAGS=""
  [AC_MSG_NOTICE([Skipping gl. snowmixgl is not enabled (default).])]
)
#AC_SUBST(WITH_GL)
AC_SUBST(GL_LIBS)
AC_SUBST(GL_CFLAGS)

# OSMesa - Off Screen Mesa (OpenGL)
dnl *** osmesa ***
AS_IF([test "x$snowmixosmesa" = "xtrue"],
  [AC_MSG_NOTICE([snowmixosmesa is enabled. Off Screen Mesa (OpenGL). Checking ...])]
  [AC_CHECK_LIB([OSMesa],
    [OSMesaCreateContext],
    WITH_OSMESA="-DHAVE_OSMESA"
#    WITH_OPENGL="-DHAVE_OPENGL"
    OSMESA_LIBS="`echo  $(pkg-config --libs osmesa)` "
    OSMESA_CFLAGS="`echo  $(pkg-config --cflags osmesa)` ",
    WITH_OSMESA=""
    OSMESA_LIBS=""
    OSMESA_CFLAGS=""
    [AC_MSG_ERROR([Could not find osmesa library])]
  )],
  WITH_OSMESA=""
  OSMESA_LIBS=""
  OSMESA_CFLAGS=""
  [AC_MSG_NOTICE([Skipping osmesa. snowmixosmesa is not enabled (default). See ./configure --help.])]
)
AC_SUBST(WITH_OSMESA)
AC_SUBST(OSMESA_LIBS)
AC_SUBST(OSMESA_CFLAGS)

## GLU - OpenGL GLU
dnl *** GLU ***
AS_IF([test "x$snowmixglu" = "xtrue"],
  [AC_MSG_NOTICE([Including GLU. snowmixglu is enabled])]
  WITH_GLU="-DHAVE_GLU"
  GLU_LIBS="`echo  $(pkg-config --libs glu)` "
  GLU_CFLAGS="`echo  $(pkg-config --cflags glu)` ",
  WITH_GLU=""
  GLU_LIBS=""
  GLU_CFLAGS=""
  [AC_MSG_NOTICE([Skipping GLU. snowmixglu is not enabled (default). See ./configure --help.])]
)
AC_SUBST(WITH_GLU)
AC_SUBST(GLU_LIBS)
AC_SUBST(GLU_CFLAGS)

AC_SUBST(WITH_OPENGL)

# Check for GLUT
AS_IF([test "x$snowmixglut" = "xtrue"],
  [AC_MSG_NOTICE([Including GLUT. snowmixglut is enabled])]
  AC_CHECK_HEADER(GL/glut.h,
	GLUT_CFLAGS=""
	GLUT_LIBS="-lglut"
	GLUT_H="-DHAVE_GLUT",
	GLUT_CFLAGS=""
	GLUT_LIBS=""
	GLUT_H=""
	[AC_MSG_WARN([glut.h not present])]
  ),
  [AC_MSG_NOTICE([Skipping GLUT. snowmixglut is not enabled (default). See ./configure --help.])]
  GLUT_CFLAGS=""
  GLUT_LIBS=""
  GLUT_H=""
)

AC_SUBST(GLUT_LIBS)
AC_SUBST(GLUT_CFLAGS)
AC_SUBST(GLUT_H)

### GLEW - OpenGL Wrangler
#dnl *** glew ***
#AS_IF([test "x$snowmixglew" = "xtrue"],
#  [AC_MSG_NOTICE([Including GLEW. snowmixglew is enabled])]
#  WITH_GLEW="-DHAVE_GLEW"
#  GLEW_LIBS="`echo  $(pkg-config --libs glew)` "
#  GLEW_CFLAGS="`echo  $(pkg-config --cflags glew)` ",
#  WITH_GLEW=""
#  GLEW_LIBS=""
#  GLEW_CFLAGS=""
#  [AC_MSG_NOTICE([Skipping GLEW. snowmixglew is not enabled (default). See ./configure --help.])]
#)
#AC_SUBST(WITH_GLEW)
#AC_SUBST(GLEW_LIBS)
#AC_SUBST(GLEW_CFLAGS)

# Check for malloc.h
AC_CHECK_HEADER(malloc.h,
	MALLOC_H="-DHAVE_MALLOC",
	MALLOC_H="-DNO_MALLOC_H"
	[AC_MSG_WARN([malloc.h not present])]
)
AC_SUBST(MALLOC_H)

dnl *** pangocairo ***
AC_CHECK_LIB([tcl8.6],
  [Tcl_CreateInterp],
  TCL_LIBS="-ltcl8.6"
  [AC_MSG_NOTICE([We have tcl8.6])],
  [AC_CHECK_LIB([tcl8.5],
    [Tcl_CreateInterp],
    TCL_LIBS="-ltcl8.5"
    [AC_MSG_NOTICE([We have tcl8.5])],
    [AC_CHECK_LIB([tcl],
      [Tcl_CreateInterp],
      TCL_LIBS="-ltcl"
      [AC_MSG_NOTICE([We have tcl, perhaps 8.5 or 8.6])],
      TCL_LIBS="",
      [AC_MSG_ERROR([Could not find libpangocairo. libpangocairo is essential for building Snowmix])]
    )]
  )]
)

# Check for tcl8.6, then tcl8.5 and then tcl (assming tcl8.5)
AC_CHECK_HEADER(tcl8.6/tcl.h,
#  TCL_LIBS="-ltcl8.6"
  TCLFLAGS="-DWITH_TCL -DHAVE_TCL"
  TCLVERSION="-DWITH_TCL86" ,
  [AC_CHECK_HEADER(tcl8.5/tcl.h,
#    TCL_LIBS="-ltcl8.5"
    TCLFLAGS="-DWITH_TCL -DHAVE_TCL"
    TCLVERSION="-DWITH_TCL85" ,
    [AC_CHECK_HEADER(tcl.h,
#      TCL_LIBS="-ltcl"
      TCLFLAGS="-DWITH_TCL -DHAVE_TCL"
      TCLVERSION="-DWITH_TCL" ,
      TCLFLAGS=""
      TCLVERSION=""
      TCL_LIBS=""
      [AC_MSG_ERROR(
        [Unable to find tcl header files, install tcl development package.] )]
    )]
  )]
)
if pkg-config --exists tcl ; then
  AC_MSG_NOTICE([Found tcl.pc for pkg-config. Using it to set TCL_LIBS.])
  tcllibs=`echo $(pkg-config --libs tcl)` 2>/dev/null
  if [[ "X$tcllibs" != X ]] ; then TCL_LIBS="$tcllibs $TCL_LIBS $TCLADDLIBS" ; fi
else
  AC_MSG_NOTICE([No tcl.pc for pkg-config. This is usually okay.])
fi
# tcllibs=`echo $(pkg-config --libs tcl)` 2>/dev/null
# if [[ "X$tcllibs" != X ]] ; then TCL_LIBS="$tcllibs $TCLADDLIBS" ; fi
AC_SUBST(TCLVERSION)
AC_SUBST(TCL_LIBS)
AC_SUBST(TCLFLAGS)
AC_SUBST(PLATFORM_OS)
AC_SUBST(WITH_LIBRT)

AC_CONFIG_FILES([Makefile
	src/Makefile
	relay/Makefile
	frames/Makefile
	images/Makefile
	images/aismaps/Makefile
	images/tcl_images/Makefile
	images/CS/Makefile
	images/GL/Makefile
	ini/Makefile
	slib/Makefile
	scripts/Makefile
	tcl/Makefile
	test/Makefile

])
AC_OUTPUT
