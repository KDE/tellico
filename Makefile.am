####### kdevelop will overwrite this part!!! (begin)##########

SUBDIRS = src po doc icons xslt admin

EXTRA_DIST = COPYING ChangeLog INSTALL README TODO tellico.lsm tellico.spec tellico.dtd x-tellico.desktop configure.in.in AUTHORS NEWS tellico.tips tellico.desktop tellico-rename.upd

####### kdevelop will overwrite this part!!! (end)############
# not a GNU package. You can remove this line, if 
# have all needed files, that a GNU package needs
AUTOMAKE_OPTIONS = foreign

KDE_OPTIONS = noautodist

#$(top_srcdir)/configure.in: configure.in.in $(top_srcdir)/subdirs
$(top_srcdir)/configure.in: configure.in.in
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common subdirs
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common configure.in

$(top_srcdir)/subdirs:
	cd $(top_srcdir) && $(MAKE) -f admin/Makefile.common subdirs

$(top_srcdir)/acinclude.m4: $(top_srcdir)/admin/acinclude.m4.in $(top_srcdir)/admin/libtool.m4.in
	@cd $(top_srcdir) && cat admin/acinclude.m4.in admin/libtool.m4.in > acinclude.m4

appdir = $(kde_appsdir)/Applications/
app_DATA = tellico.desktop

mimedir = $(kde_mimedir)/application
mime_DATA = x-tellico.desktop

localdir = $(kde_datadir)/tellico
local_DATA = tellico.dtd tellico.tips

updatedir = $(kde_datadir)/kconf_update
update_DATA = tellico-rename.upd

CLEANFILES = *~ *.loT

DISTCLEANFILES = stamp-h1 stamp-h.in stamp-h2 $(distdir).tar.gz $(CLEANFILES)

MAINTAINERCLEANFILES = subdirs configure configure.in aclocal.m4 acinclude.m4 configure.files Makefile.in $(DISTCLEANFILES)


package-messages:
	$(MAKE) -f admin/Makefile.common package-messages
	$(MAKE) -C po merge

messages:
	rm -f tips.cpp; \
	$(PREPARETIPS) < tellico.tips > tips.cpp; \
	$(XGETTEXT) tips.cpp --join-existing -o $(podir)/tellico.pot; \
	rm -f tips.cpp

dist-hook:
# am_edit needs to modify the Makefile.in files, so they must be writeable
	-find $(distdir) -exec chmod a+w {} \;
	cd $(distdir) && perl admin/am_edit -padmin
	cd $(distdir) && $(MAKE) -f admin/Makefile.common subdirs
# don't include dist file inside the dist file
#	cd $(srcdir) && rm -f $(distdir).tar.gz
# remove any executable
	rm -rf $(distdir)/src/tellico

PUBLIC_ID=-//Robby Stephenson/DTD Bookcase V6.0//EN
SYSTEM_ID=http://www.periapsis.org/bookcase/dtd/v6/bookcase.dtd

install-data-local:
	@if test x"@XML_CATALOG_FILE@" != x; then \
          echo @XMLCATALOG@ --noout --add "delegatePublic" \
               "$(PUBLIC_ID)" \
               "file://$(localdir)/bookcase.dtd" "@XML_CATALOG_FILE@" ; \
          @XMLCATALOG@ --noout --add "delegatePublic" \
               "$(PUBLIC_ID)" \
               "file://$(localdir)/bookcase.dtd" "@XML_CATALOG_FILE@" ; \
          echo @XMLCATALOG@ --noout --add "delegateSystem" \
               "$(SYSTEM_ID)" \
               "file://$(localdir)/bookcase.dtd" "@XML_CATALOG_FILE@" ; \
          @XMLCATALOG@ --noout --add "delegateSystem" \
               "$(SYSTEM_ID)" \
               "file://$(localdir)/bookcase.dtd" "@XML_CATALOG_FILE@" ; \
          echo @XMLCATALOG@ --noout --add "delegateURI" \
               "$(SYSTEM_ID)" \
               "file://$(localdir)/bookcase.dtd" "@XML_CATALOG_FILE@" ; \
          @XMLCATALOG@ --noout --add "delegateURI" \
               "$(SYSTEM_ID)" \
               "file://$(localdir)/bookcase.dtd" "@XML_CATALOG_FILE@" ; \
	fi

uninstall-local:
	@if test x"@XML_CATALOG_FILE@" != x; then \
          echo @XMLCATALOG@ --noout --del \
               "$(PUBLIC_ID)" "@XML_CATALOG_FILE@" ; \
          @XMLCATALOG@ --noout --del \
               "$(PUBLIC_ID)" "@XML_CATALOG_FILE@" ; \
          echo @XMLCATALOG@ --noout --del \
               "$(SYSTEM_ID)" "@XML_CATALOG_FILE@" ; \
          @XMLCATALOG@ --noout --del \
               "$(SYSTEM_ID)" "@XML_CATALOG_FILE@" ; \
          echo @XMLCATALOG@ --noout --del \
               "$(SYSTEM_ID)" "@XML_CATALOG_FILE@" ; \
          @XMLCATALOG@ --noout --del \
               "$(SYSTEM_ID)" "@XML_CATALOG_FILE@" ; \
	fi

# probably a better way to do this
uninstall-hook:
	-if [ -d $(localdir) ]; then rmdir $(localdir); fi
