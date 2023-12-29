########### next target ###############

SET(newstuff_STAT_SRCS
   manager.cpp
   )

qt_add_dbus_adaptor(newstuff_STAT_SRCS org.kde.tellico.newstuff.xml manager.h Tellico::NewStuff::Manager)

add_library(newstuff STATIC ${newstuff_STAT_SRCS})

TARGET_LINK_LIBRARIES(newstuff
    utils
    Qt${QT_MAJOR_VERSION}::Core
    Qt${QT_MAJOR_VERSION}::Gui
    Qt${QT_MAJOR_VERSION}::DBus
    KF${KF_MAJOR_VERSION}::ConfigCore
    KF${KF_MAJOR_VERSION}::KIOCore
    KF${KF_MAJOR_VERSION}::Archive
    )

IF( KF${KF_MAJOR_VERSION}NewStuff_FOUND )
  if(${ECM_VERSION} VERSION_GREATER "5.57.0")
    INSTALL(FILES tellico-template.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
  else()
    INSTALL(FILES tellico-template.knsrc DESTINATION ${KDE_INSTALL_CONFDIR})
  endif()
endif()
