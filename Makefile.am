####### kdevelop will overwrite this part!!! (begin)##########

SUBDIRS = src po doc icons xslt admin 

EXTRA_DIST = AUTHORS COPYING ChangeLog INSTALL README TODO bookcase.lsm bookcase.desktop bookcase.spec bookcase.dtd x-bookcase.desktop 

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Applications/
	$(INSTALL_DATA) $(srcdir)/bookcase.desktop $(kde_appsdir)/Applications/bookcase.desktop
	$(mkinstalldirs) $(kde_datadir)/bookcase/
	$(INSTALL_DATA) $(srcdir)/bookcase.dtd $(kde_datadir)/bookcase/bookcase.dtd

uninstall-local:
	-rm -f $(kde_appsdir)/Applications/bookcase.desktop
	-rm -f $(kde_datadir)/bookcase/bookcase.dtd

####### kdevelop will overwrite this part!!! (end)############
# not a GNU package. You can remove this line, if 
# have all needed files, that a GNU package needs
#AUTOMAKE_OPTIONS = foreign 

$(top_srcdir)/configure.in: configure.in.in $(top_srcdir)/subdirs
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common configure.in ;

$(top_srcdir)/subdirs:
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common subdirs

$(top_srcdir)/acinclude.m4: $(top_srcdir)/admin/acinclude.m4.in $(top_srcdir)/admin/libtool.m4.in
	@cd $(top_srcdir) && cat admin/acinclude.m4.in admin/libtool.m4.in > acinclude.m4

mimedir = $(kde_mimedir)/application
mime_DATA = x-bookcase.desktop

MAINTAINERCLEANFILES = subdirs configure.in acinclude.m4 configure.files

DISTCLEANFILES = stamp-h2 $(distdir).tar.gz

package-messages:
	$(MAKE) -f admin/Makefile.common package-messages
	$(MAKE) -C po merge

# am_edit needs to modify the Makefile.in files, so they must be writeable
dist-hook:
	-find $(distdir) -name Makefile.in -exec chmod a+w {} \;
	cd $(distdir) && perl admin/am_edit -padmin
	cd $(distdir) && $(MAKE) -f admin/Makefile.common subdirs
# don't include dist file inside the dist file
#	cd $(srcdir) && rm -f $(distdir).tar.gz

include admin/Doxyfile.am
