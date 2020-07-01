########### next target ###############

SET(newstuff_STAT_SRCS
   manager.cpp
   )

qt5_add_dbus_adaptor(newstuff_STAT_SRCS org.kde.tellico.newstuff.xml manager.h Tellico::NewStuff::Manager)

add_library(newstuff STATIC ${newstuff_STAT_SRCS})

TARGET_LINK_LIBRARIES(newstuff
    utils
    Qt5::Core
    Qt5::Gui
    KF5::ConfigCore
    KF5::KIOCore
    KF5::Archive
    )

IF( KF5NewStuff_FOUND )
  INSTALL(FILES tellico-template.knsrc tellico-script.knsrc DESTINATION ${KDE_INSTALL_CONFDIR})
endif()
