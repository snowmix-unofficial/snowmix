##SUBDIRS = src relay
##SUBDIRS = src frames ini scripts slib test tcl
##SUBDIRS = frames images ini slib scripts test tcl
#SUBDIRS = src relay frames images ini slib scripts test tcl
SUBDIRS = src frames images ini slib scripts test tcl

snowmix_dir = $(DESTDIR)$(libdir)/Snowmix-@SNOWMIX_VERSION@
include_HEADERS =

EXTRA_DIST = bootstrap NEWS README Changelog AUTHORS INSTALL COPYING scripts test

ACLOCAL_AMFLAGS = -I m4

install-exec-hook:
	find $(snowmix_dir) -type d -exec chmod 755 {} \;
