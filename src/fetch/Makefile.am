####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libfetch.a

AM_CPPFLAGS = $(all_includes) $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS) $(YAZ_CFLAGS)

libfetch_a_METASOURCES = AUTO

libfetch_a_SOURCES = amazonfetcher.cpp animenfofetcher.cpp arxivfetcher.cpp \
	bibsonomyfetcher.cpp citebasefetcher.cpp configwidget.cpp crossreffetcher.cpp \
	entrezfetcher.cpp execexternalfetcher.cpp fetcher.cpp fetchmanager.cpp \
	gcstarpluginfetcher.cpp ibsfetcher.cpp imdbfetcher.cpp isbndbfetcher.cpp messagehandler.cpp \
	srufetcher.cpp yahoofetcher.cpp z3950connection.cpp z3950fetcher.cpp \
	googlescholarfetcher.cpp discogsfetcher.cpp

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
gcstarpluginfetcher.h gcstarpluginfetcher.cpp \
crossreffetcher.h crossreffetcher.cpp \
arxivfetcher.h arxivfetcher.cpp \
citebasefetcher.h citebasefetcher.cpp \
bibsonomyfetcher.h bibsonomyfetcher.cpp \
googlescholarfetcher.h googlescholarfetcher.cpp \
discogsfetcher.h discogsfetcher.cpp \
z3950-servers.cfg

appdir = $(kde_datadir)/tellico
app_DATA = z3950-servers.cfg
