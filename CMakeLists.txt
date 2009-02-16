PROJECT(tellico)

############# Options #################

OPTION( ENABLE_AMAZON "Enable Amazon.com searching" TRUE )
OPTION( ENABLE_IMDB "Enable IMDb searching" TRUE )
OPTION( ENABLE_CDTEXT "Enable cdtext" TRUE )
OPTION( ENABLE_WEBCAM "Enable support for webcams" FALSE )
OPTION( ENABLE_SAX "Enable SAX loading" TRUE )

OPTION( ENABLE_POPPLER "Enable poppler support" TRUE )
OPTION( ENABLE_EXEMPI "Enable exempi support" TRUE )

OPTION( ENABLE_LIBKCAL "Enable libkcal support" FALSE )
OPTION( ENABLE_LIBKCDDB "Enable libkcddb support" TRUE )

FIND_PACKAGE(PkgConfig REQUIRED)
FIND_PACKAGE(Gettext REQUIRED)

FIND_PACKAGE(KDE4 REQUIRED)
FIND_PACKAGE(QImageBlitz REQUIRED)
FIND_PACKAGE(KdepimLibs REQUIRED)

FIND_PACKAGE(KdeMultimedia)

FIND_PACKAGE(LibXml2 REQUIRED)
FIND_PACKAGE(LibXslt REQUIRED)

INCLUDE(KDE4Defaults)
INCLUDE(MacroLibrary)
INCLUDE(CheckTypeSize)

PKG_CHECK_MODULES(TAGLIB taglib)
PKG_CHECK_MODULES(YAZ yaz yaz>=2.0)

IF ( ENABLE_POPPLER )
FIND_PACKAGE(PopplerQt4)
ENDIF ( ENABLE_POPPLER )

IF ( ENABLE_EXEMPI )
PKG_CHECK_MODULES(EXEMPI exempi-2.0>=1.99.0)
ENDIF ( ENABLE_EXEMPI )

CHECK_TYPE_SIZE("unsigned char" SIZEOF_UNSIGNED_CHAR)
CHECK_TYPE_SIZE("unsigned short" SIZEOF_UNSIGNED_SHORT)
CHECK_TYPE_SIZE("unsigned long" SIZEOF_UNSIGNED_LONG)
CHECK_TYPE_SIZE("unsigned int" SIZEOF_UNSIGNED_INT)
CHECK_TYPE_SIZE("short" SIZEOF_SHORT)
CHECK_TYPE_SIZE("long" SIZEOF_LONG)
CHECK_TYPE_SIZE("int" SIZEOF_INT)

INCLUDE( CheckSymbolExists )
CHECK_SYMBOL_EXISTS(strlwr "string.h" HAVE_STRLWR)
CHECK_SYMBOL_EXISTS(strupr "string.h" HAVE_STRUPR)

IF( NOT WIN32 )
SET( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wextra -Wundef" )
ENDIF( NOT WIN32 )

ADD_DEFINITIONS( ${QT_DEFINITIONS} ${KDE4_DEFINITIONS} )
#ADD_DEFINITIONS( -DQT3_SUPPORT -DQT3_SUPPORT_WARNINGS )
ADD_DEFINITIONS( -DQT_STRICT_ITERATORS )
REMOVE_DEFINITIONS( -DQT_NO_STL )

ADD_DEFINITIONS( ${TAGLIB_CFLAGS} )
ADD_DEFINITIONS( ${EXEMPI_CFLAGS} )
ADD_DEFINITIONS( ${YAZ_CFLAGS} )

LINK_DIRECTORIES( ${KDE4_LIB_DIR} )

INCLUDE_DIRECTORIES( ${KDE4_INCLUDES} ${QT_INCLUDES} )
INCLUDE_DIRECTORIES( ${LIBXML2_INCLUDE_DIR} ${LIBXSLT_INCLUDE_DIR} )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_BINARY_DIR} )
INCLUDE_DIRECTORIES( ${CMAKE_SOURCE_DIR}/src/core )

SET( TELLICO_DATA_INSTALL_DIR ${DATA_INSTALL_DIR}/tellico )

ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( po )
ADD_SUBDIRECTORY( doc )
ADD_SUBDIRECTORY( icons )
ADD_SUBDIRECTORY( xslt )

IF( NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD|Linux|kFreeBSD|NetBSD|OpenBSD|SunOS|Darwin" )
MESSAGE("WARNING: CDDB Lookup is not supported on your platform")
ENDIF( NOT CMAKE_SYSTEM_NAME MATCHES "FreeBSD|Linux|kFreeBSD|NetBSD|OpenBSD|SunOS|Darwin" )

########## Wrap tests results around the tests done within the source
IF( TAGLIB_FOUND )
SET( HAVE_TAGLIB TRUE )
ENDIF( TAGLIB_FOUND )

IF( POPPLER_QT4_FOUND )
SET( HAVE_POPPLER TRUE )
ADD_DEFINITIONS( ${POPPLER_QT4_DEFINITIONS} )
INCLUDE_DIRECTORIES( ${POPPLER_QT4_INCLUDE_DIR} )
ENDIF( POPPLER_QT4_FOUND )

IF( EXEMPI_FOUND )
SET( HAVE_EXEMPI TRUE )
ENDIF( EXEMPI_FOUND )

IF( KDEPIMLIBS_FOUND )
IF( ENABLE_LIBKCAL )
SET( HAVE_KCAL TRUE )
ENDIF( ENABLE_LIBKCAL )
ENDIF( KDEPIMLIBS_FOUND )

IF( KDEMULTIMEDIA_FOUND )
IF( ENABLE_LIBKCDDB )
SET( HAVE_KCDDB TRUE )
ENDIF( ENABLE_LIBKCDDB )
ENDIF( KDEMULTIMEDIA_FOUND )

IF( YAZ_FOUND )
SET( HAVE_YAZ TRUE )
ENDIF( YAZ_FOUND )

########### CPack stuff ###############
SET(CPACK_PACKAGE_VERSION "1.9")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "tellico-${CPACK_PACKAGE_VERSION}")

