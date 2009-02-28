SET( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

# formattest includes fieldformat that include core/tellico_config.h
# which is generated in the build directory
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR}/src )

KDE4_ADD_UNIT_TEST(isbntest NOGUI isbntest.cpp ../isbnvalidator.cpp ../upcvalidator.cpp)
TARGET_LINK_LIBRARIES(isbntest ${QT_AND_KDECORE_LIBS}  ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(entitytest NOGUI entitytest.cpp ../tellico_utils.cpp)
TARGET_LINK_LIBRARIES(entitytest ${KDE4_KDECORE_LIBS}  ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(cuecattest NOGUI cuecattest.cpp ../isbnvalidator.cpp ../upcvalidator.cpp)
TARGET_LINK_LIBRARIES(cuecattest ${KDE4_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(lccntest NOGUI lccntest.cpp ../lccnvalidator.cpp)
TARGET_LINK_LIBRARIES(lccntest ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

# fix this someday
#KDE4_ADD_UNIT_TEST(lcctest NOGUI lcctest.cpp ../lcccomparison.cpp)
#TARGET_LINK_LIBRARIES(lcctest ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} )

KDE4_ADD_UNIT_TEST(csvtest NOGUI csvtest.cpp ../translators/csvparser.cpp)
TARGET_LINK_LIBRARIES(csvtest ${QT_AND_KDECORE_LIBS} ${QT_QTTEST_LIBRARY} csv)

KDE4_ADD_UNIT_TEST(formattest NOGUI formattest.cpp ../fieldformat.cpp)
TARGET_LINK_LIBRARIES(formattest ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} core)

#KDE4_ADD_UNIT_TEST(risimporttest NOGUI risimporttest.cpp ../document.cpp)
#TARGET_LINK_LIBRARIES(risimporttest ${QT_AND_KDECORE_LIBS} ${KDE4_KDEUI_LIBS} ${QT_QTTEST_LIBRARY} core collections translators)
