SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# formattest includes fieldformat that include core/tellico_config.h
# which is generated in the build directory
INCLUDE_DIRECTORIES( ${Tellico_BINARY_DIR}/src )

KDE4_ADD_UNIT_TEST(entitytest NOGUI entitytest.cpp ../tellico_utils.cpp)
TARGET_LINK_LIBRARIES(entitytest ${KDE4_KDECORE_LIBS}  ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(cuecattest NOGUI cuecattest.cpp)
TARGET_LINK_LIBRARIES(cuecattest utils ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(isbntest NOGUI isbntest.cpp)
TARGET_LINK_LIBRARIES(isbntest utils ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(lccntest NOGUI lccntest.cpp)
TARGET_LINK_LIBRARIES(lccntest utils ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(lcctest NOGUI lcctest.cpp ../fieldformat.cpp)
TARGET_LINK_LIBRARIES(lcctest utils core ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(csvtest NOGUI csvtest.cpp ../translators/csvparser.cpp)
TARGET_LINK_LIBRARIES(csvtest csv ${QT_AND_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(formattest NOGUI formattest.cpp ../fieldformat.cpp)
TARGET_LINK_LIBRARIES(formattest core ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(fieldtest NOGUI fieldtest.cpp ../field.cpp ../fieldformat.cpp ../tellico_utils.cpp)
TARGET_LINK_LIBRARIES(fieldtest core ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

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
   )

KDE4_ADD_LIBRARY(tellicotest STATIC ${tellicotest_SRCS})

ADD_DEPENDENCIES(tellicotest tellico_config)

set(TELLICO_TEST_LIBS
   ${QT_AND_KDECORE_LIBS}
   ${QT_QTTEST_LIBRARY}
   ${KDE4_KDEUI_LIBS}
   ${KDE4_KIO_LIBS}
   ${QIMAGEBLITZ_LIBRARIES}
   ${LIBXML2_LIBRARIES}
   ${LIBXSLT_LIBRARIES}
   ${LIBEXSLT_LIBRARIES}
   tellicotest
   collections
   translators
   utils
   core
   images
   core
   gui
   )

# for some of these, the libraries seem to require being repeated so
# that the order of linking works...no idea why...
KDE4_ADD_UNIT_TEST(collectiontest NOGUI collectiontest.cpp)
TARGET_LINK_LIBRARIES(collectiontest ${TELLICO_TEST_LIBS})

KDE4_ADD_UNIT_TEST(alexandriaimporttest NOGUI alexandriaimporttest.cpp)
TARGET_LINK_LIBRARIES(alexandriaimporttest ${TELLICO_TEST_LIBS})

KDE4_ADD_UNIT_TEST(amcimporttest NOGUI amcimporttest.cpp)
TARGET_LINK_LIBRARIES(amcimporttest ${TELLICO_TEST_LIBS} collections)

KDE4_ADD_UNIT_TEST(bibteximporttest NOGUI bibteximporttest.cpp)
TARGET_LINK_LIBRARIES(bibteximporttest translators ${TELLICO_TEST_LIBS} btparse models tellicotest)

KDE4_ADD_UNIT_TEST(bibtexmlimporttest NOGUI bibtexmlimporttest.cpp)
TARGET_LINK_LIBRARIES(bibtexmlimporttest ${TELLICO_TEST_LIBS})

KDE4_ADD_UNIT_TEST(gcstarimporttest NOGUI gcstarimporttest.cpp)
TARGET_LINK_LIBRARIES(gcstarimporttest translators ${TELLICO_TEST_LIBS})

KDE4_ADD_UNIT_TEST(referencerimporttest NOGUI referencerimporttest.cpp)
TARGET_LINK_LIBRARIES(referencerimporttest translators ${TELLICO_TEST_LIBS})

KDE4_ADD_UNIT_TEST(risimporttest NOGUI risimporttest.cpp)
TARGET_LINK_LIBRARIES(risimporttest ${TELLICO_TEST_LIBS})

KDE4_ADD_UNIT_TEST(tellicoreadtest NOGUI tellicoreadtest.cpp)
TARGET_LINK_LIBRARIES(tellicoreadtest translators ${TELLICO_TEST_LIBS})

KDE4_ADD_UNIT_TEST(arxivfetchertest NOGUI arxivfetchertest.cpp)
TARGET_LINK_LIBRARIES(arxivfetchertest fetch translators ${TELLICO_TEST_LIBS})

KDE4_ADD_UNIT_TEST(citebasefetchertest NOGUI citebasefetchertest.cpp)
TARGET_LINK_LIBRARIES(citebasefetchertest fetch translators ${TELLICO_TEST_LIBS} btparse)

KDE4_ADD_UNIT_TEST(googlescholarfetchertest NOGUI googlescholarfetchertest.cpp)
TARGET_LINK_LIBRARIES(googlescholarfetchertest fetch translators ${TELLICO_TEST_LIBS} btparse)

KDE4_ADD_UNIT_TEST(imdbfetchertest NOGUI imdbfetchertest.cpp)
TARGET_LINK_LIBRARIES(imdbfetchertest fetch translators ${TELLICO_TEST_LIBS})

KDE4_ADD_UNIT_TEST(srufetchertest NOGUI srufetchertest.cpp)
TARGET_LINK_LIBRARIES(srufetchertest fetch translators ${TELLICO_TEST_LIBS})

IF( YAZ_FOUND )
    KDE4_ADD_UNIT_TEST(z3950fetchertest NOGUI z3950fetchertest.cpp)
    TARGET_LINK_LIBRARIES(z3950fetchertest fetch translators ${TELLICO_TEST_LIBS} ${YAZ_LIBRARIES})
ENDIF( YAZ_FOUND )

#KDE4_ADD_UNIT_TEST(entryupdatejobtest NOGUI entryupdatejobtest.cpp ../entryupdatejob.cpp ../entrymerger.cpp)
#TARGET_LINK_LIBRARIES(entryupdatejobtest fetch translators ${TELLICO_TEST_LIBS})
