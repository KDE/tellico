METASOURCES = AUTO

KDE_CXXFLAGS = $(USE_EXCEPTIONS)
KDE_OPTIONS = noautodist

COMID=gcc3
CPPULIB=-luno_cppu
CPPUHELPERLIB=-luno_cppuhelper$(COMID)
SALLIB=-luno_sal
SALHELPERLIB=-luno_salhelper$(COMID)
# REGLIB=-lreg

# Where the UNO includes will be generated
INCDIR = $(srcdir)/.include
UNODIR = $(INCDIR)/uno

# OpenOffice.org additional includes and libraries
# might have to be adjusted for other architectures
OFFICE_includes  = -I$(INCDIR) -I$(UNODIR) -DUNX -DGCC -DLINUX -DCPPU_ENV=$(COMID) -DOSL_DEBUG_LEVEL=0
OFFICE_libraries = $(CPPULIB) $(CPPUHELPERLIB) $(SALLIB) $(SALHELPERLIB)

AM_CPPFLAGS = $(all_includes) $(OFFICE_SDK_includes) $(OFFICE_includes)

kde_module_LTLIBRARIES = tellico_ooo.la

tellico_ooo_la_SOURCES = ooohandler.cpp interface.cpp
tellico_ooo_la_LDFLAGS = -module $(KDE_PLUGIN) $(KDE_RPATH) \
	$(all_libraries) $(OFFICE_libraries)
tellico_ooo_la_LIBADD = $(OFFICE_libadd)

EXTRA_DIST = ooohandler.h ooohandler.cpp \
	interface.h interface.cpp

CLEANFILES = *~ *.loT

# Clean target for the generated stuff
clean-local:
	rm -rf $(UNODIR) $(INCDIR) $(CLEANFILES)

UNOTYPES := \
	com.sun.star.uno.XComponentContext \
	com.sun.star.lang.XMultiServiceFactory \
	com.sun.star.lang.XSingleComponentFactory \
	com.sun.star.lang.XComponent \
	com.sun.star.lang.XServiceInfo \
	com.sun.star.bridge.XUnoUrlResolver \
	com.sun.star.frame.XDesktop \
	com.sun.star.frame.XComponentLoader \
	com.sun.star.text.ControlCharacter \
	com.sun.star.text.XDocumentIndexesSupplier \
	com.sun.star.text.XDocumentIndex \
	com.sun.star.text.XTextDocument \
	com.sun.star.text.XTextField \
	com.sun.star.text.XTextViewCursor \
	com.sun.star.text.XTextViewCursorSupplier \
	com.sun.star.text.BibliographyDataType \
	com.sun.star.container.XIndexAccess \
	com.sun.star.container.XHierarchicalNameAccess \
	com.sun.star.registry.XSimpleRegistry \
	com.sun.star.beans.XPropertySet \
	com.sun.star.sdbc.XRow \
	com.sun.star.sdbc.XRowSet \
	com.sun.star.sdbc.XResultSetMetaDataSupplier \
	com.sun.star.sdbc.XResultSetUpdate \
	com.sun.star.sdbc.XRowUpdate \
	com.sun.star.sdbc.SQLException \
	com.sun.star.sdb.CommandType \
	com.sun.star.document.XEventListener \
	com.sun.star.document.XEventBroadcaster \
	com.sun.star.uno.XWeak \
	com.sun.star.uno.XAggregation \
	com.sun.star.lang.XTypeProvider


UNOHPPFILES = $(foreach t,$(UNOTYPES),$(UNODIR)/$(subst .,/,$(t)).hpp)

ooohandler.cpp: $(UNOHPPFILES) $(INCDIR)

$(INCDIR):
	mkdir -p $(INCDIR)

$(UNODIR):
	mkdir -p $(UNODIR)

$(UNOHPPFILES): $(UNODIR)
	$(CPPUMAKER) -Gc -BUCR -O$(UNODIR) $(foreach t,$(UNOTYPES),-T$(t)) \
		$(OFFICE_registry)

