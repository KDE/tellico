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

SET(collection_SRCS
   ../collection.cpp
   ../entry.cpp
   ../entrygroup.cpp
   ../entrycomparison.cpp
   ../field.cpp
   ../fieldformat.cpp
   ../filter.cpp
   ../tellico_utils.cpp
   )

set(IMPORTER_LIBS
   ${QT_AND_KDECORE_LIBS}
   ${QT_QTTEST_LIBRARY}
   ${KDE4_KDEUI_LIBS}
   ${KDE4_KIO_LIBS}
   ${QIMAGEBLITZ_LIBRARIES}
   core
   gui
   utils
   images
   collections
   translators
   )

KDE4_ADD_UNIT_TEST(collectiontest NOGUI collectiontest.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(collectiontest core utils ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(alexandriaimporttest NOGUI alexandriaimporttest.cpp ../translators/alexandriaimporter.cpp
                   ../core/filehandler.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(alexandriaimporttest ${IMPORTER_LIBS} )

KDE4_ADD_UNIT_TEST(amcimporttest NOGUI amcimporttest.cpp ../translators/amcimporter.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(amcimporttest ${IMPORTER_LIBS} )

KDE4_ADD_UNIT_TEST(bibteximporttest NOGUI bibteximporttest.cpp ../translators/bibteximporter.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(bibteximporttest btparse ${IMPORTER_LIBS} )

KDE4_ADD_UNIT_TEST(bibtexmlimporttest NOGUI bibtexmlimporttest.cpp ../translators/bibtexmlimporter.cpp
                   ../core/filehandler.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(bibtexmlimporttest ${IMPORTER_LIBS} )

KDE4_ADD_UNIT_TEST(gcstarimporttest NOGUI gcstarimporttest.cpp ../translators/gcstarimporter.cpp
                   ../borrower.cpp ../collectionfactory.cpp ../collections/bibtexcollection.cpp ../core/filehandler.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(gcstarimporttest ${IMPORTER_LIBS} ${LIBXML2_LIBRARIES} ${LIBXSLT_LIBRARIES} ${LIBEXSLT_LIBRARIES})

KDE4_ADD_UNIT_TEST(referencerimporttest NOGUI referencerimporttest.cpp ../translators/referencerimporter.cpp
                   ../borrower.cpp ../collectionfactory.cpp ../collections/bibtexcollection.cpp ../core/filehandler.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(referencerimporttest ${IMPORTER_LIBS} ${LIBXML2_LIBRARIES} ${LIBXSLT_LIBRARIES} ${LIBEXSLT_LIBRARIES})

KDE4_ADD_UNIT_TEST(risimporttest NOGUI risimporttest.cpp ../translators/risimporter.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(risimporttest ${IMPORTER_LIBS} )

KDE4_ADD_UNIT_TEST(tellicoreadtest NOGUI tellicoreadtest.cpp ../translators/tellicoimporter.cpp
                   ../borrower.cpp ../collectionfactory.cpp ../collections/bibtexcollection.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(tellicoreadtest ${IMPORTER_LIBS} )
