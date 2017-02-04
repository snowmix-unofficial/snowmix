# Makefile for scripts
#
# Copyright (c) Peter Maersk-Moller 2015. All rights reserved
#
# Se file LICENSE for information about distributing Snowmix

scripts_SCRIPTS = av_output2dummy av_output2screen av_output2tcp_server
scripts_SCRIPTS += gstreamer-settings input2feed
scripts_SCRIPTS += output2dummy output2file output2rtp output2screen output2screen-auto
scripts_SCRIPTS += rtsp2feed rtsp2feed2 snowmix-settings v4l2input2feed
scriptsdir = $(libdir)/Snowmix-@SNOWMIX_VERSION@/scripts

demo_SCRIPTS = demo
demodir = $(libdir)/Snowmix-@SNOWMIX_VERSION@/scripts

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	for file in $(scripts_SCRIPTS) ; do $(LN_S) -f ../lib/Snowmix-$(VERSION)/scripts/$$file ; done && \
	$(LN_S) -f ../lib/Snowmix-$(VERSION)/scripts/demo snowmix_demo

uninstall-hook:
	cd $(DESTDIR)$(libdir)/Snowmix-$(VERSION)/scripts && \
	echo "Removing symbolic links. Please answer yes to each file for removal" && \
	for file in $(scripts_SCRIPTS) snowmix_demo ; do if test -h $(DESTDIR)$(bindir)/$$file  ; then rm $(DESTDIR)$(bindir)/$$file ; fi ; done
