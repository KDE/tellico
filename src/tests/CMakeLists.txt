include(ECMAddTests)
include(ECMMarkNonGuiExecutable)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})

# formattest includes fieldformat that include core/tellico_config.h
# which is generated in the build directory
include_directories(${Tellico_BINARY_DIR}/src)

ecm_add_test(entitytest.cpp ../fieldformat.cpp
    TEST_NAME entitytest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test utils config
)

ecm_add_test(comparisontest.cpp
    TEST_NAME comparisontest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test tellicotest tellicomodels
)

ecm_add_test(completiontest.cpp ../fieldcompletion.cpp ../field.cpp ../fieldformat.cpp
    TEST_NAME completiontest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF${KF_MAJOR_VERSION}::Completion utils config
)

ecm_add_test(cuecattest.cpp
    TEST_NAME cuecattest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test utils
)

ecm_add_test(isbntest.cpp
    TEST_NAME isbntest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test utils
)

ecm_add_test(lccntest.cpp
    TEST_NAME lccntest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test utils
)

ecm_add_test(lcctest.cpp ../field.cpp ../fieldformat.cpp ../tellico_debug.cpp
    TEST_NAME lcctest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test tellicomodels
)

ecm_add_test(formattest.cpp ../fieldformat.cpp
    TEST_NAME formattest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test config
)

ecm_add_test(fieldtest.cpp ../field.cpp ../fieldformat.cpp
    TEST_NAME fieldtest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test config utils
)

ecm_add_test(imagetest.cpp
    ../utils/tellico_utils.cpp
    ../utils/guiproxy.cpp
    ../utils/cursorsaver.cpp
    ../fieldformat.cpp
    ../tellico_debug.cpp
    TEST_NAME imagetest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test KF${KF_MAJOR_VERSION}::Archive images
)

ecm_add_test(imagejobtest.cpp ../fieldformat.cpp ../tellico_debug.cpp
    TEST_NAME imagejobtest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test  KF${KF_MAJOR_VERSION}::Archive images
)

ecm_add_test(iso6937test.cpp ../tellico_debug.cpp
    TEST_NAME iso6937test
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test utils
)

ecm_add_test(iso5426test.cpp ../tellico_debug.cpp
    TEST_NAME iso5426test
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test utils
)

ecm_add_test(newstufftest.cpp ../fieldformat.cpp ../tellico_debug.cpp
    TEST_NAME newstufftest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test newstuff config
)

ecm_add_test(rtftest.cpp
    TEST_NAME rtftest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test rtf2html-tellico
)

set(tellicotest_SRCS
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
    ../tellico_debug.cpp
)

add_library(tellicotest STATIC ${tellicotest_SRCS})
target_link_libraries(tellicotest
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::ConfigWidgets
)

add_dependencies(tellicotest tellico_config)

set(translatorstest_SRCS
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
target_link_libraries(translatorstest
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::Widgets
    Qt${QT_MAJOR_VERSION}::Xml
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::Archive
    KF${KF_MAJOR_VERSION}::KIOWidgets
    KF${KF_MAJOR_VERSION}::ConfigWidgets
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
    Qt${QT_MAJOR_VERSION}::Test
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::Archive
)

ecm_add_test(collectiontest.cpp
    ../document.cpp
    ../utils/mergeconflictresolver.cpp
    ../translators/tellicoxmlexporter.cpp
    ../translators/tellicozipexporter.cpp
    ../translators/exporter.cpp
    TEST_NAME collectiontest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(commandtest.cpp
    ../commands/collectioncommand.cpp
    ../document.cpp
    ../translators/tellicoxmlexporter.cpp
    ../translators/tellicozipexporter.cpp
    ../translators/exporter.cpp
    TEST_NAME commandtest
    LINK_LIBRARIES translatorstest ${TELLICO_TEST_LIBS}
    # order of library linking here apparently matters
)

ecm_add_test(documenttest.cpp
    ../document.cpp
    ../translators/tellicoxmlexporter.cpp
    ../translators/tellicozipexporter.cpp
    ../translators/exporter.cpp
    TEST_NAME documenttest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(entrycomparisontest.cpp
    TEST_NAME entrycomparisontest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS}
)

ecm_add_test(filtertest.cpp
    ../filter.cpp
    ../filterparser.cpp
    TEST_NAME filtertest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS}
)

ecm_add_test(tellicomodeltest.cpp
    modeltest.cpp
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
    TEST_NAME tellicomodeltest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} ${LIBXML2_LIBRARIES}
)

