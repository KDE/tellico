include(ECMAddTests)
include(ECMMarkNonGuiExecutable)

SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# formattest includes fieldformat that include core/tellico_config.h
# which is generated in the build directory
INCLUDE_DIRECTORIES( ${Tellico_BINARY_DIR}/src )

add_executable(entitytest entitytest.cpp ../fieldformat.cpp)
ecm_mark_nongui_executable(entitytest)
add_test(entitytest entitytest)
ecm_mark_as_test(entitytest)
TARGET_LINK_LIBRARIES(entitytest utils config Qt5::Test)

add_executable(completiontest completiontest.cpp ../fieldcompletion.cpp ../field.cpp ../fieldformat.cpp)
ecm_mark_nongui_executable(completiontest)
add_test(completiontest completiontest)
ecm_mark_as_test(completiontest)
TARGET_LINK_LIBRARIES(completiontest config utils KF5::Completion Qt5::Test)

add_executable(cuecattest cuecattest.cpp)
ecm_mark_nongui_executable(cuecattest)
add_test(cuecattest cuecattest)
ecm_mark_as_test(cuecattest)
TARGET_LINK_LIBRARIES(cuecattest utils Qt5::Test)

add_executable(isbntest isbntest.cpp)
ecm_mark_nongui_executable(isbntest)
add_test(isbntest isbntest)
ecm_mark_as_test(isbntest)
TARGET_LINK_LIBRARIES(isbntest utils Qt5::Test)

add_executable(lccntest lccntest.cpp)
ecm_mark_nongui_executable(lccntest)
add_test(lccntest lccntest)
ecm_mark_as_test(lccntest)
TARGET_LINK_LIBRARIES(lccntest utils Qt5::Test)

add_executable(lcctest lcctest.cpp ../field.cpp ../fieldformat.cpp)
ecm_mark_nongui_executable(lcctest)
add_test(lcctest lcctest)
ecm_mark_as_test(lcctest)
TARGET_LINK_LIBRARIES(lcctest tellicomodels Qt5::Test)

add_executable(formattest formattest.cpp ../fieldformat.cpp)
ecm_mark_nongui_executable(formattest)
add_test(formattest formattest)
ecm_mark_as_test(formattest)
TARGET_LINK_LIBRARIES(formattest config Qt5::Test)

add_executable(fieldtest fieldtest.cpp ../field.cpp ../fieldformat.cpp ../gui/urlfieldlogic.cpp)
ecm_mark_nongui_executable(fieldtest)
add_test(fieldtest fieldtest)
ecm_mark_as_test(fieldtest)
TARGET_LINK_LIBRARIES(fieldtest config utils Qt5::Test)

add_executable(imagetest imagetest.cpp ../utils/tellico_utils.cpp ../utils/guiproxy.cpp ../utils/cursorsaver.cpp ../fieldformat.cpp)
ecm_mark_nongui_executable(imagetest)
add_test(imagetest imagetest)
ecm_mark_as_test(imagetest)
TARGET_LINK_LIBRARIES(imagetest images KF5::Archive Qt5::Test)

add_executable(imagejobtest imagejobtest.cpp ../fieldformat.cpp)
ecm_mark_nongui_executable(imagejobtest)
add_test(imagejobtest imagejobtest)
ecm_mark_as_test(imagejobtest)
TARGET_LINK_LIBRARIES(imagejobtest images KF5::Archive Qt5::Test)

add_executable(iso6937test iso6937test.cpp)
ecm_mark_nongui_executable(iso6937test)
add_test(iso6937test iso6937test)
ecm_mark_as_test(iso6937test)
TARGET_LINK_LIBRARIES(iso6937test utils Qt5::Test)

add_executable(iso5426test iso5426test.cpp)
ecm_mark_nongui_executable(iso5426test)
add_test(iso5426test iso5426test)
ecm_mark_as_test(iso5426test)
TARGET_LINK_LIBRARIES(iso5426test utils Qt5::Test)

SET(tellicotest_SRCS
   ../collection.cpp
   ../entry.cpp
   ../entrygroup.cpp
   ../entrycomparison.cpp
   ../field.cpp
   ../fieldformat.cpp
   ../filter.cpp
   ../borrower.cpp
   ../collectionfactory.cpp
   ../derivedvalue.cpp
   ../progressmanager.cpp
)

add_library(tellicotest STATIC ${tellicotest_SRCS})
TARGET_LINK_LIBRARIES(tellicotest Qt5::Core Qt5::Gui KF5::I18n KF5::ConfigWidgets)

ADD_DEPENDENCIES(tellicotest tellico_config)

SET(translatorstest_SRCS
  ../translators/tellicoimporter.cpp
  ../translators/xsltimporter.cpp
  ../translators/textimporter.cpp
  ../translators/dataimporter.cpp
  ../translators/importer.cpp
  ../translators/tellicoxmlreader.cpp
  ../translators/tellico_xml.cpp
  ../translators/xmlstatehandler.cpp
  ../translators/xslthandler.cpp
)

