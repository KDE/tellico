########### next target ###############

SET(translators_STAT_SRCS
   adsimporter.cpp
   alexandriaexporter.cpp
   alexandriaimporter.cpp
   amcimporter.cpp
   audiofileimporter.cpp
   bibtexexporter.cpp
   bibteximporter.cpp
   bibtexmlexporter.cpp
   bibtexmlimporter.cpp
   boardgamegeekimporter.cpp
   ciwimporter.cpp
   collectorzimporter.cpp
   csvexporter.cpp
   csvimporter.cpp
   csvparser.cpp
   datacrowimporter.cpp
   dataimporter.cpp
   deliciousimporter.cpp
   discogsimporter.cpp
   ebookimporter.cpp
   exporter.cpp
   filelistingimporter.cpp
   filereader.cpp
   freedb_util.cpp
   freedbimporter.cpp
   gcstarexporter.cpp
   gcstarimporter.cpp
   goodreadsimporter.cpp
   griffithimporter.cpp
   grs1importer.cpp
   htmlexporter.cpp
   importer.cpp
   librarythingimporter.cpp
   marcimporter.cpp
   onixexporter.cpp
   pdfimporter.cpp
   referencerimporter.cpp
   risimporter.cpp
   tellico_xml.cpp
   tellicoimporter.cpp
   tellicoxmlexporter.cpp
   tellicoxmlreader.cpp
   tellicozipexporter.cpp
   textimporter.cpp
   vinoxmlimporter.cpp
   xmlstatehandler.cpp
   xmphandler.cpp
   xsltexporter.cpp
   xslthandler.cpp
   xsltimporter.cpp
   )

add_library(translators STATIC ${translators_STAT_SRCS})
TARGET_LINK_LIBRARIES(translators
    collections
    tellicomodels
    core
    gui
    images
    utils
    ${TELLICO_BTPARSE_LIBS}
    ${TELLICO_CSV_LIBS}
    rtf2html-tellico
    )

TARGET_LINK_LIBRARIES(translators
    KF${KF_MAJOR_VERSION}::Archive
    KF${KF_MAJOR_VERSION}::JobWidgets
    KF${KF_MAJOR_VERSION}::Solid
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${LIBXSLT_EXSLT_LIBRARIES}
    )

IF( KF${KF_MAJOR_VERSION}Cddb_FOUND )
  TARGET_LINK_LIBRARIES(translators KF${KF_MAJOR_VERSION}::Cddb)
ELSEIF( Libkcddb_FOUND )
  TARGET_LINK_LIBRARIES(translators KDE::Libkcddb::kcddb)
ENDIF( KF${KF_MAJOR_VERSION}Cddb_FOUND)

IF( Poppler_Qt${QT_MAJOR_VERSION}_FOUND )
  TARGET_LINK_LIBRARIES(translators Poppler::Qt${QT_MAJOR_VERSION})
ENDIF( Poppler_Qt${QT_MAJOR_VERSION}_FOUND )

IF( KF${KF_MAJOR_VERSION}FileMetaData_FOUND )
  TARGET_LINK_LIBRARIES(translators KF${KF_MAJOR_VERSION}::FileMetaData)
ENDIF( KF${KF_MAJOR_VERSION}FileMetaData_FOUND )

IF( Exempi_FOUND )
  TARGET_LINK_LIBRARIES(translators ${Exempi_LIBRARIES})
ENDIF( Exempi_FOUND )

IF( TAGLIB_FOUND )
  TARGET_LINK_LIBRARIES(translators ${TAGLIB_LIBRARIES})
ENDIF( TAGLIB_FOUND )

IF( CDIO_FOUND )
  TARGET_LINK_LIBRARIES(translators ${CDIO_LIBRARIES})
ENDIF( CDIO_FOUND )

ADD_DEPENDENCIES(translators tellico_config)

########### install files ###############

INSTALL(FILES bibtex-translation.xml DESTINATION ${TELLICO_DATA_INSTALL_DIR})