######################################################

ecm_add_test(adstest.cpp
    ../translators/adsimporter.cpp
    ../translators/importer.cpp
    TEST_NAME adstest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS}
)

ecm_add_test(alexandriatest.cpp
    ../translators/alexandriaimporter.cpp
    ../translators/importer.cpp
    ../translators/alexandriaexporter.cpp
    ../translators/exporter.cpp
    TEST_NAME alexandriatest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS}
)

ecm_add_test(amctest.cpp
    ../translators/amcimporter.cpp
    ../translators/dataimporter.cpp
    ../translators/importer.cpp
    TEST_NAME amctest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS}
)

if(TAGLIB_FOUND)
    ecm_add_test(audiofiletest.cpp
        ../translators/audiofileimporter.cpp
        ../translators/dataimporter.cpp
        ../translators/importer.cpp
        TEST_NAME audiofiletest
        LINK_LIBRARIES ${TELLICO_TEST_LIBS} ${TAGLIB_LIBRARIES}
    )
endif()

ecm_add_test(bibtextest.cpp
    ../translators/bibteximporter.cpp
    ../translators/importer.cpp
    ../translators/bibtexexporter.cpp
    ../translators/exporter.cpp
    TEST_NAME bibtextest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} ${TELLICO_BTPARSE_LIBS}
)

ecm_add_test(bibtexmltest.cpp
    ../translators/bibtexmlimporter.cpp
    ../translators/bibtexmlexporter.cpp
    ../translators/exporter.cpp
    TEST_NAME bibtexmltest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest core
)

ecm_add_test(citetest.cpp
    ../collections/bibtexcollection.cpp
    ../core/filehandler.cpp
    TEST_NAME citetest
    LINK_LIBRARIES Qt${QT_MAJOR_VERSION}::Test cite tellicotest config images
)

ecm_add_test(ciwtest.cpp
    ../translators/ciwimporter.cpp
    ../translators/importer.cpp
    TEST_NAME ciwtest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS}
)

