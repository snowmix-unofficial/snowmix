tcl_SCRIPTS = snowscene.tcl
tcl_DATA = @SNOWMIX_TCLINCFILES@
tcldir = $(libdir)/Snowmix-@SNOWMIX_VERSION@/tcl

install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(tcldir) && \
	cd $(DESTDIR)$(tcldir) && \
	$(LN_S) -f ../images/tcl_images images  && \
        $(LN_S) -f snowscene.tcl snowaudio.tcl && \
        $(LN_S) -f snowscene.tcl snowoutput.tcl && \
        $(LN_S) -f snowscene.tcl snowrelay.tcl && \
	cd $(DESTDIR)$(bindir) && \
        $(LN_S) -f ../lib/Snowmix-$(VERSION)/tcl/snowscene.tcl && \
        $(LN_S) -f ../lib/Snowmix-$(VERSION)/tcl/snowscene.tcl snowaudio.tcl && \
        $(LN_S) -f ../lib/Snowmix-$(VERSION)/tcl/snowscene.tcl snowoutput.tcl && \
        $(LN_S) -f ../lib/Snowmix-$(VERSION)/tcl/snowscene.tcl snowrelay.tcl

uninstall-hook:
	if test -h $(DESTDIR)$(bindir)/snowaudio.tcl  ; then rm $(DESTDIR)$(bindir)/snowaudio.tcl ; fi && \
	if test -h $(DESTDIR)$(bindir)/snowoutput.tcl  ; then rm $(DESTDIR)$(bindir)/snowoutput.tcl ; fi && \
	if test -h $(DESTDIR)$(bindir)/snowrelay.tcl  ; then rm $(DESTDIR)$(bindir)/snowrelay.tcl ; fi && \
	if test -h $(DESTDIR)$(bindir)/snowscene.tcl  ; then rm $(DESTDIR)$(bindir)/snowscene.tcl ; fi
