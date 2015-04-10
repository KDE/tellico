include(ECMAddTests)
include(ECMMarkNonGuiExecutable)

SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# formattest includes fieldformat that include core/tellico_config.h
# which is generated in the build directory
INCLUDE_DIRECTORIES( ${Tellico_BINARY_DIR}/src )

add_executable(entitytest entitytest.cpp)
ecm_mark_nongui_executable(entitytest)
add_test(entitytest entitytest)
ecm_mark_as_test(entitytest)
TARGET_LINK_LIBRARIES(entitytest utils Qt5::Test )

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
TARGET_LINK_LIBRARIES(lcctest utils core Qt5::Test)

add_executable(formattest formattest.cpp ../fieldformat.cpp)
ecm_mark_nongui_executable(formattest)
add_test(formattest formattest)
ecm_mark_as_test(formattest)
TARGET_LINK_LIBRARIES(formattest core Qt5::Test)

add_executable(fieldtest fieldtest.cpp ../field.cpp ../fieldformat.cpp)
ecm_mark_nongui_executable(fieldtest)
add_test(fieldtest fieldtest)
ecm_mark_as_test(fieldtest)
TARGET_LINK_LIBRARIES(fieldtest core Qt5::Test)

add_executable(comparisontest comparisontest.cpp ../field.cpp ../fieldformat.cpp)
ecm_mark_nongui_executable(comparisontest)
add_test(comparisontest comparisontest)
ecm_mark_as_test(comparisontest)
TARGET_LINK_LIBRARIES(comparisontest utils core Qt5::Test)

add_executable(imagetest imagetest.cpp ../utils/tellico_utils.cpp ../gui/guiproxy.cpp ../gui/cursorsaver.cpp)
ecm_mark_nongui_executable(imagetest)
add_test(imagetest imagetest)
ecm_mark_as_test(imagetest)
TARGET_LINK_LIBRARIES(imagetest images KF5::Archive Qt5::Test)

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
   ../tellico_utils.cpp
   ../derivedvalue.cpp
   ../document.cpp
   ../progressmanager.cpp
   )

add_library(tellicotest STATIC ${tellicotest_SRCS})
TARGET_LINK_LIBRARIES(tellicotest Qt5::Core Qt5::Gui KF5::I18n)

ADD_DEPENDENCIES(tellicotest tellico_config)

set(TELLICO_TEST_LIBS
   tellicotest
   collections
   translators
   utils
   core
   images
   core
   gui
   tellicotest
   Qt5::Test
   KF5::KIOCore
   )

set(TELLICO2_TEST_LIBS
   tellicotest
   collections
   utils
   images
   Qt5::Test
   KF5::KIOCore
   KF5::Archive
   KF5::KDELibs4Support
   )

# for some of these, the libraries seem to require being repeated so
# that the order of linking works...no idea why...
add_executable(collectiontest collectiontest.cpp)
ecm_mark_nongui_executable(collectiontest)
add_test(collectiontest collectiontest)
ecm_mark_as_test(collectiontest)
TARGET_LINK_LIBRARIES(collectiontest ${TELLICO_TEST_LIBS})

add_executable(filtertest filtertest.cpp)
ecm_mark_nongui_executable(filtertest)
add_test(filtertest filtertest)
ecm_mark_as_test(filtertest)
TARGET_LINK_LIBRARIES(filtertest ${TELLICO_TEST_LIBS})

add_executable(adstest adstest.cpp)
ecm_mark_nongui_executable(adstest)
add_test(adstest adstest)
ecm_mark_as_test(adstest)
TARGET_LINK_LIBRARIES(adstest ${TELLICO_TEST_LIBS})

add_executable(alexandriatest alexandriatest.cpp
  ../translators/alexandriaimporter.cpp
  ../translators/alexandriaexporter.cpp
  ../translators/importer.cpp
  ../translators/exporter.cpp
  ../gui/cursorsaver.cpp
  ../gui/guiproxy.cpp
  ../translators/bibtexhandler.cpp
)
ecm_mark_nongui_executable(alexandriatest)
add_test(alexandriatest alexandriatest)
ecm_mark_as_test(alexandriatest)
TARGET_LINK_LIBRARIES(alexandriatest ${TELLICO2_TEST_LIBS})

add_executable(amctest amctest.cpp
  ../translators/amcimporter.cpp
  ../translators/dataimporter.cpp
  ../translators/importer.cpp
  ../gui/cursorsaver.cpp
  ../gui/guiproxy.cpp
  ../translators/bibtexhandler.cpp
)
ecm_mark_nongui_executable(amctest)
add_test(amctest amctest)
ecm_mark_as_test(amctest)
TARGET_LINK_LIBRARIES(amctest ${TELLICO2_TEST_LIBS})