SET(CPACK_GENERATOR "TGZ")
SET(CPACK_SOURCE_GENERATOR "TGZ")

SET(CPACK_SOURCE_IGNORE_FILES "/builds/;svn.motd;Makefile;/debian/;/\\\\.svn/;\\\\.swp$;\\\\.#;/#")

INCLUDE(CPack)
########################################

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

########### install files ###############

INSTALL(FILES tellico.dtd tellico.tips DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
INSTALL(FILES tellico.desktop DESTINATION ${XDG_APPS_INSTALL_DIR} )
INSTALL(FILES tellico.xml DESTINATION  ${XDG_MIME_INSTALL_DIR} )
INSTALL(FILES x-tellico.desktop DESTINATION ${MIME_INSTALL_DIR}/application )



#original Makefile.am contents follow:

#package-messages:
#	$(MAKE) -f admin/Makefile.common package-messages
#	$(MAKE) -C po merge
#
#messages:
#	@ rm -f tips.cpp xslt.cpp z3950.cpp scripts.cpp
#	@ touch $(podir)/tellico.pot;
#	./preparetips tellico.tips > tips.cpp; \
#	$(XGETTEXT)  --join-existing tips.cpp -o $(podir)/tellico.pot; \
#	./prepare_i18n_xslt > xslt.cpp; \
#	$(XGETTEXT)  --join-existing xslt.cpp -o $(podir)/tellico.pot; \
#	./prepare_desktop src/fetch/z3950-servers.cfg > z3950.cpp; \
#	$(XGETTEXT)  --join-existing z3950.cpp -o $(podir)/tellico.pot; \
#	./prepare_desktop src/fetch/scripts/*.spec > scripts.cpp; \
#	$(XGETTEXT)  --join-existing scripts.cpp -o $(podir)/tellico.pot;
#	@ rm -f tips.cpp xslt.cpp z3950.cpp scripts.cpp
#
#
#dist-hook:
## am_edit needs to modify the Makefile.in files, so they must be writeable
#	-find $(distdir) -exec chmod a+w {} \;
#	cd $(distdir) && perl admin/am_edit -padmin
#	cd $(distdir) && $(MAKE) -f admin/Makefile.common subdirs
## don't include dist file inside the dist file
##	cd $(srcdir) && rm -f $(distdir).tar.gz
## remove any executable
#	rm -rf $(distdir)/src/tellico
#
#PUBLIC_ID=-//Robby Stephenson/DTD Tellico V9.0//EN
#SYSTEM_ID=http://www.periapsis.org/tellico/dtd/v9/tellico.dtd
#
#install-data-local:
#	@if test x"@XML_CATALOG_FILE@" != x; then \
#          echo @XMLCATALOG@ --noout --add "delegatePublic" \
#               "$(PUBLIC_ID)" \
#               "file://$(localdir)/tellico.dtd" "@XML_CATALOG_FILE@" ; \
#          @XMLCATALOG@ --noout --add "delegatePublic" \
#               "$(PUBLIC_ID)" \
#               "file://$(localdir)/tellico.dtd" "@XML_CATALOG_FILE@" ; \
#          echo @XMLCATALOG@ --noout --add "delegateSystem" \
#               "$(SYSTEM_ID)" \
#               "file://$(localdir)/tellico.dtd" "@XML_CATALOG_FILE@" ; \
#          @XMLCATALOG@ --noout --add "delegateSystem" \
#               "$(SYSTEM_ID)" \
#               "file://$(localdir)/tellico.dtd" "@XML_CATALOG_FILE@" ; \
#          echo @XMLCATALOG@ --noout --add "delegateURI" \
#               "$(SYSTEM_ID)" \
#               "file://$(localdir)/tellico.dtd" "@XML_CATALOG_FILE@" ; \
#          @XMLCATALOG@ --noout --add "delegateURI" \
#               "$(SYSTEM_ID)" \
#               "file://$(localdir)/tellico.dtd" "@XML_CATALOG_FILE@" ; \
#	fi
#
#uninstall-local:
#	@if test x"@XML_CATALOG_FILE@" != x; then \
#          echo @XMLCATALOG@ --noout --del \
#               "$(PUBLIC_ID)" "@XML_CATALOG_FILE@" ; \
#          @XMLCATALOG@ --noout --del \
#               "$(PUBLIC_ID)" "@XML_CATALOG_FILE@" ; \
#          echo @XMLCATALOG@ --noout --del \
#               "$(SYSTEM_ID)" "@XML_CATALOG_FILE@" ; \
#          @XMLCATALOG@ --noout --del \
#               "$(SYSTEM_ID)" "@XML_CATALOG_FILE@" ; \
#          echo @XMLCATALOG@ --noout --del \
#               "$(SYSTEM_ID)" "@XML_CATALOG_FILE@" ; \
#          @XMLCATALOG@ --noout --del \
#               "$(SYSTEM_ID)" "@XML_CATALOG_FILE@" ; \
#	fi
#
## probably a better way to do this
#uninstall-hook:
#	-if [ -d $(localdir) ]; then rmdir $(localdir); fi
