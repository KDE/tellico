####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = bookcase

## INCLUDES were found outside kdevelop specific part

bookcase_SOURCES = bcunititem.cpp songcollection.cpp videocollection.cpp bookcollection.cpp xslthandler.cpp bclabelaction.cpp isbnvalidator.cpp finddialog.cpp configdialog.cpp bcuniteditwidget.cpp bcunit.cpp bctabcontrol.cpp bcdetailedlistview.cpp bcgroupview.cpp bccollection.cpp bcattribute.cpp bookcasedoc.cpp bookcase.cpp main.cpp 
bookcase_LDADD   = -lkdeprint -lxml2 -lxslt -lexslt $(LIB_KFILE) $(LIB_KHTML) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)


EXTRA_DIST = main.cpp bookcase.cpp bookcase.h bookcasedoc.cpp bookcasedoc.h bookcaseui.rc bcattribute.cpp bcattribute.h bccollection.cpp bccollection.h bcgroupview.cpp bcgroupview.h bcdetailedlistview.cpp bcdetailedlistview.h bctabcontrol.cpp bctabcontrol.h bcunit.cpp bcuniteditwidget.cpp bcuniteditwidget.h bcunit.h bcunititem.h configdialog.cpp configdialog.h finddialog.cpp finddialog.h isbnvalidator.cpp isbnvalidator.h bcunitgroup.h bclabelaction.cpp bclabelaction.h xslthandler.cpp xslthandler.h bookcollection.cpp bookcollection.h videocollection.cpp videocollection.h songcollection.cpp songcollection.h bcutils.h bcunititem.cpp 

####### kdevelop will overwrite this part!!! (end)############
# These paths are KDE specific. Use them:
# kde_appsdir         Where your application's menu entry (.desktop) should go to.
# kde_icondir         Where your icon should go to - better use KDE_ICON.
# kde_sounddir        Where your sounds should go to.
# kde_htmldir         Where your docs should go to. (contains lang subdirs)
# kde_datadir         Where you install application data. (Use a subdir)
# kde_locale          Where translation files should go to. (contains lang subdirs)
# kde_cgidir          Where cgi-bin executables should go to.
# kde_confdir         Where config files should go to (system-wide ones with default values).
# kde_mimedir         Where mimetypes .desktop files should go to.
# kde_servicesdir     Where services .desktop files should go to.
# kde_servicetypesdir Where servicetypes .desktop files should go to.
# kde_toolbardir      Where general toolbar icons should go to (deprecated, use KDE_ICON).
# kde_wallpaperdir    Where general wallpapers should go to.
# kde_templatesdir    Where templates for the "New" menu (Konqueror/KDesktop) should go to.
# kde_bindir          Where executables should go to. Use bin_PROGRAMS or bin_SCRIPTS.
# kde_libdir          Where shared libraries should go to. Use lib_LTLIBRARIES.
# kde_moduledir       Where modules (e.g. parts) should go to. Use kde_module_LTLIBRARIES.
# kde_styledir        Where Qt/KDE widget styles should go to (new in KDE 3).
# kde_designerdir     Where Qt Designer plugins should go to (new in KDE 3).

# set the include path for X, qt and KDE
INCLUDES= $(all_includes) $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS)

METASOURCES = AUTO

# the library search path. 
bookcase_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIBXML_RPATH) $(LIBXSLT_RPATH)

rcdir = $(kde_datadir)/bookcase
rc_DATA = bookcaseui.rc

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/bookcase.pot; \
	fi

include ../admin/Doxyfile.am