add_executable(bibtextest bibtextest.cpp)
ecm_mark_nongui_executable(bibtextest)
add_test(bibtextest bibtextest)
ecm_mark_as_test(bibtextest)
TARGET_LINK_LIBRARIES(bibtextest ${TELLICO_TEST_LIBS})

add_executable(bibtexmltest bibtexmltest.cpp)
ecm_mark_nongui_executable(bibtexmltest)
add_test(bibtexmltest bibtexmltest)
ecm_mark_as_test(bibtexmltest)
TARGET_LINK_LIBRARIES(bibtexmltest ${TELLICO_TEST_LIBS})

add_executable(ciwtest ciwtest.cpp)
ecm_mark_nongui_executable(ciwtest)
add_test(ciwtest ciwtest)
ecm_mark_as_test(ciwtest)
TARGET_LINK_LIBRARIES(ciwtest ${TELLICO_TEST_LIBS})

add_executable(csvtest csvtest.cpp)
ecm_mark_nongui_executable(csvtest)
add_test(csvtest csvtest)
ecm_mark_as_test(csvtest)
TARGET_LINK_LIBRARIES(csvtest ${TELLICO_CSV_LIBS} ${TELLICO_TEST_LIBS})

add_executable(delicioustest delicioustest.cpp)
ecm_mark_nongui_executable(delicioustest)
add_test(delicioustest delicioustest)
ecm_mark_as_test(delicioustest)
TARGET_LINK_LIBRARIES(delicioustest ${TELLICO_TEST_LIBS})

add_executable(gcstartest gcstartest.cpp)
ecm_mark_nongui_executable(gcstartest)
add_test(gcstartest gcstartest)
ecm_mark_as_test(gcstartest)
TARGET_LINK_LIBRARIES(gcstartest ${TELLICO_TEST_LIBS})

add_executable(griffithtest griffithtest.cpp)
ecm_mark_nongui_executable(griffithtest)
add_test(griffithtest griffithtest)
ecm_mark_as_test(griffithtest)
TARGET_LINK_LIBRARIES(griffithtest ${TELLICO_TEST_LIBS})

add_executable(referencertest referencertest.cpp)
ecm_mark_nongui_executable(referencertest)
add_test(referencertest referencertest)
ecm_mark_as_test(referencertest)
TARGET_LINK_LIBRARIES(referencertest ${TELLICO_TEST_LIBS})

add_executable(ristest ristest.cpp)
ecm_mark_nongui_executable(ristest)
add_test(ristest ristest)
ecm_mark_as_test(ristest)
TARGET_LINK_LIBRARIES(ristest ${TELLICO_TEST_LIBS})

add_executable(tellicoreadtest tellicoreadtest.cpp)
ecm_mark_nongui_executable(tellicoreadtest)
add_test(tellicoreadtest tellicoreadtest)
ecm_mark_as_test(tellicoreadtest)
TARGET_LINK_LIBRARIES(tellicoreadtest tellicomodels ${TELLICO_TEST_LIBS})

add_executable(vinoxmltest vinoxmltest.cpp)
ecm_mark_nongui_executable(vinoxmltest)
add_test(vinoxmltest vinoxmltest)
ecm_mark_as_test(vinoxmltest)
TARGET_LINK_LIBRARIES(vinoxmltest ${TELLICO_TEST_LIBS})

add_executable(entryupdatejobtest entryupdatejobtest.cpp ../entryupdatejob.cpp)
ecm_mark_nongui_executable(entryupdatejobtest)
add_test(entryupdatejobtest entryupdatejobtest)
ecm_mark_as_test(entryupdatejobtest)
TARGET_LINK_LIBRARIES(entryupdatejobtest fetch translators ${TELLICO_TEST_LIBS})

# fetcher tests from here down
IF(BUILD_FETCHER_TESTS)

