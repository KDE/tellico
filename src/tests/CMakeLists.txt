SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# formattest includes fieldformat that include core/tellico_config.h
# which is generated in the build directory
INCLUDE_DIRECTORIES( ${Tellico_BINARY_DIR}/src )

add_executable(entitytest NOGUI entitytest.cpp ../tellico_utils.cpp)
add_test(entitytest entitytest)
ecm_mark_as_test(entitytest)
TARGET_LINK_LIBRARIES(entitytest KF5::KDELibs4Support   Qt5::Test )

add_executable(cuecattest NOGUI cuecattest.cpp)
add_test(cuecattest cuecattest)
ecm_mark_as_test(cuecattest)
TARGET_LINK_LIBRARIES(cuecattest utils KF5::KDELibs4Support  Qt5::Test )

add_executable(isbntest NOGUI isbntest.cpp)
add_test(isbntest isbntest)
ecm_mark_as_test(isbntest)
TARGET_LINK_LIBRARIES(isbntest utils   Qt5::Test )

add_executable(lccntest NOGUI lccntest.cpp)
add_test(lccntest lccntest)
ecm_mark_as_test(lccntest)
TARGET_LINK_LIBRARIES(lccntest utils   Qt5::Test )

add_executable(lcctest NOGUI lcctest.cpp ../field.cpp ../fieldformat.cpp ../tellico_utils.cpp)
add_test(lcctest lcctest)
ecm_mark_as_test(lcctest)
TARGET_LINK_LIBRARIES(lcctest utils core   Qt5::Test )

add_executable(formattest NOGUI formattest.cpp ../fieldformat.cpp)
add_test(formattest formattest)
ecm_mark_as_test(formattest)
TARGET_LINK_LIBRARIES(formattest core   Qt5::Test )

add_executable(fieldtest NOGUI fieldtest.cpp ../field.cpp ../fieldformat.cpp ../tellico_utils.cpp)
add_test(fieldtest fieldtest)
ecm_mark_as_test(fieldtest)
TARGET_LINK_LIBRARIES(fieldtest core   Qt5::Test )

add_executable(comparisontest NOGUI comparisontest.cpp ../field.cpp ../fieldformat.cpp ../tellico_utils.cpp)
add_test(comparisontest comparisontest)
ecm_mark_as_test(comparisontest)
TARGET_LINK_LIBRARIES(comparisontest utils core   Qt5::Test )

add_executable(imagetest NOGUI imagetest.cpp ../tellico_utils.cpp)
add_test(imagetest imagetest)
ecm_mark_as_test(imagetest)
TARGET_LINK_LIBRARIES(imagetest images core   Qt5::Test )

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

ADD_DEPENDENCIES(tellicotest tellico_config)

set(TELLICO_TEST_LIBS
   
   Qt5::Test
   
   KF5::KIOCore
   tellicotest
   collections
   translators
   utils
   core
   images
   core
   gui
   tellicotest
   )

# for some of these, the libraries seem to require being repeated so
# that the order of linking works...no idea why...
add_executable(collectiontest NOGUI collectiontest.cpp)
add_test(collectiontest collectiontest)
ecm_mark_as_test(collectiontest)
TARGET_LINK_LIBRARIES(collectiontest ${TELLICO_TEST_LIBS})

add_executable(filtertest NOGUI filtertest.cpp)
add_test(filtertest filtertest)
ecm_mark_as_test(filtertest)
TARGET_LINK_LIBRARIES(filtertest ${TELLICO_TEST_LIBS})

add_executable(adstest NOGUI adstest.cpp)
add_test(adstest adstest)
ecm_mark_as_test(adstest)
TARGET_LINK_LIBRARIES(adstest ${TELLICO_TEST_LIBS})

add_executable(alexandriatest NOGUI alexandriatest.cpp ../progressmanager.cpp)
add_test(alexandriatest alexandriatest)
ecm_mark_as_test(alexandriatest)
TARGET_LINK_LIBRARIES(alexandriatest ${TELLICO_TEST_LIBS})

add_executable(amctest NOGUI amctest.cpp)
add_test(amctest amctest)
ecm_mark_as_test(amctest)
TARGET_LINK_LIBRARIES(amctest ${TELLICO_TEST_LIBS})

add_executable(bibtextest NOGUI bibtextest.cpp)
add_test(bibtextest bibtextest)
ecm_mark_as_test(bibtextest)
TARGET_LINK_LIBRARIES(bibtextest ${TELLICO_TEST_LIBS})

