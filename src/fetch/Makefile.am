####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libfetch.a

INCLUDES = $(all_includes) $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS) $(YAZ_CFLAGS)

libfetch_a_METASOURCES = AUTO

libfetch_a_SOURCES = srufetcher.cpp fetchmanager.cpp amazonfetcher.cpp \
	z3950fetcher.cpp imdbfetcher.cpp configwidget.cpp

####### kdevelop will overwrite this part!!! (end)############

CLEANFILES = *~

KDE_OPTIONS = noautodist

EXTRA_DIST = \
fetcher.h fetchmanager.h fetchmanager.cpp srufetcher.h srufetcher.cpp \
amazonfetcher.h amazonfetcher.cpp z3950fetcher.h z3950fetcher.cpp \
imdbfetcher.h imdbfetcher.cpp fetch.h configwidget.h configwidget.cpp

