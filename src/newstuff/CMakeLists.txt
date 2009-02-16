INCLUDE_DIRECTORIES( ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDE_DIR}  )

########### next target ###############

SET(newstuff_STAT_SRCS
   manager.cpp
   )

QT4_ADD_DBUS_ADAPTOR(newstuff_STAT_SRCS org.tellico_project.newstuff.xml manager.h Tellico::NewStuff::Manager)
KDE4_ADD_LIBRARY(newstuff STATIC ${newstuff_STAT_SRCS})

install(FILES tellico-template.knsrc tellico-script.knsrc DESTINATION ${CONFIG_INSTALL_DIR})

