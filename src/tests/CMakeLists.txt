SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# formattest includes fieldformat that include core/tellico_config.h
# which is generated in the build directory
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR}/src )

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
   ../entrycomparison.cpp
   ../field.cpp
   ../fieldformat.cpp
   ../filter.cpp
   ../tellico_utils.cpp
   )

KDE4_ADD_UNIT_TEST(collectiontest NOGUI collectiontest.cpp ${collection_SRCS})
TARGET_LINK_LIBRARIES(collectiontest core utils ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

#KDE4_ADD_UNIT_TEST(risimporttest NOGUI risimporttest.cpp ${collection_SRCS})
#TARGET_LINK_LIBRARIES(risimporttest core utils translators ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${KDE4_KIO_LIBS} ${QT_QTTEST_LIBRARY} )