add_executable(allocinefetchertest allocinefetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(allocinefetchertest)
add_test(allocinefetchertest allocinefetchertest)
ecm_mark_as_test(allocinefetchertest)
TARGET_LINK_LIBRARIES(allocinefetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(amazonfetchertest amazonfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(amazonfetchertest)
add_test(amazonfetchertest amazonfetchertest)
ecm_mark_as_test(amazonfetchertest)
TARGET_LINK_LIBRARIES(amazonfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(animenfofetchertest animenfofetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(animenfofetchertest)
add_test(animenfofetchertest animenfofetchertest)
ecm_mark_as_test(animenfofetchertest)
TARGET_LINK_LIBRARIES(animenfofetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(arxivfetchertest arxivfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(arxivfetchertest)
add_test(arxivfetchertest arxivfetchertest)
ecm_mark_as_test(arxivfetchertest)
TARGET_LINK_LIBRARIES(arxivfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(bibliosharefetchertest bibliosharefetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(bibliosharefetchertest)
add_test(bibliosharefetchertest bibliosharefetchertest)
ecm_mark_as_test(bibliosharefetchertest)
TARGET_LINK_LIBRARIES(bibliosharefetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(boardgamegeekfetchertest boardgamegeekfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(boardgamegeekfetchertest)
add_test(boardgamegeekfetchertest boardgamegeekfetchertest)
ecm_mark_as_test(boardgamegeekfetchertest)
TARGET_LINK_LIBRARIES(boardgamegeekfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(citebasefetchertest citebasefetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(citebasefetchertest)
add_test(citebasefetchertest citebasefetchertest)
ecm_mark_as_test(citebasefetchertest)
TARGET_LINK_LIBRARIES(citebasefetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(darkhorsefetchertest darkhorsefetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(darkhorsefetchertest)
add_test(darkhorsefetchertest darkhorsefetchertest)
ecm_mark_as_test(darkhorsefetchertest)
TARGET_LINK_LIBRARIES(darkhorsefetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(dblpfetchertest dblpfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(dblpfetchertest)
add_test(dblpfetchertest dblpfetchertest)
ecm_mark_as_test(dblpfetchertest)
TARGET_LINK_LIBRARIES(dblpfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(discogsfetchertest discogsfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(discogsfetchertest)
add_test(discogsfetchertest discogsfetchertest)
ecm_mark_as_test(discogsfetchertest)
TARGET_LINK_LIBRARIES(discogsfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(doubanfetchertest doubanfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(doubanfetchertest)
add_test(doubanfetchertest doubanfetchertest)
ecm_mark_as_test(doubanfetchertest)
TARGET_LINK_LIBRARIES(doubanfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(dvdfrfetchertest dvdfrfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(dvdfrfetchertest)
add_test(dvdfrfetchertest dvdfrfetchertest)
ecm_mark_as_test(dvdfrfetchertest)
TARGET_LINK_LIBRARIES(dvdfrfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(entrezfetchertest entrezfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(entrezfetchertest)
add_test(entrezfetchertest entrezfetchertest)
ecm_mark_as_test(entrezfetchertest)
TARGET_LINK_LIBRARIES(entrezfetchertest fetch ${TELLICO_TEST_LIBS})

IF( QJSON_FOUND )
    add_executable(freebasefetchertest freebasefetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(freebasefetchertest)
add_test(freebasefetchertest freebasefetchertest)
ecm_mark_as_test(freebasefetchertest)
    TARGET_LINK_LIBRARIES(freebasefetchertest fetch ${TELLICO_TEST_LIBS})

    add_executable(filmasterfetchertest filmasterfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(filmasterfetchertest)
add_test(filmasterfetchertest filmasterfetchertest)
ecm_mark_as_test(filmasterfetchertest)
    TARGET_LINK_LIBRARIES(filmasterfetchertest fetch ${TELLICO_TEST_LIBS})

    add_executable(masfetchertest masfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(masfetchertest)
add_test(masfetchertest masfetchertest)
ecm_mark_as_test(masfetchertest)
    TARGET_LINK_LIBRARIES(masfetchertest fetch ${TELLICO_TEST_LIBS})

    add_executable(googlebookfetchertest googlebookfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(googlebookfetchertest)
add_test(googlebookfetchertest googlebookfetchertest)
ecm_mark_as_test(googlebookfetchertest)
    TARGET_LINK_LIBRARIES(googlebookfetchertest fetch ${TELLICO_TEST_LIBS})

ENDIF( QJSON_FOUND )

# running gcstar in the fetcher is really unreliable
#add_executable(gcstarfetchertest gcstarfetchertest.cpp)
#ecm_mark_nongui_executable(gcstarfetchertest)
#add_test(gcstarfetchertest gcstarfetchertest)
#ecm_mark_as_test(gcstarfetchertest)
#TARGET_LINK_LIBRARIES(gcstarfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(giantbombfetchertest giantbombfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(giantbombfetchertest)
add_test(giantbombfetchertest giantbombfetchertest)
ecm_mark_as_test(giantbombfetchertest)
TARGET_LINK_LIBRARIES(giantbombfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(googlescholarfetchertest googlescholarfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(googlescholarfetchertest)
add_test(googlescholarfetchertest googlescholarfetchertest)
ecm_mark_as_test(googlescholarfetchertest)
TARGET_LINK_LIBRARIES(googlescholarfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(ibsfetchertest ibsfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(ibsfetchertest)
add_test(ibsfetchertest ibsfetchertest)
ecm_mark_as_test(ibsfetchertest)
TARGET_LINK_LIBRARIES(ibsfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(imdbfetchertest imdbfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(imdbfetchertest)
add_test(imdbfetchertest imdbfetchertest)
ecm_mark_as_test(imdbfetchertest)
TARGET_LINK_LIBRARIES(imdbfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(isbndbfetchertest isbndbfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(isbndbfetchertest)
add_test(isbndbfetchertest isbndbfetchertest)
ecm_mark_as_test(isbndbfetchertest)
TARGET_LINK_LIBRARIES(isbndbfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(moviemeterfetchertest moviemeterfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(moviemeterfetchertest)
add_test(moviemeterfetchertest moviemeterfetchertest)
ecm_mark_as_test(moviemeterfetchertest)
TARGET_LINK_LIBRARIES(moviemeterfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(mrlookupfetchertest mrlookupfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(mrlookupfetchertest)
add_test(mrlookupfetchertest mrlookupfetchertest)
ecm_mark_as_test(mrlookupfetchertest)
TARGET_LINK_LIBRARIES(mrlookupfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(musicbrainzfetchertest musicbrainzfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(musicbrainzfetchertest)
add_test(musicbrainzfetchertest musicbrainzfetchertest)
ecm_mark_as_test(musicbrainzfetchertest)
TARGET_LINK_LIBRARIES(musicbrainzfetchertest fetch ${TELLICO_TEST_LIBS})

IF( QJSON_FOUND )
    add_executable(openlibraryfetchertest openlibraryfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(openlibraryfetchertest)
add_test(openlibraryfetchertest openlibraryfetchertest)
ecm_mark_as_test(openlibraryfetchertest)
    TARGET_LINK_LIBRARIES(openlibraryfetchertest fetch ${TELLICO_TEST_LIBS})
ENDIF( QJSON_FOUND )

add_executable(springerfetchertest springerfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(springerfetchertest)
add_test(springerfetchertest springerfetchertest)
ecm_mark_as_test(springerfetchertest)
TARGET_LINK_LIBRARIES(springerfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(srufetchertest srufetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(srufetchertest)
add_test(srufetchertest srufetchertest)
ecm_mark_as_test(srufetchertest)
TARGET_LINK_LIBRARIES(srufetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(thegamesdbfetchertest thegamesdbfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(thegamesdbfetchertest)
add_test(thegamesdbfetchertest thegamesdbfetchertest)
ecm_mark_as_test(thegamesdbfetchertest)
TARGET_LINK_LIBRARIES(thegamesdbfetchertest fetch ${TELLICO_TEST_LIBS})

IF( QJSON_FOUND )
    add_executable(themoviedbfetchertest themoviedbfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(themoviedbfetchertest)
add_test(themoviedbfetchertest themoviedbfetchertest)
ecm_mark_as_test(themoviedbfetchertest)
    TARGET_LINK_LIBRARIES(themoviedbfetchertest fetch ${TELLICO_TEST_LIBS})
ENDIF( QJSON_FOUND )

add_executable(vndbfetchertest vndbfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(vndbfetchertest)
add_test(vndbfetchertest vndbfetchertest)
ecm_mark_as_test(vndbfetchertest)
TARGET_LINK_LIBRARIES(vndbfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(winecomfetchertest winecomfetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(winecomfetchertest)
add_test(winecomfetchertest winecomfetchertest)
ecm_mark_as_test(winecomfetchertest)
TARGET_LINK_LIBRARIES(winecomfetchertest fetch ${TELLICO_TEST_LIBS})

IF( YAZ_FOUND )
    add_executable(z3950fetchertest z3950fetchertest.cpp abstractfetchertest.cpp)
ecm_mark_nongui_executable(z3950fetchertest)
add_test(z3950fetchertest z3950fetchertest)
ecm_mark_as_test(z3950fetchertest)
    TARGET_LINK_LIBRARIES(z3950fetchertest fetch ${TELLICO_TEST_LIBS})
ENDIF( YAZ_FOUND )

ENDIF(BUILD_FETCHER_TESTS)
