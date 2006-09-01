####### kdevelop will overwrite this part!!! (begin)##########
noinst_LIBRARIES = libtranslators.a

## INCLUDES were found outside kdevelop specific part

libtranslators_a_METASOURCES = AUTO

libtranslators_a_SOURCES = pilotdbexporter.cpp bibtexexporter.cpp \
bibtexmlimporter.cpp tellicoxmlexporter.cpp csvimporter.cpp \
xsltexporter.cpp xslthandler.cpp bibteximporter.cpp xsltimporter.cpp \
textimporter.cpp bibtexhandler.cpp bibtexmlexporter.cpp \
htmlexporter.cpp csvexporter.cpp tellicozipexporter.cpp xmlimporter.cpp \
tellicoimporter.cpp audiofileimporter.cpp alexandriaimporter.cpp \
alexandriaexporter.cpp freedbimporter.cpp risimporter.cpp \
freedb_util.cpp tellico_xml.cpp exporter.cpp onixexporter.cpp \
gcfilmsimporter.cpp gcfilmsexporter.cpp filelistingimporter.cpp \
grs1importer.cpp amcimporter.cpp dcimporter.cpp

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
dcimporter.h             dcimporter.cpp

####### kdevelop will overwrite this part!!! (end)############

INCLUDES = $(LIBXML_CFLAGS) $(LIBXSLT_CFLAGS) $(TAGLIB_CFLAGS) $(KCDDB_CFLAGS) $(all_includes)

KDE_OPTIONS = noautodist

appdir = $(kde_datadir)/tellico
app_DATA = bibtex-translation.xml
