INCLUDES = -I.. $(all_includes)

KDE_OPTIONS = noautodist

AM_LDFLAGS = $(QT_LDFLAGS) $(KDE_LDFLAGS)

check_PROGRAMS = isbntest latin1test

check: isbntest latin1test
	./isbntest
	./latin1test

METASOURCES = AUTO

DISTCLEANFILES = *~ *.Po $(CLEANFILES)

isbntest_SOURCES = isbntest.cpp
isbntest_LDADD = ../isbnvalidator.o $(LIB_QT) $(LIB_KDECORE)

latin1test_SOURCES = latin1test.cpp
latin1test_LDADD = $(LIB_QT) $(LIB_KDECORE)
