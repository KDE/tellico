ADD_SUBDIRECTORY( pilotdb )
ADD_SUBDIRECTORY( btparse )

INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} )
LINK_DIRECTORIES( ${CMAKE_BINARY_DIR} )

########### next target ###############

SET(translators_STAT_SRCS
   alexandriaexporter.cpp
   alexandriaimporter.cpp
   amcimporter.cpp
   audiofileimporter.cpp
   bibtexexporter.cpp
   bibtexhandler.cpp
   bibteximporter.cpp
   bibtexmlexporter.cpp
   bibtexmlimporter.cpp
   csvexporter.cpp
   csvimporter.cpp
   csvparser.cpp
   dataimporter.cpp
   dcimporter.cpp
   deliciousimporter.cpp
   exporter.cpp
   filelistingimporter.cpp
   freedb_util.cpp
   freedbimporter.cpp
   gcfilmsexporter.cpp
   gcfilmsimporter.cpp
   griffithimporter.cpp
   grs1importer.cpp
   htmlexporter.cpp
   importer.cpp
   libcsv.c
   onixexporter.cpp
   pdfimporter.cpp
   pilotdbexporter.cpp
   referencerimporter.cpp
   risimporter.cpp
   tellico_xml.cpp
   tellicoimporter.cpp
   tellicosaximporter.cpp
   tellicoxmlexporter.cpp
   tellicoxmlhandler.cpp
   tellicozipexporter.cpp
   textimporter.cpp
   xmlimporter.cpp
   xmlstatehandler.cpp
   xsltexporter.cpp
   xslthandler.cpp
   xsltimporter.cpp
   )

KDE4_ADD_LIBRARY(translators STATIC ${translators_STAT_SRCS})

########### install files ###############

INSTALL(PROGRAMS   griffith2tellico.py DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
INSTALL(FILES   bibtex-translation.xml DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
