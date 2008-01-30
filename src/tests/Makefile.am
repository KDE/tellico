AM_CPPFLAGS = -I$(srcdir)/.. $(all_includes)

KDE_OPTIONS = noautodist

AM_LDFLAGS = $(QT_LDFLAGS) $(KDE_LDFLAGS) $(X_LDFLAGS)

check_PROGRAMS = isbntest latin1test entitytest

check: isbntest latin1test entitytest
	./isbntest
	./latin1test
	./entitytest

METASOURCES = AUTO

DISTCLEANFILES = *~ *.Po $(CLEANFILES)

isbntest_SOURCES = isbntest.cpp
isbntest_LDADD = ../isbnvalidator.o $(LIB_QT) $(LIB_KDECORE) $(LIB_KDEUI)

latin1test_SOURCES = latin1test.cpp
latin1test_LDADD = $(LIB_QT) $(LIB_KDECORE)

entitytest_SOURCES = entitytest.cpp
entitytest_LDADD = ../tellico_utils.o $(LIB_QT) $(LIB_KDECORE)

#formattest_SOURCES = formattest.cpp
#formattest_LDADD = ../core/tellico_config.o ../core/tellico_config_addons.o ../field.o ../tellico_utils.o $(LIB_QT) $(LIB_KDECORE)
