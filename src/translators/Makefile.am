####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libtranslators.a

## INCLUDES were found outside kdevelop specific part

libtranslators_a_METASOURCES = AUTO

libtranslators_a_SOURCES = pilotdbexporter.cpp bibtexexporter.cpp bibtexmlimporter.cpp bookcasexmlexporter.cpp csvimporter.cpp xsltexporter.cpp xslthandler.cpp bibteximporter.cpp xsltimporter.cpp textimporter.cpp bibtexhandler.cpp bibtexmlexporter.cpp htmlexporter.cpp csvexporter.cpp bookcasezipexporter.cpp xmlimporter.cpp bookcaseimporter.cpp

SUBDIRS = btparse pilotdb

EXTRA_DIST = bibtex-translation.xml \
bibtexexporter.cpp       bookcasexmlexporter.h    pilotdbexporter.cpp \
bibtexexporter.h         bookcasezipexporter.cpp  pilotdbexporter.h \
bibtexhandler.cpp        bookcasezipexporter.h    textexporter.h \
bibtexhandler.h          csvexporter.cpp          textimporter.cpp \
bibteximporter.cpp       csvexporter.h            textimporter.h \
bibteximporter.h         csvimporter.cpp          xmlimporter.cpp \
bibtexmlexporter.cpp     csvimporter.h            xmlimporter.h \
bibtexmlexporter.h       dataexporter.h           xsltexporter.cpp \
bibtexmlimporter.cpp     dataimporter.h           xsltexporter.h \
bibtexmlimporter.h       exporter.h               xslthandler.cpp \
bookcaseimporter.cpp     htmlexporter.cpp         xslthandler.h \
bookcaseimporter.h       htmlexporter.h           xsltimporter.cpp \
bookcasexmlexporter.cpp  importer.h               xsltimporter.h


####### kdevelop will overwrite this part!!! (end)############

INCLUDES = $(all_includes) $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS)

KDE_OPTIONS = noautodist

appdir = $(kde_datadir)/bookcase
app_DATA = bibtex-translation.xml