add_executable(bibtexmltest NOGUI bibtexmltest.cpp)
add_test(bibtexmltest bibtexmltest)
ecm_mark_as_test(bibtexmltest)
TARGET_LINK_LIBRARIES(bibtexmltest ${TELLICO_TEST_LIBS})

add_executable(ciwtest NOGUI ciwtest.cpp)
add_test(ciwtest ciwtest)
ecm_mark_as_test(ciwtest)
TARGET_LINK_LIBRARIES(ciwtest ${TELLICO_TEST_LIBS})

add_executable(csvtest NOGUI csvtest.cpp)
add_test(csvtest csvtest)
ecm_mark_as_test(csvtest)
TARGET_LINK_LIBRARIES(csvtest ${TELLICO_CSV_LIBS} ${TELLICO_TEST_LIBS})

add_executable(delicioustest NOGUI delicioustest.cpp)
add_test(delicioustest delicioustest)
ecm_mark_as_test(delicioustest)
TARGET_LINK_LIBRARIES(delicioustest ${TELLICO_TEST_LIBS})

add_executable(gcstartest NOGUI gcstartest.cpp)
add_test(gcstartest gcstartest)
ecm_mark_as_test(gcstartest)
TARGET_LINK_LIBRARIES(gcstartest ${TELLICO_TEST_LIBS})

add_executable(griffithtest NOGUI griffithtest.cpp)
add_test(griffithtest griffithtest)
ecm_mark_as_test(griffithtest)
TARGET_LINK_LIBRARIES(griffithtest ${TELLICO_TEST_LIBS})

add_executable(referencertest NOGUI referencertest.cpp)
add_test(referencertest referencertest)
ecm_mark_as_test(referencertest)
TARGET_LINK_LIBRARIES(referencertest ${TELLICO_TEST_LIBS})

add_executable(ristest NOGUI ristest.cpp)
add_test(ristest ristest)
ecm_mark_as_test(ristest)
TARGET_LINK_LIBRARIES(ristest ${TELLICO_TEST_LIBS})

add_executable(tellicoreadtest NOGUI tellicoreadtest.cpp)
add_test(tellicoreadtest tellicoreadtest)
ecm_mark_as_test(tellicoreadtest)
TARGET_LINK_LIBRARIES(tellicoreadtest tellicomodels ${TELLICO_TEST_LIBS})

add_executable(vinoxmltest NOGUI vinoxmltest.cpp)
add_test(vinoxmltest vinoxmltest)
ecm_mark_as_test(vinoxmltest)
TARGET_LINK_LIBRARIES(vinoxmltest ${TELLICO_TEST_LIBS})

add_executable(entryupdatejobtest NOGUI entryupdatejobtest.cpp ../entryupdatejob.cpp)
add_test(entryupdatejobtest entryupdatejobtest)
ecm_mark_as_test(entryupdatejobtest)
TARGET_LINK_LIBRARIES(entryupdatejobtest fetch translators ${TELLICO_TEST_LIBS})

# fetcher tests from here down
IF(BUILD_FETCHER_TESTS)

