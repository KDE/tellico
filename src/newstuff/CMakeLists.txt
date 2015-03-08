
########### next target ###############

SET(newstuff_STAT_SRCS
   manager.cpp
   )

qt5_add_dbus_adaptor(newstuff_STAT_SRCS org.kde.tellico.newstuff.xml manager.h Tellico::NewStuff::Manager)
add_library(newstuff STATIC ${newstuff_STAT_SRCS})

INSTALL(FILES tellico-template.knsrc tellico-script.knsrc DESTINATION ${CONFIG_INSTALL_DIR})