add_library(translatorstest STATIC ${translatorstest_SRCS})
TARGET_LINK_LIBRARIES(translatorstest
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::Xml
  KF5::I18n
  KF5::Archive
  KF5::KIOWidgets
  KF5::ConfigWidgets
  ${LIBXML2_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  ${LIBXSLT_EXSLT_LIBRARIES}
)

set(TELLICO_TEST_LIBS
   tellicotest
   collections
   utils
   images
   core
   tellicomodels
   Qt5::Test
   KF5::KIOCore
   KF5::Archive
)

add_executable(collectiontest collectiontest.cpp
  ../document.cpp
  ../translators/tellicoxmlexporter.cpp
  ../translators/tellicozipexporter.cpp
  ../translators/exporter.cpp
)
ecm_mark_nongui_executable(collectiontest)
add_test(collectiontest collectiontest)
ecm_mark_as_test(collectiontest)
TARGET_LINK_LIBRARIES(collectiontest translatorstest ${TELLICO_TEST_LIBS})

add_executable(comparisontest comparisontest.cpp)
ecm_mark_nongui_executable(comparisontest)
add_test(comparisontest comparisontest)
ecm_mark_as_test(comparisontest)
TARGET_LINK_LIBRARIES(comparisontest tellicotest tellicomodels Qt5::Test)

add_executable(documenttest documenttest.cpp
  ../document.cpp
  ../translators/tellicoxmlexporter.cpp
  ../translators/tellicozipexporter.cpp
  ../translators/exporter.cpp
)
ecm_mark_nongui_executable(documenttest)
add_test(documenttest documenttest)
ecm_mark_as_test(documenttest)
TARGET_LINK_LIBRARIES(documenttest translatorstest ${TELLICO_TEST_LIBS})

add_executable(filtertest filtertest.cpp)
ecm_mark_nongui_executable(filtertest)
add_test(filtertest filtertest)
ecm_mark_as_test(filtertest)
TARGET_LINK_LIBRARIES(filtertest gui ${TELLICO_TEST_LIBS})

add_executable(tellicomodeltest tellicomodeltest.cpp modeltest.cpp
  ../document.cpp
  ../translators/tellicoimporter.cpp
  ../translators/dataimporter.cpp
  ../translators/importer.cpp
  ../translators/tellicoxmlexporter.cpp
  ../translators/tellicozipexporter.cpp
  ../translators/exporter.cpp
  ../translators/tellicoxmlreader.cpp
  ../translators/tellico_xml.cpp
  ../translators/xmlstatehandler.cpp
)
ecm_mark_nongui_executable(tellicomodeltest)
add_test(tellicomodeltest tellicomodeltest)
ecm_mark_as_test(tellicomodeltest)
TARGET_LINK_LIBRARIES(tellicomodeltest ${TELLICO_TEST_LIBS} ${LIBXML2_LIBRARIES})

######################################################

add_executable(adstest adstest.cpp
  ../translators/adsimporter.cpp
  ../translators/importer.cpp
)
ecm_mark_nongui_executable(adstest)
add_test(adstest adstest)
ecm_mark_as_test(adstest)
TARGET_LINK_LIBRARIES(adstest ${TELLICO_TEST_LIBS})

add_executable(alexandriatest alexandriatest.cpp
  ../translators/alexandriaimporter.cpp
  ../translators/importer.cpp
  ../translators/alexandriaexporter.cpp
  ../translators/exporter.cpp
)
ecm_mark_nongui_executable(alexandriatest)
add_test(alexandriatest alexandriatest)
ecm_mark_as_test(alexandriatest)
TARGET_LINK_LIBRARIES(alexandriatest ${TELLICO_TEST_LIBS})

add_executable(amctest amctest.cpp
  ../translators/amcimporter.cpp
  ../translators/dataimporter.cpp
  ../translators/importer.cpp
)
ecm_mark_nongui_executable(amctest)
add_test(amctest amctest)
ecm_mark_as_test(amctest)
TARGET_LINK_LIBRARIES(amctest ${TELLICO_TEST_LIBS})

IF( TAGLIB_FOUND )

add_executable(audiofiletest audiofiletest.cpp
  ../translators/audiofileimporter.cpp
  ../translators/dataimporter.cpp
  ../translators/importer.cpp
)
ecm_mark_nongui_executable(audiofiletest)
add_test(audiofiletest audiofiletest)
ecm_mark_as_test(audiofiletest)
TARGET_LINK_LIBRARIES(audiofiletest ${TELLICO_TEST_LIBS} ${TAGLIB_LIBRARIES})

ENDIF( TAGLIB_FOUND )

add_executable(bibtextest bibtextest.cpp
  ../translators/bibteximporter.cpp
  ../translators/importer.cpp
  ../translators/bibtexexporter.cpp
  ../translators/exporter.cpp
)
ecm_mark_nongui_executable(bibtextest)
add_test(bibtextest bibtextest)
ecm_mark_as_test(bibtextest)
TARGET_LINK_LIBRARIES(bibtextest ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS})

add_executable(bibtexmltest bibtexmltest.cpp
  ../translators/bibtexmlimporter.cpp
  ../translators/bibtexmlexporter.cpp
  ../translators/exporter.cpp
)
ecm_mark_nongui_executable(bibtexmltest)
add_test(bibtexmltest bibtexmltest)
ecm_mark_as_test(bibtexmltest)
TARGET_LINK_LIBRARIES(bibtexmltest translatorstest core ${TELLICO_TEST_LIBS})

