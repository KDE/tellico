####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libtranslators.a

## INCLUDES were found outside kdevelop specific part

libtranslators_a_METASOURCES = AUTO

libtranslators_a_SOURCES = bibtexexporter.cpp bibtexmlimporter.cpp bookcasexmlimporter.cpp bookcasexmlexporter.cpp csvimporter.cpp xsltexporter.cpp xslthandler.cpp bibteximporter.cpp xsltimporter.cpp textimporter.cpp bibtexhandler.cpp bibtexmlexporter.cpp htmlexporter.cpp csvexporter.cpp 

SUBDIRS = btparse 

EXTRA_DIST = csvexporter.cpp csvexporter.h htmlexporter.cpp htmlexporter.h bibtexmlexporter.cpp bibtexmlexporter.h bibtexexporter.h xsltexporter.h exporter.h bibtexhandler.cpp bibtexhandler.h importer.h textimporter.cpp xsltimporter.cpp bibteximporter.cpp bibteximporter.h xslthandler.h xslthandler.cpp xsltimporter.h xsltexporter.cpp csvimporter.cpp csvimporter.h textimporter.h bibtex-translation.xml bookcasexmlexporter.cpp bookcasexmlexporter.h bookcasexmlimporter.cpp bookcasexmlimporter.h bibtexmlimporter.cpp bibtexmlimporter.h bibtexexporter.cpp 

####### kdevelop will overwrite this part!!! (end)############

INCLUDES= $(all_includes) $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS)

KDE_OPTIONS = noautodist

datadir = $(kde_datadir)/bookcase
data_DATA = bibtex-translation.xml
