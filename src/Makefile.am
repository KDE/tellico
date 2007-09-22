####### kdevelop will overwrite this part!!! (begin)##########
bin_PROGRAMS = tellico

tellico_SOURCES = borrowerdialog.cpp entryitem.cpp importdialog.cpp \
				borroweritem.cpp entryview.cpp isbnvalidator.cpp exportdialog.cpp collection.cpp \
				fetchdialog.cpp collectionfactory.cpp fieldcompletion.cpp collectionfieldsdialog.cpp \
				field.cpp main.cpp configdialog.cpp mainwindow.cpp controller.cpp filehandler.cpp \
				filter.cpp filterdialog.cpp reportdialog.cpp filteritem.cpp \
				detailedlistview.cpp document.cpp tellico_kernel.cpp entry.cpp groupview.cpp \
				tellico_strings.cpp entryeditdialog.cpp image.cpp tellico_utils.cpp entrygroupitem.cpp \
				imagefactory.cpp viewstack.cpp entryiconview.cpp borrower.cpp loandialog.cpp \
				loanview.cpp calendarhandler.cpp loanitem.cpp groupiterator.cpp filterview.cpp \
				entryiconfactory.cpp entryupdater.cpp detailedentryitem.cpp statusbar.cpp \
				progressmanager.cpp upcvalidator.cpp fetcherconfigdialog.cpp iso5426converter.cpp \
				iso6937converter.cpp listviewcomparison.cpp

if !USE_LIBBTPARSE
  LDADD_LIBBTPARSE = ./translators/btparse/libbtparse.a
endif

tellico_LDADD = ./core/libcore.a ./cite/libcite.a ./fetch/libfetch.a \
	./collections/libcollections.a ./translators/libtranslators.a \
	./newstuff/libnewstuff.a ./translators/pilotdb/libpilotdb.a \
	./translators/pilotdb/libflatfile/liblibflatfile.a ./gui/libgui.a ./translators/pilotdb/libpalm/liblibpalm.a \
	./commands/libcommands.a -lexslt $(LIB_KFILE) $(LIB_KHTML) $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) \
	$(LIBSOCKET) $(LIBXSLT_LIBS) $(TAGLIB_LIBS) $(KCDDB_LIBS) $(YAZ_LIBS) $(LIB_KIO) \
	$(LIB_KABC) $(KCAL_LIBS) $(LDADD_LIBBTPARSE) $(LIB_KNEWSTUFF)

SUBDIRS = core gui collections translators fetch tests commands cite newstuff

EXTRA_DIST = tellicoui.rc tellicorc \
borrowerdialog.cpp          entryiconview.h\
borrowerdialog.h            entryitem.cpp        importdialog.cpp \
borrower.h                  entryitem.h          importdialog.h \
borroweritem.cpp            entryview.cpp        isbnvalidator.cpp \
borroweritem.h              entryview.h          isbnvalidator.h \
loanview.cpp                exportdialog.cpp     \
loanview.h                  exportdialog.h       \
collection.cpp              fetchdialog.cpp      \
collectionfactory.cpp       fetchdialog.h        \
collectionfactory.h         fieldcompletion.cpp  latin1literal.h \
collectionfieldsdialog.h    collectionfieldsdialog.cpp  fieldcompletion.h    \
collection.h                field.h              main.cpp \
configdialog.cpp            mainwindow.cpp \
configdialog.h              mainwindow.h \
controller.cpp              filehandler.cpp      \
controller.h                filehandler.h        \
filter.cpp    				 filterdialog.cpp \
filterdialog.h      		 reportdialog.cpp \
filter.h           			  reportdialog.h \
filteritem.cpp       \
filteritem.h         \
detailedlistview.h 			detailedlistview.cpp \
document.cpp                tellico_kernel.cpp \
document.h                  tellico_kernel.h \
entry.cpp                   groupview.cpp        tellico_strings.cpp \
entryeditdialog.cpp         groupview.h          tellico_strings.h \
entryeditdialog.h           image.cpp            tellico_utils.cpp \
entrygroupitem.cpp          imagefactory.cpp     tellico_utils.h \
entrygroupitem.h            imagefactory.h       viewstack.cpp \
entry.h                     image.h              viewstack.h \
entryiconview.cpp           loandialog.h \
loandialog.cpp ptrvector.h borrower.cpp datavectors.h \
calendarhandler.h calendarhandler.cpp \
loanitem.h loanitem.cpp groupiterator.h groupiterator.cpp \
stringset.h observer.h filterview.h filterview.cpp \
entryiconfactory.h entryiconfactory.cpp \
tellico_debug.h \
entryupdater.h entryupdater.cpp \
detailedentryitem.h detailedentryitem.cpp \
statusbar.h statusbar.cpp \
progressmanager.h progressmanager.cpp \
upcvalidator.h upcvalidator.cpp \
fetcherconfigdialog.h fetcherconfigdialog.cpp \
iso5426converter.h iso5426converter.cpp \
iso6937converter.h iso6937converter.cpp \
tellico_map.h \
listviewcomparison.h listviewcomparison.cpp

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
AM_CPPFLAGS = $(all_includes) $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS)

METASOURCES = AUTO

KDE_OPTIONS = noautodist

CLEANFILES = *~ *.loT

# the library search path.
tellico_LDFLAGS = $(all_libraries) $(KDE_RPATH)

uidir = $(kde_datadir)/tellico
ui_DATA = tellicoui.rc

rcdir = $(kde_confdir)
rc_DATA = tellicorc

messages: rc.cpp
	@ touch $(podir)/tellico.pot;
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
        if test -n "$$LIST"; then \
          $(XGETTEXT) --join-existing --add-comments="TRANSLATORS:" $$LIST -o $(podir)/tellico.pot; \
        fi

#include ../admin/Doxyfile.am
