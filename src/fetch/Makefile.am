####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libfetch.a

INCLUDES = $(all_includes) $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS) $(YAZ_CFLAGS)

libfetch_a_METASOURCES = AUTO

libfetch_a_SOURCES = \
	fetchmanager.cpp amazonfetcher.cpp z3950fetcher.cpp \
	imdbfetcher.cpp configwidget.cpp entrezfetcher.cpp \
	execexternalfetcher.cpp

####### kdevelop will overwrite this part!!! (end)############

CLEANFILES = *~

KDE_OPTIONS = noautodist

EXTRA_DIST = \
fetcher.h fetchmanager.h fetchmanager.cpp \
amazonfetcher.h amazonfetcher.cpp z3950fetcher.h z3950fetcher.cpp \
imdbfetcher.h imdbfetcher.cpp fetch.h configwidget.h configwidget.cpp \
entrezfetcher.h entrezfetcher.cpp \
execexternalfetcher.h execexternalfetcher.cpp
