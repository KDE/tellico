####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libtranslators.a

## AM_CPPFLAGS were found outside kdevelop specific part

libtranslators_a_METASOURCES = AUTO

libtranslators_a_SOURCES = alexandriaexporter.cpp alexandriaimporter.cpp \
	amcimporter.cpp audiofileimporter.cpp bibtexexporter.cpp bibtexhandler.cpp \
	bibteximporter.cpp bibtexmlexporter.cpp bibtexmlimporter.cpp csvexporter.cpp \
	csvimporter.cpp dcimporter.cpp deliciousimporter.cpp exporter.cpp \
	filelistingimporter.cpp freedb_util.cpp freedbimporter.cpp gcfilmsexporter.cpp \
	gcfilmsimporter.cpp griffithimporter.cpp grs1importer.cpp htmlexporter.cpp libcsv.c \
	onixexporter.cpp pdfimporter.cpp pilotdbexporter.cpp referencerimporter.cpp \
	risimporter.cpp tellico_xml.cpp tellicoimporter.cpp tellicosaximporter.cpp \
	tellicoxmlexporter.cpp tellicoxmlhandler.cpp tellicozipexporter.cpp textimporter.cpp \
	xmlimporter.cpp xmlstatehandler.cpp xsltexporter.cpp xslthandler.cpp xsltimporter.cpp

if !USE_LIBBTPARSE
  SUBDIR_LIBBTPARSE = btparse
endif

SUBDIRS = pilotdb $(SUBDIR_LIBBTPARSE)

CLEANFILES = *~

EXTRA_DIST = bibtex-translation.xml \
bibtexexporter.cpp       tellicoxmlexporter.h     pilotdbexporter.cpp \
bibtexexporter.h         tellicozipexporter.cpp   pilotdbexporter.h \
bibtexhandler.cpp        tellicozipexporter.h     \
bibtexhandler.h          csvexporter.cpp          textimporter.cpp \
bibteximporter.cpp       csvexporter.h            textimporter.h \
bibteximporter.h         csvimporter.cpp          xmlimporter.cpp \
bibtexmlexporter.cpp     csvimporter.h            xmlimporter.h \
bibtexmlexporter.h                                xsltexporter.cpp \
bibtexmlimporter.cpp     dataimporter.h           xsltexporter.h \
bibtexmlimporter.h       exporter.h               xslthandler.cpp \
tellicoimporter.cpp      htmlexporter.cpp         xslthandler.h \
tellicoimporter.h        htmlexporter.h           xsltimporter.cpp \
tellicoxmlexporter.cpp   importer.h               xsltimporter.h \
audiofileimporter.h      audiofileimporter.cpp    alexandriaimporter.h \
alexandriaimporter.cpp   alexandriaexporter.h     alexandriaexporter.cpp \
freedbimporter.h         freedbimporter.cpp       freedb_util.cpp \
risimporter.h            risimporter.cpp          tellico_xml.h \
tellico_xml.cpp          translators.h            exporter.cpp \
onixexporter.h           onixexporter.cpp         gcfilmsimporter.h \
gcfilmsimporter.cpp      gcfilmsexporter.h        gcfilmsexporter.cpp \
filelistingimporter.h    filelistingimporter.cpp  grs1importer.h \
grs1importer.cpp         amcimporter.h            amcimporter.cpp \
dcimporter.h             dcimporter.cpp           griffithimporter.h \
griffithimporter.cpp     griffith2tellico.py      pdfimporter.h \
pdfimporter.cpp          referencerimporter.h     referencerimporter.cpp \
libcsv.h                 libcsv.c \
deliciousimporter.h      deliciousimporter.cpp \
tellicosaximporter.h     tellicosaximporter.cpp \
tellicoxmlhandler.h      tellicoxmlhandler.cpp \
xmlstatehandler.h        xmlstatehandler.cpp

####### kdevelop will overwrite this part!!! (end)############

AM_CPPFLAGS = \
 $(LIBXML_CFLAGS) \
 $(LIBXSLT_CFLAGS) \
 $(TAGLIB_CFLAGS) \
 $(KCDDB_CFLAGS) \
 $(all_includes) \
 $(POPPLER_CFLAGS)

KDE_OPTIONS = noautodist

appdir = $(kde_datadir)/tellico
app_DATA = bibtex-translation.xml
app_SCRIPTS = griffith2tellico.py