add_executable(citetest citetest.cpp
  ../collections/bibtexcollection.cpp
  ../core/filehandler.cpp
)
ecm_mark_nongui_executable(citetest)
add_test(citetest citetest)
ecm_mark_as_test(citetest)
TARGET_LINK_LIBRARIES(citetest cite tellicotest config images Qt5::Test)
ADD_DEPENDENCIES(citetest tellico_config)

add_executable(ciwtest ciwtest.cpp
  ../translators/ciwimporter.cpp
  ../translators/importer.cpp
)
ecm_mark_nongui_executable(ciwtest)
add_test(ciwtest ciwtest)
ecm_mark_as_test(ciwtest)
TARGET_LINK_LIBRARIES(ciwtest ${TELLICO_TEST_LIBS})

add_executable(collectorztest collectorztest.cpp
  ../translators/collectorzimporter.cpp
)
ecm_mark_nongui_executable(collectorztest)
add_test(collectorztest collectorztest)
ecm_mark_as_test(collectorztest)
TARGET_LINK_LIBRARIES(collectorztest translatorstest ${TELLICO_TEST_LIBS})

add_executable(csvtest csvtest.cpp
  ../translators/csvparser.cpp
  ../translators/csvexporter.cpp
  ../translators/exporter.cpp
)
ecm_mark_nongui_executable(csvtest)
add_test(csvtest csvtest)
ecm_mark_as_test(csvtest)
TARGET_LINK_LIBRARIES(csvtest ${TELLICO_CSV_LIBS} translatorstest ${TELLICO_TEST_LIBS})

add_executable(delicioustest delicioustest.cpp
  ../translators/deliciousimporter.cpp
)
ecm_mark_nongui_executable(delicioustest)
add_test(delicioustest delicioustest)
ecm_mark_as_test(delicioustest)
TARGET_LINK_LIBRARIES(delicioustest rtf2html-tellico translatorstest ${TELLICO_TEST_LIBS})

add_executable(filelistingtest filelistingtest.cpp
  ../translators/filelistingimporter.cpp
  ../translators/xmphandler.cpp
)
ecm_mark_nongui_executable(filelistingtest)
add_test(filelistingtest filelistingtest)
ecm_mark_as_test(filelistingtest)
TARGET_LINK_LIBRARIES(filelistingtest
  translatorstest
  ${TELLICO_TEST_LIBS}
  KF5::Solid
)
IF( KF5FileMetaData_FOUND )
  TARGET_LINK_LIBRARIES(filelistingtest KF5::FileMetaData)
ENDIF( KF5FileMetaData_FOUND )
IF( Exempi_FOUND )
  TARGET_LINK_LIBRARIES(filelistingtest ${Exempi_LIBRARIES})
ENDIF( Exempi_FOUND )

add_executable(gcstartest gcstartest.cpp
  ../translators/gcstarimporter.cpp
  ../translators/gcstarexporter.cpp
  ../translators/tellicoxmlexporter.cpp
  ../translators/tellicozipexporter.cpp
  ../translators/exporter.cpp
  ../document.cpp
)
ecm_mark_nongui_executable(gcstartest)
add_test(gcstartest gcstartest)
ecm_mark_as_test(gcstartest)
TARGET_LINK_LIBRARIES(gcstartest translatorstest ${TELLICO_TEST_LIBS})

add_executable(griffithtest griffithtest.cpp
  ../translators/griffithimporter.cpp
)
ecm_mark_nongui_executable(griffithtest)
add_test(griffithtest griffithtest)
ecm_mark_as_test(griffithtest)
TARGET_LINK_LIBRARIES(griffithtest translatorstest ${TELLICO_TEST_LIBS})

add_executable(htmlexportertest htmlexportertest.cpp
  ../translators/htmlexporter.cpp
  ../translators/tellicoxmlexporter.cpp
  ../translators/tellicozipexporter.cpp
  ../translators/exporter.cpp
  ../document.cpp
)
ecm_mark_nongui_executable(htmlexportertest)
add_test(htmlexportertest htmlexportertest)
ecm_mark_as_test(htmlexportertest)
TARGET_LINK_LIBRARIES(htmlexportertest translatorstest ${TELLICO_TEST_LIBS})

add_executable(modstest modstest.cpp)
ecm_mark_nongui_executable(modstest)
add_test(modstest modstest)
ecm_mark_as_test(modstest)
TARGET_LINK_LIBRARIES(modstest translatorstest ${TELLICO_TEST_LIBS})

add_executable(referencertest referencertest.cpp
  ../translators/referencerimporter.cpp
)
ecm_mark_nongui_executable(referencertest)
add_test(referencertest referencertest)
ecm_mark_as_test(referencertest)
TARGET_LINK_LIBRARIES(referencertest translatorstest ${TELLICO_TEST_LIBS})

add_executable(ristest ristest.cpp
  ../translators/risimporter.cpp
  ../translators/importer.cpp
)
ecm_mark_nongui_executable(ristest)
add_test(ristest ristest)
ecm_mark_as_test(ristest)
TARGET_LINK_LIBRARIES(ristest ${TELLICO_TEST_LIBS})

