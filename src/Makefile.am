####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = bookcase
bookcase_SOURCES = bccollectionview.cpp bcuniteditwidget.cpp bcattribute.cpp bcunit.cpp bccollection.cpp bookcasedoc.cpp bclistview.cpp bookcase.cpp main.cpp 
bookcase_LDADD   =  $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET)


EXTRA_DIST = main.cpp bookcase.cpp bookcase.h bookcaseui.rc bclistview.cpp bclistview.h bookcasedoc.cpp bookcasedoc.h bccollection.cpp bccollection.h bcunit.cpp bcunit.h bcattribute.cpp bcattribute.h bcuniteditwidget.cpp bcuniteditwidget.h bccollectionview.cpp bccollectionview.h bcunititem.h 

####### kdevelop will overwrite this part!!! (end)############
# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.

# set the include path for X, qt and KDE
INCLUDES= $(all_includes)

METASOURCES = AUTO

# the library search path. 
bookcase_LDFLAGS = $(all_libraries) $(KDE_RPATH)

rcdir = $(kde_datadir)/bookcase
rc_DATA = bookcaseui.rc

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
	 $(XGETTEXT) $$LIST -o $(podir)/bookcase.pot; \
	fi

