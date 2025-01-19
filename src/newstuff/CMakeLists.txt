########### next target ###############

set(newstuff_STAT_SRCS
    manager.cpp
)

qt_add_dbus_adaptor(newstuff_STAT_SRCS org.kde.tellico.newstuff.xml manager.h Tellico::NewStuff::Manager)

add_library(newstuff STATIC ${newstuff_STAT_SRCS})

target_link_libraries(newstuff
    utils
    Qt6::Core
    Qt6::Gui
    Qt6::DBus
    KF6::ConfigCore
    KF6::KIOCore
    KF6::Archive
)

if(KF6NewStuff_FOUND)
    install(FILES tellico-template.knsrc DESTINATION ${KDE_INSTALL_KNSRCDIR})
endif()