add_executable(tellicoreadtest tellicoreadtest.cpp
  ../translators/tellicoxmlexporter.cpp
  ../translators/tellicozipexporter.cpp
  ../translators/exporter.cpp
  ../document.cpp
)
ecm_mark_nongui_executable(tellicoreadtest)
add_test(tellicoreadtest tellicoreadtest)
ecm_mark_as_test(tellicoreadtest)
TARGET_LINK_LIBRARIES(tellicoreadtest translatorstest ${TELLICO_TEST_LIBS})

add_executable(vinoxmltest vinoxmltest.cpp
  ../translators/vinoxmlimporter.cpp
)
ecm_mark_nongui_executable(vinoxmltest)
add_test(vinoxmltest vinoxmltest)
ecm_mark_as_test(vinoxmltest)
TARGET_LINK_LIBRARIES(vinoxmltest translatorstest ${TELLICO_TEST_LIBS})

SET(fetcherstest_SRCS
  ../fetch/fetcher.cpp
  ../fetch/fetcherjob.cpp
  ../fetch/fetchrequest.cpp
  ../fetch/fetchresult.cpp
  ../fetch/fetchmanager.cpp
  ../fetch/messagehandler.cpp
  ../fetch/configwidget.cpp
  ../document.cpp
  ../translators/tellicoxmlexporter.cpp
  ../translators/tellicozipexporter.cpp
  ../translators/exporter.cpp
)

# FetchManager links to SRUFetcher for the default sources
# so have to include everything it needs
SET(fetcherstest_SRCS
  ${fetcherstest_SRCS}
  ../fetch/srufetcher.cpp
  ../fetch/xmlfetcher.cpp
  ../gui/lineedit.cpp
  ../gui/combobox.cpp
  ../gui/stringmapwidget.cpp
)

add_library(fetcherstest STATIC ${fetcherstest_SRCS})
TARGET_LINK_LIBRARIES(fetcherstest
  translatorstest
  Qt5::Core
  Qt5::Gui
  Qt5::DBus
  KF5::ConfigCore
  KF5::ConfigGui
  KF5::ConfigWidgets
  KF5::I18n
  KF5::Archive
  KF5::IconThemes
  KF5::XmlGui
  KF5::SonnetCore
  KF5::SonnetUi
  ${LIBXML2_LIBRARIES}
  ${LIBXSLT_LIBRARIES}
  ${LIBXSLT_EXSLT_LIBRARIES}
)
ADD_DEPENDENCIES(fetcherstest tellico_config)

IF(${KF5KIO_VERSION} VERSION_GREATER "5.18.0")
  TARGET_LINK_LIBRARIES(fetcherstest
      KF5::KIOGui
  )
ENDIF(${KF5KIO_VERSION} VERSION_GREATER "5.18.0")

# the PDF importer uses CrossRefFetcher, so include the test in the fetchers
add_executable(pdftest pdftest.cpp
  ../translators/pdfimporter.cpp
  ../translators/xmphandler.cpp
  ../entryupdatejob.cpp
)
ecm_mark_nongui_executable(pdftest)
add_test(pdftest pdftest)
ecm_mark_as_test(pdftest)
TARGET_LINK_LIBRARIES(pdftest translatorstest fetcherstest ${TELLICO_TEST_LIBS})
IF( Exempi_FOUND )
  TARGET_LINK_LIBRARIES(pdftest ${Exempi_LIBRARIES})
ENDIF( Exempi_FOUND )
IF( Poppler_Qt5_FOUND )
  TARGET_LINK_LIBRARIES(pdftest Poppler::Qt5)
ENDIF( Poppler_Qt5_FOUND )

# fetcher tests from here down
IF(BUILD_FETCHER_TESTS)

