INCLUDES = -I.. $(all_includes)

KDE_OPTIONS = noautodist

AM_LDFLAGS = $(QT_LDFLAGS) $(KDE_LDFLAGS)

check_PROGRAMS = isbntest latin1test entitytest formattest

check: isbntest latin1test entitytest formattest
	./isbntest
	./latin1test
	./entitytest
	./formattest

METASOURCES = AUTO

DISTCLEANFILES = *~ *.Po $(CLEANFILES)

isbntest_SOURCES = isbntest.cpp
isbntest_LDADD = ../isbnvalidator.o $(LIB_QT) $(LIB_KDECORE) $(LIB_KDEUI)

latin1test_SOURCES = latin1test.cpp
latin1test_LDADD = $(LIB_QT) $(LIB_KDECORE)

entitytest_SOURCES = entitytest.cpp
entitytest_LDADD = $(LIB_QT) $(LIB_KDECORE)

formattest_SOURCES = formattest.cpp
formattest_LDADD = ../field.o $(LIB_QT) $(LIB_KDECORE)