ecm_add_test(collectorztest.cpp
    ../translators/collectorzimporter.cpp
    TEST_NAME collectorztest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(csvtest.cpp
    ../translators/csvparser.cpp
    ../translators/csvimporter.cpp
    ../translators/csvexporter.cpp
    ../translators/exporter.cpp
    ../gui/collectiontypecombo.cpp
    ../gui/combobox.cpp
    ../gui/listwidgetitem.cpp
    ../gui/stringmapwidget.cpp
    ../gui/stringmapdialog.cpp
    ../collectionfieldsdialog.cpp
    TEST_NAME csvtest
    LINK_LIBRARIES ${TELLICO_CSV_LIBS} ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(datacrowtest.cpp
    ../translators/datacrowimporter.cpp
    TEST_NAME datacrowtest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(delicioustest.cpp
    ../translators/deliciousimporter.cpp
    TEST_NAME delicioustest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest rtf2html-tellico
)

ecm_add_test(discogstest.cpp
    ../translators/discogsimporter.cpp
    TEST_NAME discogstest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(fieldwidgettest.cpp
    ../gui/boolfieldwidget.cpp
    ../gui/checkablecombobox.cpp
    ../gui/choicefieldwidget.cpp
    ../gui/datefieldwidget.cpp
    ../gui/datewidget.cpp
    ../gui/spinbox.cpp
    ../gui/linefieldwidget.cpp
    ../gui/lineedit.cpp
    ../gui/numberfieldwidget.cpp
    ../gui/parafieldwidget.cpp
    ../gui/ratingwidget.cpp
    ../gui/ratingfieldwidget.cpp
    ../gui/tablefieldwidget.cpp
    ../gui/urlfieldwidget.cpp
    ../gui/fieldwidget.cpp
    ../document.cpp
    ../fieldcompletion.cpp
    ../translators/tellicoxmlexporter.cpp
    ../translators/tellicozipexporter.cpp
    ../translators/exporter.cpp
    TEST_NAME fieldwidgettest
    LINK_LIBRARIES translatorstest
        ${TELLICO_TEST_LIBS}
        KF${KF_MAJOR_VERSION}::XmlGui
        KF${KF_MAJOR_VERSION}::SonnetCore
        KF${KF_MAJOR_VERSION}::SonnetUi
        KF${KF_MAJOR_VERSION}::TextWidgets
)

ecm_add_test(filelistingtest.cpp
    ../translators/filelistingimporter.cpp
    ../translators/filereader.cpp
    ../translators/filereaderbook.cpp
    ../translators/filereadervideo.cpp
    ../translators/xmphandler.cpp
    ../gui/collectiontypecombo.cpp
    ../gui/combobox.cpp
    TEST_NAME filelistingtest
    LINK_LIBRARIES translatorstest ${TELLICO_TEST_LIBS} KF${KF_MAJOR_VERSION}::Solid #order of libs here is important
)
if(KF${KF_MAJOR_VERSION}FileMetaData_FOUND)
    target_link_libraries(filelistingtest KF${KF_MAJOR_VERSION}::FileMetaData)
endif()
if(Exempi_FOUND)
    target_link_libraries(filelistingtest ${Exempi_LIBRARIES})
endif()

ecm_add_test(gcstartest.cpp
    ../translators/gcstarimporter.cpp
    ../translators/gcstarexporter.cpp
    ../translators/tellicoxmlexporter.cpp
    ../translators/tellicozipexporter.cpp
    ../translators/exporter.cpp
    ../document.cpp
    TEST_NAME gcstartest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(goodreadstest.cpp
    ../translators/goodreadsimporter.cpp
    TEST_NAME goodreadstest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(griffithtest.cpp
    ../translators/griffithimporter.cpp
    TEST_NAME griffithtest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(htmlexportertest.cpp
    ../translators/htmlexporter.cpp
    ../translators/tellicoxmlexporter.cpp
    ../translators/tellicozipexporter.cpp
    ../translators/exporter.cpp
    ../document.cpp
    ../../icons/icons.qrc
    TEST_NAME htmlexportertest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(librarythingtest.cpp
    ../translators/librarythingimporter.cpp
    ../translators/importer.cpp
    TEST_NAME librarythingtest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS}
)

ecm_add_test(marctest.cpp
    ../translators/marcimporter.cpp
    TEST_NAME marctest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(modstest.cpp
    TEST_NAME modstest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(referencertest.cpp
    ../translators/referencerimporter.cpp
    TEST_NAME referencertest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(ristest.cpp
    ../translators/risimporter.cpp
    ../translators/importer.cpp
    TEST_NAME ristest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS}
)

ecm_add_test(tellicoreadtest.cpp
    ../translators/tellicoxmlexporter.cpp
    ../translators/tellicozipexporter.cpp
    ../translators/exporter.cpp
    ../document.cpp
    TEST_NAME tellicoreadtest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

ecm_add_test(vinoxmltest.cpp
    ../translators/vinoxmlimporter.cpp
    TEST_NAME vinoxmltest
    LINK_LIBRARIES ${TELLICO_TEST_LIBS} translatorstest
)

set(fetcherstest_SRCS
    abstractfetchertest.cpp
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
set(fetcherstest_SRCS
    ${fetcherstest_SRCS}
    ../fetch/srufetcher.cpp
    ../fetch/xmlfetcher.cpp
    ../gui/lineedit.cpp
    ../gui/combobox.cpp
    ../gui/stringmapwidget.cpp
)

add_library(fetcherstest STATIC ${fetcherstest_SRCS})
target_link_libraries(fetcherstest
    translatorstest
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::DBus
    KF${KF_MAJOR_VERSION}::ConfigCore
    KF${KF_MAJOR_VERSION}::ConfigGui
    KF${KF_MAJOR_VERSION}::ConfigWidgets
    KF${KF_MAJOR_VERSION}::I18n
    KF${KF_MAJOR_VERSION}::Archive
    KF${KF_MAJOR_VERSION}::IconThemes
    KF${KF_MAJOR_VERSION}::XmlGui
    KF${KF_MAJOR_VERSION}::SonnetCore
    KF${KF_MAJOR_VERSION}::SonnetUi
    KF${KF_MAJOR_VERSION}::KIOGui
    ${LIBXML2_LIBRARIES}
    ${LIBXSLT_LIBRARIES}
    ${LIBXSLT_EXSLT_LIBRARIES}
)
add_dependencies(fetcherstest tellico_config)

# the PDF importer uses CrossRefFetcher, so include the test near the fetchers section
ecm_add_test(pdftest.cpp
    ../translators/pdfimporter.cpp
    ../translators/xmphandler.cpp
    ../translators/ebookimporter.cpp
    ../translators/filereaderbook.cpp
    ../translators/filereader.cpp
    ../entryupdatejob.cpp
    TEST_NAME pdftest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS} #order of libs here is important
)
if(Exempi_FOUND)
    target_link_libraries(pdftest ${Exempi_LIBRARIES})
endif()
if(Poppler_Qt${QT_MAJOR_VERSION}_FOUND)
    target_link_libraries(pdftest Poppler::Qt${QT_MAJOR_VERSION})
endif()
if(KF${KF_MAJOR_VERSION}FileMetaData_FOUND)
    target_link_libraries(pdftest KF${KF_MAJOR_VERSION}::FileMetaData)
endif()

# fetcher tests from here down
if(BUILD_FETCHER_TESTS)

# the fetch library really depends on the GUI static library
# but rather than linking all of that in, selectively choose the
# specific classes used
ecm_add_test(fetchertest.cpp
    ../fetch/fetcherinitializer.cpp
    ../gui/collectiontypecombo.cpp
    ../gui/combobox.cpp
    ../gui/kwidgetlister.cpp
    ../gui/lineedit.cpp
    ../gui/stringmapwidget.cpp
    ../translators/adsimporter.cpp
    ../translators/bibteximporter.cpp
    ../translators/gcstarimporter.cpp
    ../translators/grs1importer.cpp
    ../translators/risimporter.cpp
    TEST_NAME fetchertest
    LINK_LIBRARIES fetch
        translatorstest
        ${TELLICO_TEST_LIBS}
        KF${KF_MAJOR_VERSION}::XmlGui
        KF${KF_MAJOR_VERSION}::SonnetCore
        KF${KF_MAJOR_VERSION}::SonnetUi
)

ecm_add_test(entryupdatejobtest.cpp
    ../entryupdatejob.cpp
    ../fetch/arxivfetcher.cpp
    TEST_NAME entryupdatejobtest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(adsfetchertest.cpp
    ../fetch/adsfetcher.cpp
    ../translators/risimporter.cpp
    TEST_NAME adsfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(amazonfetchertest.cpp
    ../fetch/amazonfetcher.cpp
    ../fetch/amazonrequest.cpp
    ../fetch/messagelogger.cpp
    TEST_NAME amazonfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(arxivfetchertest.cpp
    ../fetch/arxivfetcher.cpp
    TEST_NAME arxivfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(bedethequefetchertest.cpp
    ../fetch/bedethequefetcher.cpp
    TEST_NAME bedethequefetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(bibliosharefetchertest.cpp
    ../fetch/bibliosharefetcher.cpp
    TEST_NAME bibliosharefetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(boardgamegeekfetchertest.cpp
    ../fetch/boardgamegeekfetcher.cpp
    TEST_NAME boardgamegeekfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(comicvinefetchertest.cpp
    ../fetch/comicvinefetcher.cpp
    TEST_NAME comicvinefetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(colnectfetchertest.cpp
    ../fetch/colnectfetcher.cpp
    TEST_NAME colnectfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(crossreffetchertest.cpp
    ../fetch/crossreffetcher.cpp
    TEST_NAME crossreffetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(darkhorsefetchertest.cpp
    ../fetch/execexternalfetcher.cpp
    ../translators/bibteximporter.cpp
    ../translators/risimporter.cpp
    ../gui/collectiontypecombo.cpp
    TEST_NAME darkhorsefetchertest
    LINK_LIBRARIES fetcherstest newstuff ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS}
)

ecm_add_test(dbcfetchertest.cpp
    ../fetch/dbcfetcher.cpp
    TEST_NAME dbcfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(dblpfetchertest.cpp
    ../fetch/dblpfetcher.cpp
    TEST_NAME dblpfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(discogsfetchertest.cpp
    ../fetch/discogsfetcher.cpp
    TEST_NAME discogsfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(doubanfetchertest.cpp
    ../fetch/doubanfetcher.cpp
    TEST_NAME doubanfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(dvdfrfetchertest.cpp
    ../fetch/dvdfrfetcher.cpp
    TEST_NAME dvdfrfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(externalfetchertest.cpp
    ../fetch/execexternalfetcher.cpp
    ../translators/bibteximporter.cpp
    ../translators/risimporter.cpp
    ../gui/collectiontypecombo.cpp
    TEST_NAME externalfetchertest
    LINK_LIBRARIES fetcherstest
        newstuff
        ${TELLICO_BTPARSE_LIBS}
        ${TELLICO_TEST_LIBS}
        KF${KF_MAJOR_VERSION}::SonnetCore
        KF${KF_MAJOR_VERSION}::SonnetUi
        KF${KF_MAJOR_VERSION}::XmlGui
)

ecm_add_test(filmaffinityfetchertest.cpp
    ../fetch/filmaffinityfetcher.cpp
    TEST_NAME filmaffinityfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(filmasterfetchertest.cpp
    ../fetch/filmasterfetcher.cpp
    TEST_NAME filmasterfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(gaminghistoryfetchertest.cpp
    ../fetch/gaminghistoryfetcher.cpp
    TEST_NAME gaminghistoryfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

# running gcstar in the fetcher is really unreliable
#ecm_add_test(gcstarfetchertest.cpp
#    ../fetch/gcstarpluginfetcher.cpp
#    ../fetch/gcstarthread.cpp
#    ../translators/gcstarimporter.cpp
#    ../gui/collectiontypecombo.cpp
#    TEST_NAME gcstarfetchertest
#    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
#)

ecm_add_test(giantbombfetchertest.cpp
    ../fetch/giantbombfetcher.cpp
    TEST_NAME giantbombfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(googlebookfetchertest.cpp
    ../fetch/googlebookfetcher.cpp
    TEST_NAME googlebookfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(googlescholarfetchertest.cpp
    ../fetch/googlescholarfetcher.cpp
    ../translators/bibteximporter.cpp
    TEST_NAME googlescholarfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS}
)

ecm_add_test(hathitrustfetchertest.cpp
    ../fetch/hathitrustfetcher.cpp
    TEST_NAME hathitrustfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(ibsfetchertest.cpp
    ../fetch/ibsfetcher.cpp
    TEST_NAME ibsfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(igdbfetchertest.cpp
    ../fetch/igdbfetcher.cpp
    TEST_NAME igdbfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(imdbfetchertest.cpp
    ../fetch/imdbfetcher.cpp
    ../fetch/imdbfetcher_query.cpp
    TEST_NAME imdbfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(isbndbfetchertest.cpp
    ../fetch/isbndbfetcher.cpp
    ../fetch/messagelogger.cpp
    TEST_NAME isbndbfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(itunesfetchertest.cpp
    ../fetch/itunesfetcher.cpp
    TEST_NAME itunesfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(kinofetchertest.cpp
    ../fetch/kinofetcher.cpp
    TEST_NAME kinofetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(kinopoiskfetchertest.cpp
    ../fetch/kinopoiskfetcher.cpp
    TEST_NAME kinopoiskfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(kinoteatrfetchertest.cpp
    ../fetch/kinoteatrfetcher.cpp
    TEST_NAME kinoteatrfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(mobygamesfetchertest.cpp
    ../fetch/mobygamesfetcher.cpp
    TEST_NAME mobygamesfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(moviemeterfetchertest.cpp
    ../fetch/moviemeterfetcher.cpp
    TEST_NAME moviemeterfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(mrlookupfetchertest.cpp
    ../fetch/mrlookupfetcher.cpp
    ../translators/bibteximporter.cpp
    TEST_NAME mrlookupfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS}
)

ecm_add_test(multifetchertest.cpp
    ../fetch/multifetcher.cpp
    ../fetch/execexternalfetcher.cpp
    ../fetch/messagelogger.cpp
    ../translators/bibteximporter.cpp
    ../translators/risimporter.cpp
    ../gui/collectiontypecombo.cpp
    ../gui/kwidgetlister.cpp
    TEST_NAME multifetchertest
    LINK_LIBRARIES fetcherstest newstuff ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS}
)

ecm_add_test(musicbrainzfetchertest.cpp
    ../fetch/musicbrainzfetcher.cpp
    TEST_NAME musicbrainzfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(numistafetchertest.cpp
    ../fetch/numistafetcher.cpp
    TEST_NAME numistafetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(opdsfetchertest.cpp
    ../fetch/opdsfetcher.cpp
    TEST_NAME opdsfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(openlibraryfetchertest.cpp
    ../fetch/openlibraryfetcher.cpp
    TEST_NAME openlibraryfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(omdbfetchertest.cpp
    ../fetch/omdbfetcher.cpp
    TEST_NAME omdbfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(rpggeekfetchertest.cpp
    ../fetch/rpggeekfetcher.cpp
    TEST_NAME rpggeekfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(springerfetchertest.cpp
    ../fetch/springerfetcher.cpp
    TEST_NAME springerfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

# fetcherstest lib already includes srufetcher.cpp
ecm_add_test(srufetchertest.cpp
    TEST_NAME srufetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(thegamesdbfetchertest.cpp
    ../fetch/thegamesdbfetcher.cpp
    TEST_NAME thegamesdbfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(themoviedbfetchertest.cpp
    ../fetch/themoviedbfetcher.cpp
    TEST_NAME themoviedbfetchertest
    LINK_LIBRARIES fetcherstest translatorstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(thetvdbfetchertest.cpp
    ../fetch/thetvdbfetcher.cpp
    TEST_NAME thetvdbfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(tvmazefetchertest.cpp
    ../fetch/tvmazefetcher.cpp
    TEST_NAME tvmazefetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(upcitemdbfetchertest.cpp
    ../fetch/upcitemdbfetcher.cpp
    ../translators/bibteximporter.cpp
    TEST_NAME upcitemdbfetchertest
    LINK_LIBRARIES fetcherstest translatorstest ${TELLICO_BTPARSE_LIBS} ${TELLICO_TEST_LIBS}
)

ecm_add_test(vgcollectfetchertest.cpp
    ../fetch/vgcollectfetcher.cpp
    TEST_NAME vgcollectfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(videogamegeekfetchertest.cpp
    ../fetch/videogamegeekfetcher.cpp
    TEST_NAME videogamegeekfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

ecm_add_test(vndbfetchertest.cpp
    ../fetch/vndbfetcher.cpp
    TEST_NAME vndbfetchertest
    LINK_LIBRARIES fetcherstest ${TELLICO_TEST_LIBS}
)

if(Yaz_FOUND)
    ecm_add_test(z3950fetchertest.cpp
        ../fetch/z3950fetcher.cpp
        ../fetch/z3950connection.cpp
        ../translators/grs1importer.cpp
        ../translators/adsimporter.cpp
        TEST_NAME z3950fetchertest
        LINK_LIBRARIES fetcherstest ${Yaz_LIBRARIES} ${TELLICO_TEST_LIBS}
    )
endif()

endif()