add_executable(allocinefetchertest NOGUI allocinefetchertest.cpp abstractfetchertest.cpp)
add_test(allocinefetchertest allocinefetchertest)
ecm_mark_as_test(allocinefetchertest)
TARGET_LINK_LIBRARIES(allocinefetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(amazonfetchertest NOGUI amazonfetchertest.cpp abstractfetchertest.cpp)
add_test(amazonfetchertest amazonfetchertest)
ecm_mark_as_test(amazonfetchertest)
TARGET_LINK_LIBRARIES(amazonfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(animenfofetchertest NOGUI animenfofetchertest.cpp abstractfetchertest.cpp)
add_test(animenfofetchertest animenfofetchertest)
ecm_mark_as_test(animenfofetchertest)
TARGET_LINK_LIBRARIES(animenfofetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(arxivfetchertest NOGUI arxivfetchertest.cpp abstractfetchertest.cpp)
add_test(arxivfetchertest arxivfetchertest)
ecm_mark_as_test(arxivfetchertest)
TARGET_LINK_LIBRARIES(arxivfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(bibliosharefetchertest NOGUI bibliosharefetchertest.cpp abstractfetchertest.cpp)
add_test(bibliosharefetchertest bibliosharefetchertest)
ecm_mark_as_test(bibliosharefetchertest)
TARGET_LINK_LIBRARIES(bibliosharefetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(boardgamegeekfetchertest NOGUI boardgamegeekfetchertest.cpp abstractfetchertest.cpp)
add_test(boardgamegeekfetchertest boardgamegeekfetchertest)
ecm_mark_as_test(boardgamegeekfetchertest)
TARGET_LINK_LIBRARIES(boardgamegeekfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(citebasefetchertest NOGUI citebasefetchertest.cpp abstractfetchertest.cpp)
add_test(citebasefetchertest citebasefetchertest)
ecm_mark_as_test(citebasefetchertest)
TARGET_LINK_LIBRARIES(citebasefetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(darkhorsefetchertest NOGUI darkhorsefetchertest.cpp abstractfetchertest.cpp)
add_test(darkhorsefetchertest darkhorsefetchertest)
ecm_mark_as_test(darkhorsefetchertest)
TARGET_LINK_LIBRARIES(darkhorsefetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(dblpfetchertest NOGUI dblpfetchertest.cpp abstractfetchertest.cpp)
add_test(dblpfetchertest dblpfetchertest)
ecm_mark_as_test(dblpfetchertest)
TARGET_LINK_LIBRARIES(dblpfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(discogsfetchertest NOGUI discogsfetchertest.cpp abstractfetchertest.cpp)
add_test(discogsfetchertest discogsfetchertest)
ecm_mark_as_test(discogsfetchertest)
TARGET_LINK_LIBRARIES(discogsfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(doubanfetchertest NOGUI doubanfetchertest.cpp abstractfetchertest.cpp)
add_test(doubanfetchertest doubanfetchertest)
ecm_mark_as_test(doubanfetchertest)
TARGET_LINK_LIBRARIES(doubanfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(dvdfrfetchertest NOGUI dvdfrfetchertest.cpp abstractfetchertest.cpp)
add_test(dvdfrfetchertest dvdfrfetchertest)
ecm_mark_as_test(dvdfrfetchertest)
TARGET_LINK_LIBRARIES(dvdfrfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(entrezfetchertest NOGUI entrezfetchertest.cpp abstractfetchertest.cpp)
add_test(entrezfetchertest entrezfetchertest)
ecm_mark_as_test(entrezfetchertest)
TARGET_LINK_LIBRARIES(entrezfetchertest fetch ${TELLICO_TEST_LIBS})

IF( QJSON_FOUND )
    add_executable(freebasefetchertest NOGUI freebasefetchertest.cpp abstractfetchertest.cpp)
add_test(freebasefetchertest freebasefetchertest)
ecm_mark_as_test(freebasefetchertest)
    TARGET_LINK_LIBRARIES(freebasefetchertest fetch ${TELLICO_TEST_LIBS})

    add_executable(filmasterfetchertest NOGUI filmasterfetchertest.cpp abstractfetchertest.cpp)
add_test(filmasterfetchertest filmasterfetchertest)
ecm_mark_as_test(filmasterfetchertest)
    TARGET_LINK_LIBRARIES(filmasterfetchertest fetch ${TELLICO_TEST_LIBS})

    add_executable(masfetchertest NOGUI masfetchertest.cpp abstractfetchertest.cpp)
add_test(masfetchertest masfetchertest)
ecm_mark_as_test(masfetchertest)
    TARGET_LINK_LIBRARIES(masfetchertest fetch ${TELLICO_TEST_LIBS})

    add_executable(googlebookfetchertest NOGUI googlebookfetchertest.cpp abstractfetchertest.cpp)
add_test(googlebookfetchertest googlebookfetchertest)
ecm_mark_as_test(googlebookfetchertest)
    TARGET_LINK_LIBRARIES(googlebookfetchertest fetch ${TELLICO_TEST_LIBS})

ENDIF( QJSON_FOUND )

# running gcstar in the fetcher is really unreliable
#add_executable(gcstarfetchertest NOGUI gcstarfetchertest.cpp)
add_test(gcstarfetchertest gcstarfetchertest)
ecm_mark_as_test(gcstarfetchertest)
#TARGET_LINK_LIBRARIES(gcstarfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(giantbombfetchertest NOGUI giantbombfetchertest.cpp abstractfetchertest.cpp)
add_test(giantbombfetchertest giantbombfetchertest)
ecm_mark_as_test(giantbombfetchertest)
TARGET_LINK_LIBRARIES(giantbombfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(googlescholarfetchertest NOGUI googlescholarfetchertest.cpp abstractfetchertest.cpp)
add_test(googlescholarfetchertest googlescholarfetchertest)
ecm_mark_as_test(googlescholarfetchertest)
TARGET_LINK_LIBRARIES(googlescholarfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(ibsfetchertest NOGUI ibsfetchertest.cpp abstractfetchertest.cpp)
add_test(ibsfetchertest ibsfetchertest)
ecm_mark_as_test(ibsfetchertest)
TARGET_LINK_LIBRARIES(ibsfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(imdbfetchertest NOGUI imdbfetchertest.cpp abstractfetchertest.cpp)
add_test(imdbfetchertest imdbfetchertest)
ecm_mark_as_test(imdbfetchertest)
TARGET_LINK_LIBRARIES(imdbfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(isbndbfetchertest NOGUI isbndbfetchertest.cpp abstractfetchertest.cpp)
add_test(isbndbfetchertest isbndbfetchertest)
ecm_mark_as_test(isbndbfetchertest)
TARGET_LINK_LIBRARIES(isbndbfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(moviemeterfetchertest NOGUI moviemeterfetchertest.cpp abstractfetchertest.cpp)
add_test(moviemeterfetchertest moviemeterfetchertest)
ecm_mark_as_test(moviemeterfetchertest)
TARGET_LINK_LIBRARIES(moviemeterfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(mrlookupfetchertest NOGUI mrlookupfetchertest.cpp abstractfetchertest.cpp)
add_test(mrlookupfetchertest mrlookupfetchertest)
ecm_mark_as_test(mrlookupfetchertest)
TARGET_LINK_LIBRARIES(mrlookupfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(musicbrainzfetchertest NOGUI musicbrainzfetchertest.cpp abstractfetchertest.cpp)
add_test(musicbrainzfetchertest musicbrainzfetchertest)
ecm_mark_as_test(musicbrainzfetchertest)
TARGET_LINK_LIBRARIES(musicbrainzfetchertest fetch ${TELLICO_TEST_LIBS})

IF( QJSON_FOUND )
    add_executable(openlibraryfetchertest NOGUI openlibraryfetchertest.cpp abstractfetchertest.cpp)
add_test(openlibraryfetchertest openlibraryfetchertest)
ecm_mark_as_test(openlibraryfetchertest)
    TARGET_LINK_LIBRARIES(openlibraryfetchertest fetch ${TELLICO_TEST_LIBS})
ENDIF( QJSON_FOUND )

add_executable(springerfetchertest NOGUI springerfetchertest.cpp abstractfetchertest.cpp)
add_test(springerfetchertest springerfetchertest)
ecm_mark_as_test(springerfetchertest)
TARGET_LINK_LIBRARIES(springerfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(srufetchertest NOGUI srufetchertest.cpp abstractfetchertest.cpp)
add_test(srufetchertest srufetchertest)
ecm_mark_as_test(srufetchertest)
TARGET_LINK_LIBRARIES(srufetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(thegamesdbfetchertest NOGUI thegamesdbfetchertest.cpp abstractfetchertest.cpp)
add_test(thegamesdbfetchertest thegamesdbfetchertest)
ecm_mark_as_test(thegamesdbfetchertest)
TARGET_LINK_LIBRARIES(thegamesdbfetchertest fetch ${TELLICO_TEST_LIBS})

IF( QJSON_FOUND )
    add_executable(themoviedbfetchertest NOGUI themoviedbfetchertest.cpp abstractfetchertest.cpp)
add_test(themoviedbfetchertest themoviedbfetchertest)
ecm_mark_as_test(themoviedbfetchertest)
    TARGET_LINK_LIBRARIES(themoviedbfetchertest fetch ${TELLICO_TEST_LIBS})
ENDIF( QJSON_FOUND )

add_executable(vndbfetchertest NOGUI vndbfetchertest.cpp abstractfetchertest.cpp)
add_test(vndbfetchertest vndbfetchertest)
ecm_mark_as_test(vndbfetchertest)
TARGET_LINK_LIBRARIES(vndbfetchertest fetch ${TELLICO_TEST_LIBS})

add_executable(winecomfetchertest NOGUI winecomfetchertest.cpp abstractfetchertest.cpp)
add_test(winecomfetchertest winecomfetchertest)
ecm_mark_as_test(winecomfetchertest)
TARGET_LINK_LIBRARIES(winecomfetchertest fetch ${TELLICO_TEST_LIBS})

IF( YAZ_FOUND )
    add_executable(z3950fetchertest NOGUI z3950fetchertest.cpp abstractfetchertest.cpp)
add_test(z3950fetchertest z3950fetchertest)
ecm_mark_as_test(z3950fetchertest)
    TARGET_LINK_LIBRARIES(z3950fetchertest fetch ${TELLICO_TEST_LIBS})
ENDIF( YAZ_FOUND )

ENDIF(BUILD_FETCHER_TESTS)
