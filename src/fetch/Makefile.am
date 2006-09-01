####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libfetch.a

INCLUDES = $(all_includes) $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS) $(YAZ_CFLAGS)

libfetch_a_METASOURCES = AUTO

libfetch_a_SOURCES = fetchmanager.cpp amazonfetcher.cpp z3950fetcher.cpp \
imdbfetcher.cpp configwidget.cpp entrezfetcher.cpp \
execexternalfetcher.cpp messagehandler.cpp fetcher.cpp \
z3950connection.cpp yahoofetcher.cpp animenfofetcher.cpp \
ibsfetcher.cpp srufetcher.cpp isbndbfetcher.cpp

####### kdevelop will overwrite this part!!! (end)############

SUBDIRS = scripts

CLEANFILES = *~

KDE_OPTIONS = noautodist

EXTRA_DIST = \
fetcher.h fetcher.cpp fetchmanager.h fetchmanager.cpp \
amazonfetcher.h amazonfetcher.cpp z3950fetcher.h z3950fetcher.cpp \
imdbfetcher.h imdbfetcher.cpp fetch.h configwidget.h configwidget.cpp \
entrezfetcher.h entrezfetcher.cpp \
execexternalfetcher.h execexternalfetcher.cpp \
messagehandler.h messagehandler.cpp \
z3950connection.h z3950connection.cpp \
yahoofetcher.h yahoofetcher.cpp \
animenfofetcher.h animenfofetcher.cpp \
ibsfetcher.h ibsfetcher.cpp \
srufetcher.h srufetcher.cpp \
isbndbfetcher.h isbndbfetcher.cpp \
z3950-servers.cfg

appdir = $(kde_datadir)/tellico
app_DATA = z3950-servers.cfg
