####### kdevelop will overwrite this part!!! (begin)##########

SUBDIRS = src po doc icons xslt admin 

EXTRA_DIST = COPYING ChangeLog INSTALL README TODO bookcase.lsm bookcase.desktop bookcase.spec bookcase.dtd x-bookcase.desktop configure.in.in AUTHORS 

####### kdevelop will overwrite this part!!! (end)############
# not a GNU package. You can remove this line, if 
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign

KDE_OPTIONS = noautodist

$(top_srcdir)/configure.in: configure.in.in $(top_srcdir)/subdirs
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common configure.in ;

$(top_srcdir)/subdirs:
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common subdirs

$(top_srcdir)/acinclude.m4: $(top_srcdir)/admin/acinclude.m4.in $(top_srcdir)/admin/libtool.m4.in
	@cd $(top_srcdir) && cat admin/acinclude.m4.in admin/libtool.m4.in > acinclude.m4

appdir = $(kde_appsdir)/Applications/
app_DATA = bookcase.desktop

mimedir = $(kde_mimedir)/application
mime_DATA = x-bookcase.desktop

datadir = $(kde_datadir)/bookcase
data_DATA = bookcase.dtd

MAINTAINERCLEANFILES = subdirs configure.in acinclude.m4 configure.files Makefile.in

DISTCLEANFILES = stamp-h2 $(distdir).tar.gz autom4te-2.5x.cache

package-messages:
	$(MAKE) -f admin/Makefile.common package-messages
	$(MAKE) -C po merge

dist-hook:
# am_edit needs to modify the Makefile.in files, so they must be writeable
	-find $(distdir) -name Makefile.in -exec chmod a+w {} \;
	cd $(distdir) && perl admin/am_edit -padmin
	cd $(distdir) && $(MAKE) -f admin/Makefile.common subdirs
# don't include dist file inside the dist file
#	cd $(srcdir) && rm -f $(distdir).tar.gz
# remove any executable
	rm -rf $(distdir)/src/bookcase

include admin/Doxyfile.am
