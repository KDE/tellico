####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = bookcase

## INCLUDES were found outside kdevelop specific part

bookcase_SOURCES = \
entryview.cpp collection.cpp mainwindow.cpp \
controller.cpp kwidgetlister.cpp isbnvalidator.cpp \
finddialog.cpp configdialog.cpp main.cpp image.cpp \
collectionfactory.cpp collectionfieldsdialog.cpp \
detailedlistview.cpp document.cpp entry.cpp \
entryitem.cpp exportdialog.cpp field.cpp fieldwidget.cpp \
filehandler.cpp filter.cpp filterdialog.cpp groupview.cpp \
importdialog.cpp labelaction.cpp tabcontrol.cpp \
imagewidget.cpp fieldcompletion.cpp imagefactory.cpp \
stringmapdialog.cpp entryeditdialog.cpp fetchdialog.cpp \
kernel.cpp viewstack.cpp entryiconview.cpp \
datewidget.cpp multiselectionlistview.cpp

bookcase_LDADD = ./fetch/libfetch.a ./collections/libcollections.a \
        ./translators/libtranslators.a ./translators/pilotdb/libpilotdb.a \
        ./translators/pilotdb/libflatfile/liblibflatfile.a \
        ./translators/pilotdb/libpalm/liblibpalm.a -lkdeprint -lexslt \
        $(LIB_KFILE) $(LIB_KHTML) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) $(LIBSOCKET) \
        $(LIBXSLT_LIBS) $(TAGLIB_LIBS) $(KCDDB_LIBS) $(CDDA_LIBS)

if !USE_LIBBTPARSE
  bookcase_LDADD += ./translators/btparse/libbtparse.a
endif

SUBDIRS = collections translators fetch tests

EXTRA_DIST = bookcaseui.rc \
collection.cpp              entryview.h          imagefactory.h \
collectionfactory.cpp       exportdialog.cpp     image.h \
collectionfactory.h         exportdialog.h       imagewidget.cpp \
collectionfieldsdialog.cpp  fieldcompletion.cpp  imagewidget.h \
collectionfieldsdialog.h    fieldcompletion.h    importdialog.cpp \
collection.h                field.cpp            importdialog.h \
configdialog.cpp            field.h              isbnvalidator.cpp \
configdialog.h              fieldwidget.cpp      isbnvalidator.h \
controller.cpp              fieldwidget.h        kwidgetlister.cpp \
controller.h                filehandler.cpp      kwidgetlister.h \
detailedlistview.cpp        filehandler.h        labelaction.cpp \
detailedlistview.h          filter.cpp           labelaction.h \
document.cpp                filterdialog.cpp     main.cpp \
document.h                  filterdialog.h       mainwindow.cpp \
entry.cpp                   filter.h             mainwindow.h \
entryeditdialog.cpp         finddialog.cpp       stringmapdialog.cpp \
entryeditdialog.h           finddialog.h         stringmapdialog.h \
entry.h                     groupview.cpp        latin1literal.h \
entryitem.cpp               groupview.h          tabcontrol.cpp \
entryitem.h                 image.cpp            tabcontrol.h \
entryview.cpp               imagefactory.cpp     utils.h \
fetchdialog.h               fetchdialog.cpp      kernel.h \
kernel.cpp                  viewstack.h          viewstack.cpp \
entryiconview.h             entryiconview.cpp    datewidget.h \
datewidget.cpp              multiselectionlistview.h \
multiselectionlistview.cpp

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

KDE_OPTIONS = noautodist

CLEANFILES = *~ *.loT

# the library search path.
bookcase_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(LIBXML_RPATH) $(LIBXSLT_RPATH)

rcdir = $(kde_datadir)/bookcase
rc_DATA = bookcaseui.rc

messages: rc.cpp
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
          $(XGETTEXT) $$LIST -o $(podir)/bookcase.pot; \
        fi

#include ../admin/Doxyfile.am
