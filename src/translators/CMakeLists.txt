
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
   gcstarimporter.cpp
   griffithimporter.cpp
   grs1importer.cpp
   htmlexporter.cpp
   importer.cpp
   onixexporter.cpp
   pdfimporter.cpp
   pilotdbexporter.cpp
   referencerimporter.cpp
   risimporter.cpp
   tellico_xml.cpp
   tellicoimporter.cpp
   tellicoxmlexporter.cpp
   tellicoxmlhandler.cpp
   tellicozipexporter.cpp
   textimporter.cpp
   xmlimporter.cpp
   xmlstatehandler.cpp
   xmphandler.cpp
   xsltexporter.cpp
   xslthandler.cpp
   xsltimporter.cpp
   )

KDE4_ADD_LIBRARY(translators STATIC ${translators_STAT_SRCS})

ADD_DEPENDENCIES(translators tellico_config)

########### install files ###############

INSTALL(PROGRAMS   griffith2tellico.py DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
INSTALL(FILES   bibtex-translation.xml DESTINATION ${TELLICO_DATA_INSTALL_DIR} )