add_executable(entryupdatejobtest entryupdatejobtest.cpp
  ../entryupdatejob.cpp
  ../fetch/arxivfetcher.cpp
)
ecm_mark_nongui_executable(entryupdatejobtest)
add_test(entryupdatejobtest entryupdatejobtest)
ecm_mark_as_test(entryupdatejobtest)
TARGET_LINK_LIBRARIES(entryupdatejobtest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(allocinefetchertest allocinefetchertest.cpp abstractfetchertest.cpp
  ../fetch/allocinefetcher.cpp
  ../fetch/execexternalfetcher.cpp
  ../translators/bibteximporter.cpp
  ../translators/risimporter.cpp
  ../gui/collectiontypecombo.cpp
)
ecm_mark_nongui_executable(allocinefetchertest)
add_test(allocinefetchertest allocinefetchertest)
ecm_mark_as_test(allocinefetchertest)
# SonnetCore is needed for GUI::LineEdit
TARGET_LINK_LIBRARIES(allocinefetchertest
  fetcherstest
  translatorstest
  newstuff
  ${TELLICO_BTPARSE_LIBS}
  ${TELLICO_TEST_LIBS}
)

add_executable(amazonfetchertest amazonfetchertest.cpp abstractfetchertest.cpp
  ../fetch/amazonfetcher.cpp
  ../fetch/amazonrequest.cpp
  ../fetch/messagelogger.cpp
)
ecm_mark_nongui_executable(amazonfetchertest)
add_test(amazonfetchertest amazonfetchertest)
ecm_mark_as_test(amazonfetchertest)
TARGET_LINK_LIBRARIES(amazonfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(animenfofetchertest animenfofetchertest.cpp abstractfetchertest.cpp
  ../fetch/animenfofetcher.cpp
)
ecm_mark_nongui_executable(animenfofetchertest)
add_test(animenfofetchertest animenfofetchertest)
ecm_mark_as_test(animenfofetchertest)
TARGET_LINK_LIBRARIES(animenfofetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(arxivfetchertest arxivfetchertest.cpp abstractfetchertest.cpp
  ../fetch/arxivfetcher.cpp
)
ecm_mark_nongui_executable(arxivfetchertest)
add_test(arxivfetchertest arxivfetchertest)
ecm_mark_as_test(arxivfetchertest)
TARGET_LINK_LIBRARIES(arxivfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(bedethequefetchertest bedethequefetchertest.cpp abstractfetchertest.cpp
  ../fetch/bedethequefetcher.cpp
)
ecm_mark_nongui_executable(bedethequefetchertest)
add_test(bedethequefetchertest bedethequefetchertest)
ecm_mark_as_test(bedethequefetchertest)
TARGET_LINK_LIBRARIES(bedethequefetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(bibliosharefetchertest bibliosharefetchertest.cpp abstractfetchertest.cpp
  ../fetch/bibliosharefetcher.cpp
)
ecm_mark_nongui_executable(bibliosharefetchertest)
add_test(bibliosharefetchertest bibliosharefetchertest)
ecm_mark_as_test(bibliosharefetchertest)
TARGET_LINK_LIBRARIES(bibliosharefetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(boardgamegeekfetchertest boardgamegeekfetchertest.cpp abstractfetchertest.cpp
  ../fetch/boardgamegeekfetcher.cpp
)
ecm_mark_nongui_executable(boardgamegeekfetchertest)
add_test(boardgamegeekfetchertest boardgamegeekfetchertest)
ecm_mark_as_test(boardgamegeekfetchertest)
TARGET_LINK_LIBRARIES(boardgamegeekfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(comicvinefetchertest comicvinefetchertest.cpp abstractfetchertest.cpp
  ../fetch/comicvinefetcher.cpp
)
ecm_mark_nongui_executable(comicvinefetchertest)
add_test(comicvinefetchertest comicvinefetchertest)
ecm_mark_as_test(comicvinefetchertest)
TARGET_LINK_LIBRARIES(comicvinefetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(colnectfetchertest colnectfetchertest.cpp abstractfetchertest.cpp
  ../fetch/colnectfetcher.cpp
)
ecm_mark_nongui_executable(colnectfetchertest)
add_test(colnectfetchertest colnectfetchertest)
ecm_mark_as_test(colnectfetchertest)
TARGET_LINK_LIBRARIES(colnectfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(crossreffetchertest crossreffetchertest.cpp abstractfetchertest.cpp
  ../fetch/crossreffetcher.cpp
)
ecm_mark_nongui_executable(crossreffetchertest)
add_test(crossreffetchertest crossreffetchertest)
ecm_mark_as_test(crossreffetchertest)
TARGET_LINK_LIBRARIES(crossreffetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(darkhorsefetchertest darkhorsefetchertest.cpp abstractfetchertest.cpp
  ../fetch/execexternalfetcher.cpp
  ../translators/bibteximporter.cpp
  ../translators/risimporter.cpp
  ../gui/collectiontypecombo.cpp
)
ecm_mark_nongui_executable(darkhorsefetchertest)
add_test(darkhorsefetchertest darkhorsefetchertest)
ecm_mark_as_test(darkhorsefetchertest)
TARGET_LINK_LIBRARIES(darkhorsefetchertest
  fetcherstest
  newstuff
  ${TELLICO_BTPARSE_LIBS}
  ${TELLICO_TEST_LIBS}
)

add_executable(dbcfetchertest dbcfetchertest.cpp abstractfetchertest.cpp
  ../fetch/dbcfetcher.cpp
)
ecm_mark_nongui_executable(dbcfetchertest)
add_test(dbcfetchertest dbcfetchertest)
ecm_mark_as_test(dbcfetchertest)
TARGET_LINK_LIBRARIES(dbcfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(dblpfetchertest dblpfetchertest.cpp abstractfetchertest.cpp
  ../fetch/dblpfetcher.cpp
)
ecm_mark_nongui_executable(dblpfetchertest)
add_test(dblpfetchertest dblpfetchertest)
ecm_mark_as_test(dblpfetchertest)
TARGET_LINK_LIBRARIES(dblpfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(discogsfetchertest discogsfetchertest.cpp abstractfetchertest.cpp
  ../fetch/discogsfetcher.cpp
)
ecm_mark_nongui_executable(discogsfetchertest)
add_test(discogsfetchertest discogsfetchertest)
ecm_mark_as_test(discogsfetchertest)
TARGET_LINK_LIBRARIES(discogsfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(doubanfetchertest doubanfetchertest.cpp abstractfetchertest.cpp
  ../fetch/doubanfetcher.cpp
)
ecm_mark_nongui_executable(doubanfetchertest)
add_test(doubanfetchertest doubanfetchertest)
ecm_mark_as_test(doubanfetchertest)
TARGET_LINK_LIBRARIES(doubanfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(dvdfrfetchertest dvdfrfetchertest.cpp abstractfetchertest.cpp
  ../fetch/dvdfrfetcher.cpp
)
ecm_mark_nongui_executable(dvdfrfetchertest)
add_test(dvdfrfetchertest dvdfrfetchertest)
ecm_mark_as_test(dvdfrfetchertest)
TARGET_LINK_LIBRARIES(dvdfrfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(entrezfetchertest entrezfetchertest.cpp abstractfetchertest.cpp
  ../fetch/entrezfetcher.cpp
)
ecm_mark_nongui_executable(entrezfetchertest)
add_test(entrezfetchertest entrezfetchertest)
ecm_mark_as_test(entrezfetchertest)
TARGET_LINK_LIBRARIES(entrezfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(externalfetchertest externalfetchertest.cpp abstractfetchertest.cpp
  ../fetch/execexternalfetcher.cpp
  ../translators/bibteximporter.cpp
  ../translators/risimporter.cpp
  ../gui/collectiontypecombo.cpp
)
ecm_mark_nongui_executable(externalfetchertest)
add_test(externalfetchertest externalfetchertest)
ecm_mark_as_test(externalfetchertest)
TARGET_LINK_LIBRARIES(externalfetchertest
  fetcherstest
  newstuff
  ${TELLICO_BTPARSE_LIBS}
  ${TELLICO_TEST_LIBS}
  KF5::SonnetCore
  KF5::SonnetUi
  KF5::XmlGui
)

add_executable(filmasterfetchertest filmasterfetchertest.cpp abstractfetchertest.cpp
  ../fetch/filmasterfetcher.cpp
)
ecm_mark_nongui_executable(filmasterfetchertest)
add_test(filmasterfetchertest filmasterfetchertest)
ecm_mark_as_test(filmasterfetchertest)
TARGET_LINK_LIBRARIES(filmasterfetchertest fetcherstest translatorstest ${TELLICO_TEST_LIBS})

# running gcstar in the fetcher is really unreliable
add_executable(gcstarfetchertest gcstarfetchertest.cpp abstractfetchertest
  ../fetch/gcstarpluginfetcher.cpp
  ../fetch/gcstarthread.cpp
  ../translators/gcstarimporter.cpp
  ../gui/collectiontypecombo.cpp
)
ecm_mark_nongui_executable(gcstarfetchertest)
#add_test(gcstarfetchertest gcstarfetchertest)
#ecm_mark_as_test(gcstarfetchertest)
TARGET_LINK_LIBRARIES(gcstarfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(giantbombfetchertest giantbombfetchertest.cpp abstractfetchertest.cpp
  ../fetch/giantbombfetcher.cpp
)
ecm_mark_nongui_executable(giantbombfetchertest)
add_test(giantbombfetchertest giantbombfetchertest)
ecm_mark_as_test(giantbombfetchertest)
TARGET_LINK_LIBRARIES(giantbombfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(goodreadstest goodreadstest.cpp
  ../translators/goodreadsimporter.cpp
)
ecm_mark_nongui_executable(goodreadstest)
add_test(goodreadstest goodreadstest)
ecm_mark_as_test(goodreadstest)
TARGET_LINK_LIBRARIES(goodreadstest translatorstest ${TELLICO_TEST_LIBS})

add_executable(googlebookfetchertest googlebookfetchertest.cpp abstractfetchertest.cpp
  ../fetch/googlebookfetcher.cpp
)
ecm_mark_nongui_executable(googlebookfetchertest)
add_test(googlebookfetchertest googlebookfetchertest)
ecm_mark_as_test(googlebookfetchertest)
TARGET_LINK_LIBRARIES(googlebookfetchertest fetcherstest translatorstest ${TELLICO_TEST_LIBS})

add_executable(googlescholarfetchertest googlescholarfetchertest.cpp abstractfetchertest.cpp
  ../fetch/googlescholarfetcher.cpp
  ../translators/bibteximporter.cpp
)
ecm_mark_nongui_executable(googlescholarfetchertest)
add_test(googlescholarfetchertest googlescholarfetchertest)
ecm_mark_as_test(googlescholarfetchertest)
TARGET_LINK_LIBRARIES(googlescholarfetchertest fetcherstest ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS})

add_executable(hathitrustfetchertest hathitrustfetchertest.cpp abstractfetchertest.cpp
  ../fetch/hathitrustfetcher.cpp
)
ecm_mark_nongui_executable(hathitrustfetchertest)
add_test(hathitrustfetchertest hathitrustfetchertest)
ecm_mark_as_test(hathitrustfetchertest)
TARGET_LINK_LIBRARIES(hathitrustfetchertest fetcherstest translatorstest ${TELLICO_TEST_LIBS})

add_executable(ibsfetchertest ibsfetchertest.cpp abstractfetchertest.cpp
  ../fetch/ibsfetcher.cpp
)
ecm_mark_nongui_executable(ibsfetchertest)
add_test(ibsfetchertest ibsfetchertest)
ecm_mark_as_test(ibsfetchertest)
TARGET_LINK_LIBRARIES(ibsfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(igdbfetchertest igdbfetchertest.cpp abstractfetchertest.cpp
  ../fetch/igdbfetcher.cpp
)
ecm_mark_nongui_executable(igdbfetchertest)
add_test(igdbfetchertest igdbfetchertest)
ecm_mark_as_test(igdbfetchertest)
TARGET_LINK_LIBRARIES(igdbfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(imdbfetchertest imdbfetchertest.cpp abstractfetchertest.cpp
  ../fetch/imdbfetcher.cpp
  ../gui/listwidgetitem.cpp
)
ecm_mark_nongui_executable(imdbfetchertest)
add_test(imdbfetchertest imdbfetchertest)
ecm_mark_as_test(imdbfetchertest)
TARGET_LINK_LIBRARIES(imdbfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(isbndbfetchertest isbndbfetchertest.cpp abstractfetchertest.cpp
  ../fetch/isbndbfetcher.cpp
  ../fetch/messagelogger.cpp
)
ecm_mark_nongui_executable(isbndbfetchertest)
add_test(isbndbfetchertest isbndbfetchertest)
ecm_mark_as_test(isbndbfetchertest)
TARGET_LINK_LIBRARIES(isbndbfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(kinofetchertest kinofetchertest.cpp abstractfetchertest.cpp
  ../fetch/kinofetcher.cpp
)
ecm_mark_nongui_executable(kinofetchertest)
add_test(kinofetchertest kinofetchertest)
ecm_mark_as_test(kinofetchertest)
TARGET_LINK_LIBRARIES(kinofetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(kinopoiskfetchertest kinopoiskfetchertest.cpp abstractfetchertest.cpp
  ../fetch/kinopoiskfetcher.cpp
)
ecm_mark_nongui_executable(kinopoiskfetchertest)
add_test(kinopoiskfetchertest kinopoiskfetchertest)
ecm_mark_as_test(kinopoiskfetchertest)
TARGET_LINK_LIBRARIES(kinopoiskfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(kinoteatrfetchertest kinoteatrfetchertest.cpp abstractfetchertest.cpp
  ../fetch/kinoteatrfetcher.cpp
)
ecm_mark_nongui_executable(kinoteatrfetchertest)
add_test(kinoteatrfetchertest kinoteatrfetchertest)
ecm_mark_as_test(kinoteatrfetchertest)
TARGET_LINK_LIBRARIES(kinoteatrfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(mobygamesfetchertest mobygamesfetchertest.cpp abstractfetchertest.cpp
  ../fetch/mobygamesfetcher.cpp
)
ecm_mark_nongui_executable(mobygamesfetchertest)
add_test(mobygamesfetchertest mobygamesfetchertest)
ecm_mark_as_test(mobygamesfetchertest)
TARGET_LINK_LIBRARIES(mobygamesfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(moviemeterfetchertest moviemeterfetchertest.cpp abstractfetchertest.cpp
  ../fetch/moviemeterfetcher.cpp
)
ecm_mark_nongui_executable(moviemeterfetchertest)
add_test(moviemeterfetchertest moviemeterfetchertest)
ecm_mark_as_test(moviemeterfetchertest)
TARGET_LINK_LIBRARIES(moviemeterfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(mrlookupfetchertest mrlookupfetchertest.cpp abstractfetchertest.cpp
  ../fetch/mrlookupfetcher.cpp
  ../translators/bibteximporter.cpp
)
ecm_mark_nongui_executable(mrlookupfetchertest)
add_test(mrlookupfetchertest mrlookupfetchertest)
ecm_mark_as_test(mrlookupfetchertest)
TARGET_LINK_LIBRARIES(mrlookupfetchertest fetcherstest ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS})

add_executable(multifetchertest multifetchertest.cpp abstractfetchertest.cpp
  ../fetch/multifetcher.cpp
  ../fetch/execexternalfetcher.cpp
  ../fetch/messagelogger.cpp
  ../translators/bibteximporter.cpp
  ../translators/risimporter.cpp
  ../gui/collectiontypecombo.cpp
  ../gui/kwidgetlister.cpp
)
ecm_mark_nongui_executable(multifetchertest)
add_test(multifetchertest multifetchertest)
ecm_mark_as_test(multifetchertest)
TARGET_LINK_LIBRARIES(multifetchertest fetcherstest newstuff ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS})

add_executable(musicbrainzfetchertest musicbrainzfetchertest.cpp abstractfetchertest.cpp
  ../fetch/musicbrainzfetcher.cpp
)
ecm_mark_nongui_executable(musicbrainzfetchertest)
add_test(musicbrainzfetchertest musicbrainzfetchertest)
ecm_mark_as_test(musicbrainzfetchertest)
TARGET_LINK_LIBRARIES(musicbrainzfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(numistafetchertest numistafetchertest.cpp abstractfetchertest.cpp
  ../fetch/numistafetcher.cpp
)
ecm_mark_nongui_executable(numistafetchertest)
add_test(numistafetchertest numistafetchertest)
ecm_mark_as_test(numistafetchertest)
TARGET_LINK_LIBRARIES(numistafetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(openlibraryfetchertest openlibraryfetchertest.cpp abstractfetchertest.cpp
  ../fetch/openlibraryfetcher.cpp
)
ecm_mark_nongui_executable(openlibraryfetchertest)
add_test(openlibraryfetchertest openlibraryfetchertest)
ecm_mark_as_test(openlibraryfetchertest)
TARGET_LINK_LIBRARIES(openlibraryfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(omdbfetchertest omdbfetchertest.cpp abstractfetchertest.cpp
  ../fetch/omdbfetcher.cpp
)
ecm_mark_nongui_executable(omdbfetchertest)
add_test(omdbfetchertest omdbfetchertest)
ecm_mark_as_test(omdbfetchertest)
TARGET_LINK_LIBRARIES(omdbfetchertest fetcherstest translatorstest ${TELLICO_TEST_LIBS})

add_executable(springerfetchertest springerfetchertest.cpp abstractfetchertest.cpp
  ../fetch/springerfetcher.cpp
)
ecm_mark_nongui_executable(springerfetchertest)
add_test(springerfetchertest springerfetchertest)
ecm_mark_as_test(springerfetchertest)
TARGET_LINK_LIBRARIES(springerfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(srufetchertest srufetchertest.cpp abstractfetchertest.cpp
)
ecm_mark_nongui_executable(srufetchertest)
add_test(srufetchertest srufetchertest)
ecm_mark_as_test(srufetchertest)
# Sonnet needed for gui/lineedit
TARGET_LINK_LIBRARIES(srufetchertest
  fetcherstest
  ${TELLICO_TEST_LIBS}
)

add_executable(thegamesdbfetchertest thegamesdbfetchertest.cpp abstractfetchertest.cpp
  ../fetch/thegamesdbfetcher.cpp
)
ecm_mark_nongui_executable(thegamesdbfetchertest)
add_test(thegamesdbfetchertest thegamesdbfetchertest)
ecm_mark_as_test(thegamesdbfetchertest)
TARGET_LINK_LIBRARIES(thegamesdbfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(themoviedbfetchertest themoviedbfetchertest.cpp abstractfetchertest.cpp
  ../fetch/themoviedbfetcher.cpp
)
ecm_mark_nongui_executable(themoviedbfetchertest)
add_test(themoviedbfetchertest themoviedbfetchertest)
ecm_mark_as_test(themoviedbfetchertest)
TARGET_LINK_LIBRARIES(themoviedbfetchertest fetcherstest translatorstest ${TELLICO_TEST_LIBS})

add_executable(tvmazefetchertest tvmazefetchertest.cpp abstractfetchertest.cpp
  ../fetch/tvmazefetcher.cpp
)
ecm_mark_nongui_executable(tvmazefetchertest)
add_test(tvmazefetchertest tvmazefetchertest)
ecm_mark_as_test(tvmazefetchertest)
TARGET_LINK_LIBRARIES(tvmazefetchertest fetcherstest translatorstest ${TELLICO_TEST_LIBS})

add_executable(upcitemdbfetchertest upcitemdbfetchertest.cpp abstractfetchertest.cpp
  ../fetch/upcitemdbfetcher.cpp
  ../translators/bibteximporter.cpp
)
ecm_mark_nongui_executable(upcitemdbfetchertest)
add_test(upcitemdbfetchertest upcitemdbfetchertest)
ecm_mark_as_test(upcitemdbfetchertest)
TARGET_LINK_LIBRARIES(upcitemdbfetchertest fetcherstest ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS})

add_executable(videogamegeekfetchertest videogamegeekfetchertest.cpp abstractfetchertest.cpp
  ../fetch/videogamegeekfetcher.cpp
)
ecm_mark_nongui_executable(videogamegeekfetchertest)
add_test(videogamegeekfetchertest videogamegeekfetchertest)
ecm_mark_as_test(videogamegeekfetchertest)
TARGET_LINK_LIBRARIES(videogamegeekfetchertest fetcherstest ${TELLICO_TEST_LIBS})

add_executable(vndbfetchertest vndbfetchertest.cpp abstractfetchertest.cpp
  ../fetch/vndbfetcher.cpp
)
ecm_mark_nongui_executable(vndbfetchertest)
add_test(vndbfetchertest vndbfetchertest)
ecm_mark_as_test(vndbfetchertest)
TARGET_LINK_LIBRARIES(vndbfetchertest fetcherstest ${TELLICO_TEST_LIBS})

IF( Yaz_FOUND )
  add_executable(z3950fetchertest z3950fetchertest.cpp abstractfetchertest.cpp
    ../fetch/z3950fetcher.cpp
    ../fetch/z3950connection.cpp
    ../translators/grs1importer.cpp
    ../translators/adsimporter.cpp
  )
  ecm_mark_nongui_executable(z3950fetchertest)
  add_test(z3950fetchertest z3950fetchertest)
  ecm_mark_as_test(z3950fetchertest)
  TARGET_LINK_LIBRARIES(z3950fetchertest
                        fetcherstest
                        ${Yaz_LIBRARIES}
                        ${TELLICO_TEST_LIBS}
  )
ENDIF( Yaz_FOUND )

ENDIF(BUILD_FETCHER_TESTS)
