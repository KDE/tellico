########### next target ###############

set(translators_STAT_SRCS
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
    filereaderbook.cpp
    filereadervideo.cpp
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
    onmyshelfimporter.cpp
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
target_link_libraries(translators
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

target_link_libraries(translators
    KF6::Archive
    KF6::JobWidgets
    KF6::Solid
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${LIBXSLT_EXSLT_LIBRARIES}
)

if(KCddb6_FOUND)
    target_link_libraries(translators KCddb6)
endif()

if(Poppler_Qt6_FOUND)
    target_link_libraries(translators Poppler::Qt6)
endif()

if(KF6FileMetaData_FOUND)
    target_link_libraries(translators KF6::FileMetaData)
endif()

if(Exempi_FOUND)
    target_link_libraries(translators ${Exempi_LIBRARIES})
endif()

if(TAGLIB_FOUND)
    target_link_libraries(translators ${TAGLIB_LIBRARIES})
endif()

if(CDIO_FOUND)
    target_link_libraries(translators ${CDIO_LIBRARIES})
endif()

add_dependencies(translators tellico_config)

########### install files ###############

install(FILES bibtex-translation.xml DESTINATION ${TELLICO_DATA_INSTALL_DIR})
